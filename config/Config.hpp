#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <vector>

#include "ServerConfig.hpp"

class Config {
private:
    std::vector<ServerConfig> server_block_;

public:
    Config();
    Config(const Config& copy);
    Config& operator=(const Config& other);
    ~Config();

    const std::vector<ServerConfig>& getServerBlock() const;
    // server { ... } (one block at a time)
    void addServerBlock(const ServerConfig& server_block);
};

#endif