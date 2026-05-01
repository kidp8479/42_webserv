#include "Config.hpp"

Config::Config() {
}

Config::Config(const Config& copy) : server_block_(copy.server_block_) {
}

Config& Config::operator=(const Config& other) {
    if (this != &other) {
        server_block_ = other.server_block_;
    }
    return *this;
}

Config::~Config() {
}

const std::vector<ServerConfig>& Config::getServerBlock() const {
    return server_block_;
}

void Config::addServerBlock(const ServerConfig& server_block) {
    server_block_.push_back(server_block);
}
