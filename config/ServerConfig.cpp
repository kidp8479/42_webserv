#include "ServerConfig.hpp"

#include "HttpConstants.hpp"

ServerConfig::ServerConfig()
    : port_(kPortNotSet), max_body_size_(HttpConstants::kDefaultMaxBodySize) {
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

const std::string& ServerConfig::getHost() const {
    return host_;
}

int ServerConfig::getPort() const {
    return port_;
}

size_t ServerConfig::getMaxBodySize() const {
    return max_body_size_;
}

const std::map<int, std::string>& ServerConfig::getErrorPages() const {
    return error_pages_;
}

const std::vector<LocationConfig>& ServerConfig::getLocationBlock() const {
    return location_block_;
}

void ServerConfig::setHost(const std::string& host) {
    host_ = host;
}

void ServerConfig::setPort(int port) {
    port_ = port;
}

void ServerConfig::setMaxBodySize(size_t max_body_size) {
    max_body_size_ = max_body_size;
}

void ServerConfig::addErrorPage(int code, const std::string& path) {
    error_pages_[code] = path;
}

void ServerConfig::addLocationBlock(const LocationConfig& location_block) {
    location_block_.push_back(location_block);
}
