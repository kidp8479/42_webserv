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

	bool spawn(HandlerContext& context);

private:
	CgiSpawner(const CgiSpawner&);
	CgiSpawner& operator=(const CgiSpawner&);

	EventLoop& loop_;

	bool createPipes(int stdin_pipe[2], int stdout_pipe[2]);
	
	std::string resolveInterpreter(const HandlerContext& context,
			const std::string& script_path);
	std::string buildScriptPath	(const HandlerContext& context);
	std::vector<char*> buildEnv(HandlerContext& context,
			const::vector<std::string>& env);
};

#endif
