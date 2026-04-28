#ifndef CONFIG_VALIDATOR_HPP
#define CONFIG_VALIDATOR_HPP

#include <sstream>

#include "../logger/Logger.hpp"
#include "Config.hpp"

class ConfigValidator {
public:
    ConfigValidator();
    ~ConfigValidator();

    void validate(const Config& config) const;

private:
    ConfigValidator(const ConfigValidator& copy);
    ConfigValidator& operator=(const ConfigValidator& other);

    void configError(const std::string& msg) const;

    void serverChecks(const Config& config) const;
    void checkPort(const ServerConfig& server) const;
    void checkHost(const ServerConfig& server) const;
    void checkDuplicateHostPort(const Config& server) const;
    void checkServerErrorCodes(const ServerConfig& server) const;

    void locationChecks(const ServerConfig& server) const;
    void checkPath(const LocationConfig& location) const;
    void checkDuplicatePath(const ServerConfig& server) const;
    void checkReturnCode(const LocationConfig& location) const;
    void checkUrl(const LocationConfig& location) const;
};

#endif