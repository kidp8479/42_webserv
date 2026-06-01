#include "Server.hpp"

#include "../config/ServerConfig.hpp"
#include "../logger/Logger.hpp"
#include "EventLoop.hpp"
#include "ServerResources.hpp"
#include "Signal.hpp"
#include "Timeout.hpp"

Server::Server(const Config& config) : config_(config), loop_() {
    setupListeners();
}

Server::~Server() {
    // EventLoop owns and deletes all handlers including Listeners
    // just clear our tracking vector without deleting
    listeners_.clear();
}

bool Server::start() {
    LOG_INFO() << BR_CYN "[Server] starting..." RESET;
    while (g_running) {
        int ready = loop_.wait(TimeoutMs::kPollHeartbeat);
        if (!g_running) {
            break;
        }
        if (ready < 0) {
            LOG_ERROR() << "[Server] Event loop wait failed";
            return false;
        }
        loop_.dispatch();
        loop_.cleanup();
    }
    return true;
}

void Server::setupListeners() {
    const std::vector<ServerConfig>& servers = config_.getServerBlock();

    for (size_t i = 0; i < servers.size(); i++) {
        ServerResources resources(servers[i]);

        const ServerConfig& cfg = resources.getServerConfig();
        LOG_INFO() << BR_CYN "[Server] starting listener on " << cfg.getHost()
                   << ":" << cfg.getPort() << RESET;
        Listener* listener = new Listener(loop_, resources);
        listeners_.push_back(listener);
    }
}
