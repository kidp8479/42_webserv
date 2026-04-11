#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <vector>

#include "ServerConfig.hpp"

class Config {
public:
    Config();
    Config(const Config& copy);
    Config& operator=(const Config& other);
    ~Config();

    // getter
    const std::vector<ServerConfig>& getServerBlock() const;
    // add (not exactly a setter, does not work on the whole vector, works on 1
    // unit server { ... } one block at a time)
    void addServerBlock(const ServerConfig& server_block);

private:
    std::vector<ServerConfig> server_block_;
};

#endif
