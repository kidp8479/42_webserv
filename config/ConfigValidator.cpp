#include "ConfigValidator.hpp"

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

// high level validation, serverChecks will validate every server block
// location block validation is triggered inside the serverChecks, we go to
// high level checks to the lowest levels checks
// checkDuplicateHostPort needs to compare between all valid server blocks,
// hence the high level
void ConfigValidator::validate(const Config& config) const {
    serverChecks(config);
    checkDuplicateHostPort(config);
}

void ConfigValidator::serverChecks(const Config& config) const {
    const std::vector<ServerConfig>& server_block = config.getServerBlock();
    std::vector<ServerConfig>::const_iterator it;

    for (it = server_block.begin(); it != server_block.end(); ++it) {
        LOG_DEBUG() << "one server block found";

        checkPort(*it);
        checkHost(*it);
        checkServerErrorCodes(*it);
        checkDuplicatePath(*it);

        const std::vector<LocationConfig>& location_block =
            it->getLocationBlock();
        (void)location_block;
        // locationChecks(*it);
    }
}

void ConfigValidator::checkDuplicateHostPort(const Config& server) const {
    (void)server;
}

void ConfigValidator::checkPort(const ServerConfig& server) const {
    (void)server;
}

void ConfigValidator::checkHost(const ServerConfig& server) const {
    (void)server;
}

void ConfigValidator::checkServerErrorCodes(const ServerConfig& server) const {
    (void)server;
}

void ConfigValidator::checkDuplicatePath(const ServerConfig& server) const {
    (void)server;
}

void ConfigValidator::locationChecks(const ServerConfig& server) const {
    (void)server;
}

void ConfigValidator::checkPath(const LocationConfig& location) const {
    (void)location;
}

void ConfigValidator::checkReturnCode(const LocationConfig& location) const {
    (void)location;
}

void ConfigValidator::checkUrl(const LocationConfig& location) const {
    (void)location;
}
