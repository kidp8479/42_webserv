#ifndef CONFIG_VALIDATOR_HPP
#define CONFIG_VALIDATOR_HPP

#include "../logger/Logger.hpp"
#include "Config.hpp"

/**
 * @brief Phase 3 of the config parsing pipeline. Validates the semantic
 * correctness of a Config object produced by ConfigBuilder.
 *
 * @throws std::runtime_error on the first invalid value found. If validate()
 * returns without throwing, the Config is guaranteed fully valid.
 *
 * @note Format policy: filesystem paths (root, upload_path, index) are not
 * format-checked, relative paths are valid in this self-contained project.
 * CGI binary paths are the exception: execve() requires an absolute path.
 */
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
    void checkDuplicateHostPort(const Config& config) const;
    void checkServerErrorCodes(const ServerConfig& server) const;

    void locationChecks(const ServerConfig& server) const;
    void checkPath(const LocationConfig& location) const;
    void checkDuplicatePath(const ServerConfig& server) const;
    void checkReturnCode(const LocationConfig& location) const;
    void checkCgiBinaryPaths(const LocationConfig& location) const;
};

#endif