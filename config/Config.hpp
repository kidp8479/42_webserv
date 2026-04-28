#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <vector>

#include "ServerConfig.hpp"

/**
 * @brief Top-level config container, holds all parsed server blocks.
 *
 * Built incrementally by ConfigBuilder via addServerBlock().
 * Pure data container, no validation logic.
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
