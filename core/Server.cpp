#include "Server.hpp"
#include "../config/ServerConfig.hpp"
#include "../logger/Logger.hpp"
#include "EventLoop.hpp"
#include "Signal.hpp"

Server::Server(const Config& config) :
	config_(config),
	resources_(config_),
	loop_()
{
	//EventLoop loop_ constructed automatically
	//vector listeners_ constructed automatically
	setupListeners();
}

Server::~Server() {
	// EventLoop owns and deletes all handlers including Listeners
    // just clear our tracking vector without deleting
    listeners_.clear();
}

bool Server::start() {
    LOG_INFO() << "Server starting...";
	while (g_running) {
		// sleeps until something happens
		int ready = loop_.wait(-1);
		if (!g_running) {
			break;
		}
		if (ready <= 0)
            continue;
		loop_.dispatch();
		// remove dead handlers
		loop_.cleanup();
	}
	return true;
}

void Server::setupListeners() {
	const std::vector<ServerConfig>& servers = config_.getServerBlock();

	for (size_t i = 0; i < servers.size(); i++) {
		int port = servers[i].getPort();

		Listener* listener = new Listener(port, loop_, resources_);
		listeners_.push_back(listener);
    }
}
