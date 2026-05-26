#include "CgiSpawner.hpp"

#include <unistd.h>

CgiSpawner::CgiSpawner(EventLoop& loop) : loop_(loop) {}

CgiSpawner::~CgiSpawner() {}

bool CgiSpawner::spawn(HandlerContext& context)
{
    // resolve interpreter
    // build script path
    // create pipes
    // build env
    // fork

    // child:
    //      dup2()
    //      execve()

    // parent:
    //      create CgiProcess
    //      register in EventLoop

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
	if (pipe(stdin_pipe) < -1) {
		LOG_ERROR() << "[CgiSpawner] failed to create stdin pipe";
		return false;
	}
	// stdout_pipe[0] = read end parent
	// stdout_pipe[1] = write end child
	if (pipe(stdout_pipe) < -1) {
		LOG_ERROR() << "[CgiSpawner] failed to create stdout pipe";
		close(std_inpipe[0]);
		close(stdin_pipe[1]);
		return false;
	}
}

/** turn root + uri into a valid filesystem path
 * some edge cases to consider
 * /var/www + /cgi-bin/a.py
 * /var/www/ + /cgi-bin/a.py
 * /var/www + cgi-bin/a.py
 */
std::string CgiSpawner::buildScriptPath(const HandlerContext& context) {
	std::string uri = context.request.getPath();
	const std::string& root = context.location.getRoot();

	if (root.empty()) {
		LOG_ERROR() << "[CgiSpawner] missing root";
		throw std::runtime_error("[CGI] missing root");
	}
	if (uri.empty() || uri[0] != '/') {
		LOG_ERROR() << "[CgiSpawner] invalid request uri";
		throw std::runtime_error("[CGI] invalid request uri");
	}
	std::string path = root;

	// ensure exactly one '/'
	if (!path.empty() && path[path.size() - 1] != '/' && uri[0] != '/') {
		path += '/';
	}
	// handle root="/" or root ending in '/'
	if (!path.empty() && path[path.size() - 1] != '/' && uri[0] == '/') {
		path += uri.substr(1);
	} else {
		path += uri;
	}
	return path;
}


