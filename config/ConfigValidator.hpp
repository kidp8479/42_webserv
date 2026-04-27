#ifndef CONFIG_VALIDATOR_HPP
#define CONFIG_VALIDATOR_HPP

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

    void serverChecks(const Config& server_block) const;
    void checkPort(const ServerConfig& server_block) const;
    void checkHost(const ServerConfig& server_block) const;
    void checkDuplicateHostPort(const Config& server_block) const;
    void checkServerErrorCodes(const ServerConfig& server_block) const;

    void locationChecks(const ServerConfig& location_block) const;
    void checkPath(const LocationConfig& location_block) const;
    void checkDuplicatePath(const ServerConfig& server_block) const;
    void checkReturnCode(const LocationConfig& location_block) const;
    void checkUrl(const LocationConfig& location_block) const;
};

#endif