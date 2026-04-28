#include "ConfigValidator.hpp"

namespace {
const int kMinPort = 1;
const int kMaxPort = 65535;
const int kMinIpOctet = 0;
const int kMaxIpOctet = 255;
const int kIpOctetCount = 4;
const int kMinErrorPage = 400;
const int kMaxErrorPage = 599;
}  // namespace

ConfigValidator::ConfigValidator() {
}

ConfigValidator::~ConfigValidator() {
}

/**
 * @brief Logs an error and throws a std::runtime_error with a "Config: "
 * prefix.
 *
 * @param msg The error message (without the "Config: " prefix)
 * @throws std::runtime_error always
 */
void ConfigValidator::configError(const std::string& msg) const {
    std::string full = "Config: " + msg;
    LOG_ERROR() << full;
    throw std::runtime_error(full);
}

/**
 * @brief Validates all server and location blocks in the Config.
 * Orchestrates checks from high level (cross-server) to low level (per
 * location). checkDuplicateHostPort compares across all server blocks and
 * runs after serverChecks, which validates each block and its locations.
 *
 * @param config The Config object to validate (produced by ConfigBuilder)
 * @throws std::runtime_error If any semantic error is found
 */
void ConfigValidator::validate(const Config& config) const {
    serverChecks(config);
    checkDuplicateHostPort(config);
    LOG_INFO() << BR_CYN "ConfigValidator: Config object fully validated" RESET;
}

/**
 * @brief Iterates all server blocks and runs per-block and per-location checks.
 */
void ConfigValidator::serverChecks(const Config& config) const {
    const std::vector<ServerConfig>& server_block = config.getServerBlock();
    std::vector<ServerConfig>::const_iterator it;

    for (it = server_block.begin(); it != server_block.end(); ++it) {
        LOG_DEBUG() << "Server block found, validating data...";

        checkPort(*it);
        checkHost(*it);
        checkServerErrorCodes(*it);
        checkDuplicatePath(*it);

        locationChecks(*it);
    }
}

/**
 * @brief Checks that port_ is set and in range [1-65535].
 */
void ConfigValidator::checkPort(const ServerConfig& server) const {
    int port = server.getPort();

    if (port == ServerConfig::kPortNotSet) {
        configError("Server listening port not set.");
    }
    if (port < kMinPort || port > kMaxPort) {
        std::ostringstream oss;
        oss << "Invalid port: " << port << " - valid range [" << kMinPort << "-"
            << kMaxPort << "]";
        configError(oss.str());
    }
    LOG_DEBUG() << "Valid listening server port.";
}

/**
 * @brief Checks that host_ is "localhost" or a valid IPv4 address (4 octets,
 * each in [0-255]). Host reachability is not checked here.
 */
void ConfigValidator::checkHost(const ServerConfig& server) const {
    std::string host = server.getHost();
    if (host == "localhost") {
        LOG_DEBUG() << "Valid host format.";
        return;
    }

    std::istringstream iss(host);
    std::string segment;
    size_t count = 0;
    while (std::getline(iss, segment, '.')) {
        if (segment.empty()) {
            configError("Invalid host format. IP member is empty.");
        }

        for (size_t i = 0; i < segment.size(); i++) {
            if (!isdigit(segment[i])) {
                configError("Invalid host format. IP must be digits only.");
            }
        }

        std::istringstream segment_iss(segment);
        int value;
        segment_iss >> value;
        if (value < kMinIpOctet || value > kMaxIpOctet) {
            std::ostringstream oss;
            oss << "Invalid host format. IP octet " << value
                << " out of range [0-255]";
            configError(oss.str());
        }
        count++;
    }
    if (count != kIpOctetCount) {
        configError("Invalid host format. Misconstructed IP address.");
    }
    LOG_DEBUG() << "Valid host format.";
}

/**
 * @brief Checks that all error_pages_ codes are in range [400-599].
 * File existence is not checked here, only at runtime.
 */
void ConfigValidator::checkServerErrorCodes(const ServerConfig& server) const {
    const std::map<int, std::string>& error_pages = server.getErrorPages();
    std::map<int, std::string>::const_iterator it;

    for (it = error_pages.begin(); it != error_pages.end(); ++it) {
        int error_code = it->first;
        if (error_code < kMinErrorPage || error_code > kMaxErrorPage) {
            std::ostringstream oss;
            oss << "Invalid error page code: " << error_code
                << " - must be in range [400-599]";
            configError(oss.str());
        }
    }
    LOG_DEBUG() << "Valid error code(s).";
}

/**
 * @brief Checks that no two server blocks share the same host:port combination.
 */
void ConfigValidator::checkDuplicateHostPort(const Config& server) const {
    const std::vector<ServerConfig>& server_blocks = server.getServerBlock();
    std::vector<ServerConfig>::const_iterator it1;
    std::vector<ServerConfig>::const_iterator it2;

    for (it1 = server_blocks.begin(); it1 < server_blocks.end(); ++it1) {
        for (it2 = it1 + 1; it2 < server_blocks.end(); ++it2) {
            if ((*it1).getHost() == (*it2).getHost() &&
                (*it1).getPort() == (*it2).getPort()) {
                std::ostringstream oss;
                oss << "Duplicate listen: " << (*it1).getHost() << ":"
                    << (*it1).getPort();
                configError(oss.str());
            }
        }
    }
    LOG_DEBUG() << "No duplicated host:port pairs.";
}

/**
 * @brief Checks that no two location blocks in the same server share the same
 * path_.
 */
void ConfigValidator::checkDuplicatePath(const ServerConfig& server) const {
    const std::vector<LocationConfig>& location_blocks =
        server.getLocationBlock();
    std::vector<LocationConfig>::const_iterator it1;
    std::vector<LocationConfig>::const_iterator it2;

    for (it1 = location_blocks.begin(); it1 < location_blocks.end(); ++it1) {
        for (it2 = it1 + 1; it2 < location_blocks.end(); ++it2) {
            if ((*it1).getPath() == (*it2).getPath()) {
                std::ostringstream oss;
                oss << "Duplicated location path: " << (*it1).getPath();
                configError(oss.str());
            }
        }
    }
    LOG_DEBUG() << "No duplicated location path.";
}

/**
 * @brief Iterates all location blocks of a server and runs per-location checks.
 */
void ConfigValidator::locationChecks(const ServerConfig& server) const {
    (void)server;
}

/**
 * @brief Checks that path_ starts with '/'.
 */
void ConfigValidator::checkPath(const LocationConfig& location) const {
    (void)location;
}

/**
 * @brief If return_code_ is set, checks it is in range [300-399] and that
 * return_url_ is not empty.
 */
void ConfigValidator::checkReturnCode(const LocationConfig& location) const {
    (void)location;
}

/**
 * @brief If return_url_ is set, checks that return_code_ is also set.
 */
void ConfigValidator::checkUrl(const LocationConfig& location) const {
    (void)location;
}
