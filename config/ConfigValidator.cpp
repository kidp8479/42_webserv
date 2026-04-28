#include "ConfigValidator.hpp"

namespace {
const int kMinPort = 1;
const int kMaxPort = 65535;
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
        LOG_DEBUG() << "server block found, validating data...";

        checkPort(*it);
        checkHost(*it);
        checkServerErrorCodes(*it);
        checkDuplicatePath(*it);

        locationChecks(*it);
    }
}

void ConfigValidator::checkDuplicateHostPort(const Config& server) const {
    (void)server;
}

void ConfigValidator::checkPort(const ServerConfig& server) const {
    int port = server.getPort();

    if (port == ServerConfig::kPortNotSet) {
        configError("server listening port not set.");
    }
    if (port < kMinPort || port > kMaxPort) {
        std::ostringstream oss;
        oss << "invalid port: " << port << " - valid range [" << kMinPort << "-"
            << kMaxPort << "]";
        configError(oss.str());
    }
    LOG_DEBUG() << "valid listening server port.";
}

void ConfigValidator::checkHost(const ServerConfig& server) const {
    std::string host = server.getHost();
    if (host == "localhost") {
        return;
    }

    std::istringstream iss(host);
    std::string segment;
    size_t count = 0;
    while (std::getline(iss, segment, '.')) {
        if (segment.empty()) {
            configError("invalid host format. IP member is empty.");
        }

        for (size_t i = 0; i < segment.size(); i++) {
            if (!isdigit(segment[i])) {
                configError("invalid host format. IP must be digits only.");
            }
        }

        std::istringstream segment_iss(segment);
        int value;
        segment_iss >> value;
        if (value < 0 || value > 255) {
            configError(
                "invalid host format. IP member must be in range [0-255]");
        }
        count++;
    }
    if (count != 4) {
        configError("invalid host format. Misconstructed IP address.");
    }
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
