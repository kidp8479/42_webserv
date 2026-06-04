#include "CgiSpawner.hpp"

#include <ctype.h>
#include <unistd.h>

#include "../core/CgiProcess.hpp"
#include "../core/CgiStdinWriter.hpp"
#include "../core/Client.hpp"
#include "../core/FdUtils.hpp"

/**
 * @brief Constructs CGI spawner bound to event loop.
 */
CgiSpawner::CgiSpawner(EventLoop& loop) : loop_(loop) {
}

/**
 * @brief Destructor.
 */
CgiSpawner::~CgiSpawner() {
}

/**
 * @brief Validates CGI script before execution.
 * Checks script path resolution and filesystem permissions.
 *
 * @param request Incoming HTTP request.
 * @param location Matched location configuration.
 * @param client Client to report HTTP errors on failure.
 * @param out_script_path Resolved absolute script path.
 *
 * @return true if script exists and is executable, false otherwise.
 * @note Returns HTTP errors directly to client:
 *       - 404 if file not found
 *       - 403 if permission denied
 *       - 500 for resolution errors
 */
bool CgiSpawner::validateScript(const Request& request,
                                const LocationConfig& location, Client& client,
                                std::string& out_script_path) {
    try {
        out_script_path = buildScriptPath(request, location);
    } catch (const std::exception& e) {
        LOG_WARNING() << "[CgiSpawner] " << e.what();
        client.receiveError(HttpConstants::kInternalServerError);
        return false;
    }
    if (access(out_script_path.c_str(), F_OK) != 0) {
        LOG_WARNING() << "[CgiSpawner] path not found";
        client.receiveError(HttpConstants::kNotFound);
        return false;
    }
    if (access(out_script_path.c_str(), X_OK) != 0) {
        LOG_WARNING() << "[CgiSpawner] Permission Denied";
        client.receiveError(HttpConstants::kForbidden);
        return false;
    }
    return true;
}

/**
 * @brief Spawns a CGI process for the given request.
 *
 * Prepares execution environment, creates pipes, forks a child process,
 * and wires stdout/stdin through async event handlers.
 *
 * Flow:
 * - Validate script and interpreter
 * - Build CGI environment variables
 * - Create stdin/stdout pipes
 * - Fork process
 * - Child:
 *   - Redirects stdin/stdout
 *   - Executes CGI script via execve
 * - Parent:
 *   - Registers CgiProcess (stdout reader)
 *   - Optionally registers CgiStdinWriter (request body writer)
 *
 * @param request Incoming HTTP request.
 * @param location Matched location configuration.
 * @param client Client owning this CGI execution.
 *
 * @return true if CGI was successfully spawned, false otherwise.
 *
 * @note Client owns lifecycle coordination via setPendingCgi().
 * @note Pipes must be correctly closed in both parent and child
 *       to avoid FD leaks.
 */
