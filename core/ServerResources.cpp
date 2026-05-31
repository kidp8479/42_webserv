#include "ServerResources.hpp"

ServerResources::ServerResources(const ServerConfig& server_config)
    : server_config_(server_config), router_(server_config_) {
}

ServerResources::ServerResources(const ServerResources& other)
    : server_config_(other.server_config_),
      router_(server_config_),
      sessions_(other.sessions_) {
}

ServerResources::~ServerResources() {
}

const Router& ServerResources::getRouter() const {
    return router_;
}

const ServerConfig& ServerResources::getServerConfig() const {
    return server_config_;
}

/**
 * @brief Creates an empty session entry for the given session ID.
 * @param id Session ID to register. Does nothing if empty.
 */
void ServerResources::createSession(const std::string& id) {
    if (id.empty()) {
        LOG_WARNING() << "[ServerResources] createSession called with empty ID";
        return;
    }

    // if we have a valid ID, create an empty entry in the map, associated with
    // the id map content will be populated by the handler
    sessions_[id] =
        std::map<std::string, std::string>();  // explicitly initializes the
                                               // session data map to empty
    LOG_INFO() << "[ServerResources] cookie session created: " << id;
    return;
}

/**
 * @brief Checks whether a session with the given ID exists.
 * @param id Session ID to look up.
 * @return true if the session exists, false otherwise.
 */
bool ServerResources::hasSession(const std::string& id) const {
    // count() returns 1 if the key exists, 0 otherwise
    if (sessions_.count(id) > 0) {
        LOG_DEBUG() << "[ServerResources] cookie session found: " << id;
        return true;
    }
    LOG_DEBUG() << "[ServerResources] cookie session not found: " << id;
    return false;
}

/**
 * @brief Returns a reference to the session data map for the given ID.
 * @param id Session ID to retrieve. Creates an empty entry if not found.
 * @return Reference to the session data map (read/write).
 */
std::map<std::string, std::string>& ServerResources::getSession(
    const std::string& id) {
    return sessions_[id];
}