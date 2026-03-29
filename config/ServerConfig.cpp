#include "ServerConfig.hpp"

ServerConfig::ServerConfig() {
}

ServerConfig::ServerConfig(const ServerConfig& copy) {
}

ServerConfig& ServerConfig::operator=(const ServerConfig& other) {
}

ServerConfig::~ServerConfig() {
}

// getters
const std::string& ServerConfig::getHost() const {
}

int ServerConfig::getPort() const {
}

size_t ServerConfig::getMaxBodySize() const {
}

const std::map<int, std::string>& ServerConfig::getErrorPages() const {
}

const std::vector<LocationConfig>& ServerConfig::getLocationBlock() const {
}

// setters
void ServerConfig::setHost(const std::string& host) {
}

void ServerConfig::setPort(int port) {
}

void ServerConfig::setMaxBodySize(size_t max_body_size) {
}

void ServerConfig::addErrorPage(int code, const std::string& path) {
}

void ServerConfig::addLocationBlock(const LocationConfig& location_block) {
}
