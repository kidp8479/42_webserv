#include "Router.hpp"

Router::Router(const ServerConfig& config_server) : config_server_(config_server) {}

/**
 * @brief Stub: returns first available location config.
 * @note Will be replaced by proper path matching logic.
 */
const LocationConfig& Router::resolve(const Request& request) const {
    (void)request;
    // stub: just return first location of first server block
    return config_server_.getLocationBlock()[0];
}
