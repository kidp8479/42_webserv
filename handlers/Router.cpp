#include "Router.hpp"

Router::Router(const ServerConfig& server_config)
    : server_config_(server_config) {
}

/**
 * @brief Stub: returns first available location config.
 * @note Will be replaced by proper path matching logic.
 */
const LocationConfig& Router::resolve(const Request& request) const {
    (void)request;
    // stub: just return first location of first server block
    return server_config_.getLocationBlock()[0];
}
