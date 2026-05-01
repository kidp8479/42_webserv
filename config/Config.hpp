#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <vector>

#include "ServerConfig.hpp"

/**
 * @brief Top-level output of the config parsing pipeline.
 *
 * Holds all ServerConfig objects produced by ConfigBuilder and validated by
 * ConfigValidator. Returned by value from ConfigParser::parse(), then passed
 * by const reference to Server for its entire lifetime.
 *
 * Copy constructor and assignment operator are public because ConfigParser
 * returns Config by value.
 *
 * @note Pure data container, no validation logic.
 */
class Config {
public:
    Config();
    Config(const Config& copy);
    Config& operator=(const Config& other);
    ~Config();

    const std::vector<ServerConfig>& getServerBlock() const;
    void addServerBlock(const ServerConfig& server_block);

private:
    std::vector<ServerConfig> server_block_;
};

#endif
