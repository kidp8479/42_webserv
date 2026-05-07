#ifndef ROUTER_HPP
#define ROUTER_HPP

#include "../config/LocationConfig.hpp"
#include "../config/ServerConfig.hpp"

/**
 * @brief Resolves an HTTP request URI to the matching LocationConfig.
 *
 * Implements longest-prefix match against the location blocks of a
 * ServerConfig. Given a URI, iterates all locations and returns the one
 * whose path is the longest prefix of the URI. Throws if no location matches.
 *
 * @note No regex support — prefix matching only, per subject requirements.
 */
class Router {
public:
    Router(const ServerConfig& server_config);
    ~Router();

    const LocationConfig& resolve(const std::string& uri) const;

private:
    void routerError(const std::string& msg) const;

private:
    Router(const Router& copy);
    Router& operator=(const Router& other);

    const ServerConfig& server_config_;
};
#endif