bool CgiSpawner::spawn(const Request& request, const LocationConfig& location,
                       Client& client) {
    std::string script_path;
    if (!validateScript(request, location, client, script_path)) {
        return false;
    }
    std::string interpreter = resolveInterpreter(request, location);
    if (interpreter.empty()) {
        client.receiveError(HttpConstants::kInternalServerError);
        return false;
    }
    std::vector<std::string> env_strings =
        buildEnvStrings(request, script_path, client);
    std::vector<char*> envp = buildEnvp(env_strings);
    int stdin_pipe[2];
    int stdout_pipe[2];

    bool has_body = !request.getBody().empty();

    if (!createPipes(stdin_pipe, stdout_pipe, has_body)) {
        client.receiveError(HttpConstants::kInternalServerError);
        return false;
    }
    pid_t pid = fork();
    if (pid < 0) {
        if (has_body) {
            close(stdin_pipe[0]);
            close(stdin_pipe[1]);
        }
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        client.receiveError(HttpConstants::kInternalServerError);
        return false;
    }
    if (pid == 0) {
        dup2(stdout_pipe[1], STDOUT_FILENO);
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);

        if (has_body) {
            dup2(stdin_pipe[0], STDIN_FILENO);
            close(stdin_pipe[0]);
            close(stdin_pipe[1]);
        } else {
            int devnull = open("/dev/null", O_RDONLY);
            if (devnull != -1) {
                dup2(devnull, STDIN_FILENO);
                close(devnull);
            }
        }
        std::string dir = script_path.substr(0, script_path.rfind('/'));
        std::string filename = script_path.substr(script_path.rfind('/') + 1);
        if (chdir(dir.c_str()) == -1) {
            write(STDERR_FILENO, "[CgiSpawner] chdir failed\n", 26);
            _exit(1);
        }
        char* argv[] = {const_cast<char*>(interpreter.c_str()),
                        const_cast<char*>(filename.c_str()), NULL};
        execve(interpreter.c_str(), argv, &envp[0]);
        write(STDERR_FILENO, "[CgiSpawner] execve failed\n", 27);
        _exit(1);
    }
    close(stdout_pipe[1]);  // parent doesnt write stdout pipe
    if (has_body) {
        close(stdin_pipe[0]);  // parent doesn read stdin pipe
        const std::string& body = request.getBody();
        CgiStdinWriter* writer = new CgiStdinWriter(stdin_pipe[1], body, loop_);
        loop_.addHandler(writer, POLLOUT);
    }
    FdUtils::setNonBlocking(stdout_pipe[0]);
    CgiProcess* cgi = new CgiProcess(pid, stdout_pipe[0], client, loop_);
    client.setPendingCgi(cgi);
    loop_.addHandler(cgi, POLLIN);
    return true;
}

/**
 * @brief Resolves CGI interpreter from request extension.
 * Maps file extension (e.g. ".py") to configured interpreter path.
 * Example:
 * - /cgi-bin/upload.py -> ".py" -> "/usr/bin/python3"
 *
 * @return Interpreter path if found, otherwise empty string.
 * @note Request path is expected to be validated before reaching this step.
 * @note Returns empty string if no matching CGI handler exists.
 */
std::string CgiSpawner::resolveInterpreter(const Request& request,
                                           const LocationConfig& location) {
    const std::string& uri = request.getPath();
    size_t dot_pos = uri.rfind('.');
    if (dot_pos == std::string::npos) {
        return "";
    }
    std::string extension = uri.substr(dot_pos);
    const std::map<std::string, std::string>& cgi_map =
        location.getCgiInterpreters();
    std::map<std::string, std::string>::const_iterator it =
        cgi_map.find(extension);
    if (it == cgi_map.end()) {
        return "";
    }
    return it->second;
}

/**
 * @brief Creates pipes for CGI stdin/stdout redirection.
 * If request has a body, a stdin pipe is created for streaming input
 * to the CGI process. A stdout pipe is always created to capture output.
 *
 * @param stdin_pipe Pipe used to send request body to CGI (if needed).
 * @param stdout_pipe Pipe used to read CGI output.
 * @param has_body Whether request contains a body.
 *
 * @return true on success, false on failure.
 * @note On failure, any allocated file descriptors are cleaned up.
 */
bool CgiSpawner::createPipes(int stdin_pipe[2], int stdout_pipe[2],
                             bool has_body) {
    if (has_body && pipe(stdin_pipe) == -1) {
        LOG_ERROR() << "[CgiSpawner] failed to create stdin pipe";
        return false;
    }
    if (pipe(stdout_pipe) == -1) {
        LOG_ERROR() << "[CgiSpawner] failed to create stdout pipe";
        if (has_body) {
            close(stdin_pipe[0]);
            close(stdin_pipe[1]);
        }
        return false;
    }
    return true;
}

/**
 * @brief Builds filesystem path for CGI script execution.
 * Converts request URI into an absolute script path using the location root.
 *
 * @throws std::runtime_error if root or URI is invalid.
 * @note Strips location prefix if present before joining with root.
 */
