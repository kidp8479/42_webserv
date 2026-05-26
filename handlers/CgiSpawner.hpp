#ifndef CGISPAWNER_HPP
#define CGISPAWNER_HPP

#include //pipes
#include <vector>
#include "../core/EventLoop.hpp"
#include "Handler.hpp"
#include <string>


class CgiSpawner {
public:
	explicit CgiSpawner(EventLoop& loop);
	~CgiSpawner();

	bool spawn(HandlerContext& context);

private:
	CgiSpawner(const CgiSpawner&);
	CgiSpanwer& operator=(const CgiSpawner&);

	EventLoop& loop_;

	bool createPipes(int stdin_pipe[2], int stdout_pipe[2]);
	
	std::string resolveInterpreter(const HandlerContext& context,
			const std::string& script_path);
	
	std::vector<char*> buildEnv(HandlerContext& ctx,
			const::vector<std::string>& env);
};

#endif
