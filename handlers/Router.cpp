#include "Router.hpp"

Router::Router(const Config& config) : config_(config) {}

/**
 * @brief Stub: returns first available location config.
 * @note Will be replaced by proper path matching logic.
 */
const LocationConfig& Router::resolve(const Request& request) const {
    (void)request;
    // stub: just return first location of first server block
    return config_.getServerBlock()[0].getLocationBlock()[0];
}
