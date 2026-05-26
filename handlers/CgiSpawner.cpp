#include "CgiSpawner.hpp"

#include <unistd.h>

CgiSpawner::CgiSpawner(EventLoop& loop) : loop_(loop) {}

CgiSpawner::~CgiSpawner() {}

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

	// if none exist return empty, well check against this later to
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
	if (pipe(stdin_pipe) < 0) {
		return false;
	}
	// stdout_pipe[0] = read end parent
	// stdout_pipe[1] = write end child
	if (pipe(stdout_pipe) < 0) {
		close(std_inpipe[0]);
		close(stdin_pipe[1]);
		return false;
	}
}
