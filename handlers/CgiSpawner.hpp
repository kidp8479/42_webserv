#ifndef CGISPAWNER_HPP
#define CGISPAWNER_HPP

#include <vector>
#include "../core/EventLoop.hpp"
#include "Handler.hpp"
#include <string>


class CgiSpawner {
public:
	explicit CgiSpawner(EventLoop& loop);
	~CgiSpawner();

	bool spawn(const Request& request, const LocationConfig& location,
			Client& client);

private:
	CgiSpawner(const CgiSpawner&);
	CgiSpawner& operator=(const CgiSpawner&);

	bool createPipes(int stdin_pipe[2], int stdout_pipe[2]);
	std::string resolveInterpreter(const Request& request,
			const LocationConfig& location);
	std::string buildScriptPath	(const Request& request,
			const LocationConfig& config);
	std::vector<std::string> buildEnvStrings(const Request& request);
	std::vector<char*> buildEnvp(const std::vector<std::string>& env_strings);

	EventLoop& loop_;
};

#endif
