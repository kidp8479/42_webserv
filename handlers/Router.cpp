#include "Router.hpp"

#include "../logger/Logger.hpp"

Router::Router(const ServerConfig& server_config)
    : server_config_(server_config) {
}

Router::~Router() {
}

/**
 * @brief Logs an error and throws a std::runtime_error with a "[Router] "
 * prefix.
 *
 * @param msg The error message (without the "[Router] " prefix)
 * @throws std::runtime_error always
 */
void Router::routerError(const std::string& msg) const {
    std::string full = "[Router] " + msg;
    LOG_ERROR() << full;
    throw std::runtime_error(full);
}

/**
 * @brief Resolves a URI to the best matching LocationConfig using
 * longest-prefix match.
 *
 * Iterates all location blocks and selects the one whose path is the longest
 * prefix of the URI. Throws if no location matches.
 *
 * @param uri The request URI path (ex: "/api/users/123")
 * @return Const reference to the matching LocationConfig
 * @throws std::runtime_error if no location prefix matches the URI
 */
const LocationConfig& Router::resolve(const std::string& uri) const {
    LOG_DEBUG() << BR_YEL "[Router] resolving request" << RESET;
    LOG_DEBUG() << "[Router] resolving URI: " << GRN << uri << RESET;

    const std::vector<LocationConfig>& location_blocks =
        server_config_.getLocationBlock();
    std::vector<LocationConfig>::const_iterator it;

    // running best pattern, update only when better match is found
    const LocationConfig* best_path_match = NULL;
    size_t best_path_len = 0;

    for (it = location_blocks.begin(); it != location_blocks.end(); it++) {
        // uri.compare(0, path.size(), path) checks if path is a prefix of uri.
        // strip trailing slash from location path before comparison (except
        // "/"), so "/api/users/" and "/api/users" both match URI
        // "/api/users/123"
        std::string path = it->getPath();
        if (path.size() > 1 && path[path.size() - 1] == '/') {
            path = path.substr(0, path.size() - 1);
        }

        if (uri.compare(0, path.size(), path) == 0) {
            // guard against false positives: /api must not match /apiary
            // valid match only if: exact match, next char is '/', or path is
            // "/" (root always matches - every URI starts with /)
            if (path == "/" || uri.size() == path.size() ||
                uri[path.size()] == '/') {
                if (path.size() > best_path_len) {
                    best_path_len = path.size();
                    best_path_match = &(*it);
                }
            }
        }
    }

    if (best_path_match == NULL) {
        routerError("No matching location for URI: " + uri);
    }

    LOG_INFO() << BR_CYN "[Router] resolved " << uri << " => "
               << best_path_match->getPath() << RESET;

    return (*best_path_match);
}
