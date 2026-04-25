#include "ServerConfig.hpp"

ServerConfig::ServerConfig()
    : port_(kPortNotSet), max_body_size_(kDefaultMaxBodySize) {
}

ServerConfig::ServerConfig(const ServerConfig& copy)
    : host_(copy.host_),
      port_(copy.port_),
      max_body_size_(copy.max_body_size_),
      error_pages_(copy.error_pages_),
      location_block_(copy.location_block_) {
}

ServerConfig& ServerConfig::operator=(const ServerConfig& other) {
    if (this != &other) {
        host_ = other.host_;
        port_ = other.port_;
        max_body_size_ = other.max_body_size_;
        error_pages_ = other.error_pages_;
        location_block_ = other.location_block_;
    }
    return *this;
}

ServerConfig::~ServerConfig() {
}

// getters
const std::string& ServerConfig::getHost() const {
    return this->host_;
}

int ServerConfig::getPort() const {
    return this->port_;
}

size_t ServerConfig::getMaxBodySize() const {
    return this->max_body_size_;
}

const std::map<int, std::string>& ServerConfig::getErrorPages() const {
    return this->error_pages_;
}

const std::vector<LocationConfig>& ServerConfig::getLocationBlock() const {
    return this->location_block_;
}

// setters
void ServerConfig::setHost(const std::string& host) {
    this->host_ = host;
}

void ServerConfig::setPort(int port) {
    this->port_ = port;
}

void ServerConfig::setMaxBodySize(size_t max_body_size) {
    this->max_body_size_ = max_body_size;
}

void ServerConfig::addErrorPage(int code, const std::string& path) {
    this->error_pages_[code] = path;
}

void ServerConfig::addLocationBlock(const LocationConfig& location_block) {
    this->location_block_.push_back(location_block);
}
