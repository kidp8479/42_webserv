#include "ServerResources.hpp"

/**
 * @brief Initializes server-scoped resources.
 * Builds routing configuration from the provided ServerConfig.
 */
ServerResources::ServerResources(const ServerConfig& server_config)
    : server_config_(server_config), router_(server_config_) {
}

/**
 * @brief Copy-constructs server resources.
 * Copies session data while rebuilding router from the same config.
 *
 * @note Router is not shallow-copied; it is re-initialized.
 */
ServerResources::ServerResources(const ServerResources& other)
    : server_config_(other.server_config_),
      router_(server_config_),
      sessions_(other.sessions_) {
}

/**
 * @brief Destroys server resources.
 */
ServerResources::~ServerResources() {
}

/**
 * @brief Retrieves the request router.
 * @return Const reference to the router.
 */
const Router& ServerResources::getRouter() const {
    return router_;
}

/**
 * @brief Retrieves the server configuration.
 * @return Const reference to ServerConfig.
 */
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
    LOG_INFO() << BR_CYN "[ServerResources] cookie session created: " << id
               << RESET;
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
        LOG_DEBUG() << BR_YEL "[ServerResources] cookie session found: " << id
                    << RESET;
        return true;
    }
    LOG_DEBUG() << BR_YEL "[ServerResources] cookie session not found: " << id
                << RESET;
    return false;
}

/**
 * @brief Returns a reference to the cookie session data map for the given ID.
 * @param id Session ID to retrieve.
 * @return Reference to the session data map (read/write).
 * @note operator[] inserts an empty entry if id is absent - hence the name
 *       getOrCreateSession. Always call hasSession() first if you only want
 *       to read an existing session without creating a phantom one.
 */
std::map<std::string, std::string>& ServerResources::getOrCreateSession(
    const std::string& id) {
    return sessions_[id];
}