std::string CgiSpawner::buildScriptPath(const Request& request,
                                        const LocationConfig& location) {
    const std::string& root = location.getRoot();
    const std::string& uri = request.getPath();
    const std::string& prefix = location.getPath();

    if (root.empty()) {
        LOG_ERROR() << "[CgiSpawner] missing root";
        throw std::runtime_error("[CGI] missing root");
    }
    if (uri.empty() || uri[0] != '/') {
        LOG_ERROR() << "[CgiSpawner] invalid request uri";
        throw std::runtime_error("[CGI] invalid request uri");
    }
    std::string relative_uri = uri;
    if (uri.compare(0, prefix.size(), prefix) == 0) {
        relative_uri = uri.substr(prefix.size());
    }
    if (relative_uri.empty() || relative_uri[0] != '/') {
        relative_uri = "/" + relative_uri;
    }
    if (!root.empty() && root[root.size() - 1] == '/') {
        return root.substr(0, root.size() - 1) + relative_uri;
    }
    return root + relative_uri;
}

/**
 * @brief Builds CGI environment variables from request and client data.
 * Populates a full CGI/1.1 compliant environment including:
 * - Standard CGI fields (REQUEST_METHOD, QUERY_STRING, etc.)
 * - Server metadata (host, port, software)
 * - Request headers as HTTP_* variables
 *
 * @note CONTENT_LENGTH and CONTENT_TYPE are always derived from request body
 *       and headers.
 * @note Headers are forwarded as HTTP_* except Content-Type and Content-Length.
 * @note Used directly by execve() in CGI child process.
 */
std::vector<std::string> CgiSpawner::buildEnvStrings(
    const Request& request, const std::string& script_path,
    const Client& client) {
    std::vector<std::string> env;

    env.push_back("AUTH_TYPE=");
    env.push_back("GATEWAY_INTERFACE=CGI/1.1");
    env.push_back("PATH_INFO=");
    env.push_back("PATH_TRANSLATED=");
    env.push_back("QUERY_STRING=" + request.getQuery());
    env.push_back("REMOTE_ADDR=" + client.getPeerIp());
    env.push_back("REMOTE_HOST=" + client.getPeerIp());
    env.push_back("REMOTE_USER=");
    env.push_back("REQUEST_METHOD=" + request.getMethod());
    env.push_back("SCRIPT_NAME=" + request.getPath());
    env.push_back("SERVER_NAME=" + request.getHeaderValue("Host"));
    env.push_back("SERVER_PROTOCOL=" + request.getProtocol());
    env.push_back("SERVER_SOFTWARE=webserv/1.0");

    std::ostringstream len_oss;
    len_oss << request.getBody().size();
    env.push_back("CONTENT_LENGTH=" + len_oss.str());
    env.push_back("CONTENT_TYPE=" + request.getHeaderValue("Content-Type"));

    std::ostringstream port_oss;
    port_oss << client.getServerConfig().getPort();
    env.push_back("SERVER_PORT=" + port_oss.str());

    env.push_back("REDIRECT_STATUS=200");
    env.push_back("REQUEST_URI=" + request.getTarget());
    env.push_back("SCRIPT_FILENAME=" + script_path);

    // HTTP_* loop - fwd all headers except CL and Content-Type (cookies
    // included here)
    const std::map<std::string, std::string>& headers = request.getHeaders();
    std::map<std::string, std::string>::const_iterator it;
    for (it = headers.begin(); it != headers.end(); ++it) {
        const std::string& name = it->first;
        if (name == "content-type" || name == "content-length") {
            continue;
        }
        std::string key = "HTTP_";
        for (size_t i = 0; i < name.size(); ++i) {
            key += (name[i] == '-') ? '_' : toupper(name[i]);
        }
        env.push_back(key + "=" + it->second);
    }
    return env;
}

/**
 * @brief Converts environment strings into execve-compatible envp array.
 * Null-terminated array of char* pointers referencing input strings.
 *
 * @return envp array for execve.
 */
std::vector<char*> CgiSpawner::buildEnvp(
    const std::vector<std::string>& env_strings) {
    std::vector<char*> envp;

    for (size_t i = 0; i < env_strings.size(); ++i) {
        envp.push_back(const_cast<char*>(env_strings[i].c_str()));
    }
    envp.push_back(NULL);
    return envp;
}
