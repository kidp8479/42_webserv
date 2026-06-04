#include "Server.hpp"

#include "../config/ServerConfig.hpp"
#include "../logger/Logger.hpp"
#include "EventLoop.hpp"
#include "ServerResources.hpp"
#include "Signal.hpp"
#include "Timeout.hpp"

/**
 * @brief Constructs the server and initializes listeners.
 * Builds an EventLoop and creates a Listener per configured
 * server block.
 */
Server::Server(const Config& config) : config_(config), loop_() {
    setupListeners();
}

/**
 * @brief Destroys the server.
 * Listener lifetime is managed by the EventLoop; the local
 * tracking vector is only cleared.
 */
Server::~Server() {
    listeners_.clear();
}

/**
 * @brief Runs the main event loop.
 * Continuously polls for events, dispatches handlers, and
 * performs cleanup until the global shutdown flag is set.
 *
 * @return true if shutdown completed cleanly, false on error.
 */
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

/**
 * @brief Creates a listener for each server configuration.
 * Each ServerConfig is wrapped into ServerResources and used
 * to initialize a Listener registered in the EventLoop.
 * Each listener is immediately owned by the EventLoop.
 */
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
