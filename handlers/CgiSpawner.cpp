#include "CgiSpawner.hpp"

#include <unistd.h>

CgiSpawner::CgiSpawner(EventLoop& loop) : loop_(loop) {}

CgiSpawner::~CgiSpawner() {}

/**
 */
bool CgiSpawner::spawn(HandlerContext& context)
{
	int stdin_pipe[2];
	int stdout_pipe[2];

    // create pipes
	if (!createPipes(stdin_pipe, stdout_pipe)) {
		return false;
	}
    // build script path
	std::string script_path = buildScriptPath(context);
    // resolve interpreter
	std::string interpreter = resolveInterpreter(context, script_path);
	if (interpreter.empty()) {
		return false;
	}
    // build env
	std::vector<std::string> env_strings = buildEnvStrings(context);
	std::vector<char*> envp = buildEnvp(env_strings);

    // fork
	pid_t pid = fork();
	if (pid < 0) {
		return false;
	}
	if (pid == 0)
	{
		// child
		dup2(stdin_pipe[0], STDIN_FILENO); // read
		dup2(stdout_pipe[1], STDOUT_FILENO); // write

		// close unused fds
		close(stdin_pipe[1]);
		close(stdout_pipe[0]);

		std::string dir = script_path.substr(0, script_path.rfind('/'));
		chdir(dir.c_str());

		char* argv[] = {
			const_cast<char*>(interpreter.c_str()),
			const_cast<char*>(script_path.c_str()),
			NULL
		};
		//  execve()
		execve(interpreter.c_str(), argv, envp.data());
		_exit(1);
	}
    // parent:
	close(stdin_pipe[0]); // parent doesn read stdin pipe
	close(stdout_pipe[1]); // parent doesnt write stdout pipe
						   //

	// write post body
	cosnt std::string& body = context.request.getBody();
	if (!body.empty()) {
		write(stdin_pipe[1], body.data(), body.size());
	}

    // create CgiProcess
	CgiProcess* cgi = new CgiProcess(pid, stdout_pipe[0], context.client,
		context.client.getLoop());
    // register in EventLoop
	context.client.getLoop().addHandler(cgi, POLLIN);
    return true;
}

// during parsing/validation cgi paths should already be correct -
// not empty and starting with '/'
// ex: goal input: /cgi-bin/upload.py: output /usr/bin/python3
std::string CgiSpawner::resolveInterpreter(HandlerContext& handler_context) {
	const std::string& uri = handler_context.request.getPath();

	//find extension from requested script
	size_t dot_pos = uri.rfind('.');
	if (dot_pos == std::string::npos) {
		return "";
	}

	std::string extension = uri.substr(dot_pos);

	// determine interpreter from extension
	const std::map<std::string, std::string>& cgi_map =
		handler_context.location.getCgiInterpreters();

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
 */
std::string CgiSpawner::buildScriptPath(const HandlerContext& context) {
	std::string uri = context.request.getPath();
	const std::string& root = context.location.getRoot();
	const std::string& uri = context.request.getPath();

	if (root.empty()) {
		LOG_ERROR() << "[CgiSpawner] missing root";
		throw std::runtime_error("[CGI] missing root");
	}
	if (uri.empty() || uri[0] != '/') {
		LOG_ERROR() << "[CgiSpawner] invalid request uri";
		throw std::runtime_error("[CGI] invalid request uri");
	}
	if (!root.empty() && root[root.size() - 1] == '/') {
		return root + uri.substr(1);
	}
	return root + uri;
}

std::vector<std::string> CgiSpawner::buildEnvStrings(
		const HandlerContext& context) {
	std::vector<std::string> env;
	const Request& request = context.request;

	env.push_back("REQUEST_METHOD=" + request.getMethod());
	env.push_back("SCRIPT_NAME=" + request.getPath());
	env.push_back("QUERY_STRING=" + request.getQuery());
	env.push_back("REQUEST_URI=" + request.getTarget());
	env.push_back("SERVER_PROTOCOL=" + request.getProtocol());

	std::ostringstream oss;
	oss << request.getBody().size();

	env.push_back("CONTENT_LENGTH=" + oss.str());
	env.push_back("CONTENT_TYPE=" + request.getHeaderValue("Content-Type"));

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
std::vector<char*> CgiSpawner::buildEnvp(const::vector<std::string>& env_strings) {
	std::vector<char*> envp;

	for (size_t i = 0; i < env_strings.size(); ++i) {
		envp.push_back(const_cast<char*>(env_strings[i].c_str()));
	}
	envp.push_back(NULL);
	return envp;
}
