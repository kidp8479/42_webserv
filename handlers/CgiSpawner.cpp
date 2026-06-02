#include "CgiSpawner.hpp"

#include <unistd.h>

#include "../core/CgiProcess.hpp"
#include "../core/CgiStdinWriter.hpp"
#include "../core/Client.hpp"
#include "../core/FdUtils.hpp"

CgiSpawner::CgiSpawner(EventLoop& loop) : loop_(loop) {
}

CgiSpawner::~CgiSpawner() {
}

/**
 */
bool CgiSpawner::spawn(const Request& request, const LocationConfig& location,
                       Client& client) {
        // build script path
    std::string script_path = buildScriptPath(request, location);
	if (script_path.empty()) {
		return false;
	}
    // resolve interpreter
    std::string interpreter = resolveInterpreter(request, location);
    if (interpreter.empty()) {
        return false;
    }
    // build env
    std::vector<std::string> env_strings = buildEnvStrings(request);
    std::vector<char*> envp = buildEnvp(env_strings);
	// create pipes after validating everything so i dont have to close pipes
	// if there is a failure
	int stdin_pipe[2];
    int stdout_pipe[2];

    // create pipes
    if (!createPipes(stdin_pipe, stdout_pipe)) {
        return false;
    }

    // fork
    pid_t pid = fork();
    if (pid < 0) {
        return false;
    }
    if (pid == 0) {
        // child
        dup2(stdin_pipe[0], STDIN_FILENO);    // read
        dup2(stdout_pipe[1], STDOUT_FILENO);  // write

        // close unused fds
        close(stdin_pipe[0]);
        close(stdin_pipe[1]);
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);

        std::string dir = script_path.substr(0, script_path.rfind('/'));
        std::string filename = script_path.substr(script_path.rfind('/') + 1);

        chdir(dir.c_str());

        char* argv[] = {const_cast<char*>(interpreter.c_str()),
                        const_cast<char*>(filename.c_str()), NULL};
        //  execve()
        execve(interpreter.c_str(), argv, &envp[0]);
        _exit(1);
    }
    // parent:
    close(stdin_pipe[0]);   // parent doesn read stdin pipe
    close(stdout_pipe[1]);  // parent doesnt write stdout pipe

	// set non blocking flag on read end before we hand to CgiProcess
	FdUtils::setNonBlocking(stdout_pipe[0]);

    const std::string& body = request.getBody();
    // write post body
    if (!body.empty()) {
        CgiStdinWriter* writer = new CgiStdinWriter(stdin_pipe[1], body, loop_);
        loop_.addHandler(writer, POLLOUT);
    } else {
        close(stdin_pipe[1]);
    }
    // create Cgi Handler
    CgiProcess* cgi = new CgiProcess(pid, stdout_pipe[0], client, loop_);
    // register with Client
    client.setPendingCgi(cgi);
    // register in EventLoop
    loop_.addHandler(cgi, POLLIN);
    return true;
}

// during parsing/validation cgi paths should already be correct -
// not empty and starting with '/'
// ex: goal input: /cgi-bin/upload.py: output /usr/bin/python3
std::string CgiSpawner::resolveInterpreter(const Request& request,
                                           const LocationConfig& location) {
    const std::string& uri = request.getPath();

    // find extension from requested script
    size_t dot_pos = uri.rfind('.');
    if (dot_pos == std::string::npos) {
        return "";
    }

    std::string extension = uri.substr(dot_pos);

    // determine interpreter from extension
    const std::map<std::string, std::string>& cgi_map =
        location.getCgiInterpreters();

    std::map<std::string, std::string>::const_iterator it =
        cgi_map.find(extension);

    // no mathc found, return empty, well check against this later to
    // return error
    if (it == cgi_map.end()) {
        return "";
    }
    // return interpreter path (/usr/bin/python3 or /usr/bin/php-cgi)
    return it->second;
}

// stdin pipe: send POST body to CGI
// stdout pipe: receive CGI output
//
bool CgiSpawner::createPipes(int stdin_pipe[2], int stdout_pipe[2]) {
    // create the pipes
    // stdin_pipe[0] = read end of child
    // stdin_pipe[1] = write end parent
    if (pipe(stdin_pipe) == -1) {
        LOG_ERROR() << "[CgiSpawner] failed to create stdin pipe";
        return false;
    }
    // stdout_pipe[0] = read end parent
    // stdout_pipe[1] = write end child
    if (pipe(stdout_pipe) == -1) {
        LOG_ERROR() << "[CgiSpawner] failed to create stdout pipe";
        close(stdin_pipe[0]);
        close(stdin_pipe[1]);
        return false;
    }
    return true;
}

/**
 * turn root + uri into a valid filesystem path
 * example of a cgi request
 * GET /cgi-bin/hello.py HTTP/1.1
 * Host: location:8086
 * after GET it's followed by SPACE and then TARGET beginning with /
 */
std::string CgiSpawner::buildScriptPath(const Request& request,
                                        const LocationConfig& location) {
    const std::string& root = location.getRoot();
    const std::string& uri = request.getPath();
    const std::string& prefix = location.getPath();

    if (root.empty()) {
        LOG_ERROR() << "[CgiSpawner] missing root";
        // is root empty checked at validation?
        throw std::runtime_error("[CGI] missing root");
    }
    // TARGET must begin with '/'
    if (uri.empty() || uri[0] != '/') {
        LOG_ERROR() << "[CgiSpawner] invalid request uri";
        throw std::runtime_error("[CGI] invalid request uri");
    }
    std::string relative_uri = uri;
    // strip location prefix
    if (uri.compare(0, prefix.size(), prefix) == 0) {
        relative_uri = uri.substr(prefix.size());
    }
    // ensure leading '/'
    if (relative_uri.empty() || relative_uri[0] != '/') {
        relative_uri = "/" + relative_uri;
    }
    // avoid double '/'
    if (!root.empty() && root[root.size() - 1] == '/') {
        return root.substr(0, root.size() - 1) + relative_uri;
    }

    return root + relative_uri;
}

std::vector<std::string> CgiSpawner::buildEnvStrings(const Request& request) {
    std::vector<std::string> env;

    env.push_back("REQUEST_METHOD=" + request.getMethod());
    env.push_back("SCRIPT_NAME=" + request.getPath());
    env.push_back("QUERY_STRING=" + request.getQuery());
    env.push_back("REQUEST_URI=" + request.getTarget());
    env.push_back("SERVER_PROTOCOL=" + request.getProtocol());

    std::ostringstream oss;
    oss << request.getBody().size();

    env.push_back("CONTENT_LENGTH=" + oss.str());
    env.push_back("CONTENT_TYPE=" + request.getHeaderValue("Content-Type"));
    // pass cookies to CGI via HTTP_COOKIE — required for session-aware scripts
    env.push_back("HTTP_COOKIE=" + request.getHeaderValue("Cookie"));

	env.push_back("GATEWAY_INTERFACE=CGI/1.1");
	env.push_back("SERVER_NAME=" + request.getHeaderValue("Host"));
	env.push_back("HTTP_HOST=" + request.getHeaderValue("Host"));
	env.push_back("HTTP_ACCEPT=" + request.getHeaderValue("Accept"));
	env.push_back("HTTP_USER_AGENT=" + request.getHeaderValue("User-Agent"));
    return env;
}

/**
 * example upload request:
 * POST /cgi-bin/upload.py HTTP/1.1
 * Content-Type: multipart/form-data; boundary=abc
 * Content-Length: 5120
 * Build environment that will be passed to execve() so CGI script can
 * learn about the HTTP request
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
