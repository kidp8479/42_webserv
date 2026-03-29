#include "Config.hpp"

Config::Config() {
}

Config::Config(const Config& copy) {
}

Config& Config::operator=(const Config& other) {
}

Config::~Config() {
}

// getter
const std::vector<ServerConfig>& Config::getServerBlock() const {
}

// add
void Config::addServerBlock(const ServerConfig& server_block) {
}
