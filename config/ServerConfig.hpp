#ifndef SERVER_CONFIG_HPP
#define SERVER_CONFIG_HPP

#include <map>
#include <string>
#include <vector>

#include "LocationConfig.hpp"

#define PORT_NOT_SET -1
#define DEFAULT_MAX_BODY_SIZE 1048576  // 1MB

class ServerConfig {
private:
    std::string host_;
    int port_;
    size_t max_body_size_;
    std::map<int, std::string> error_pages_;
    std::vector<LocationConfig> location_block_;

public:
    ServerConfig();
    ServerConfig(const ServerConfig& copy);
    ServerConfig& operator=(const ServerConfig& other);
    ~ServerConfig();

    // getters
    const std::string& getHost() const;
    int getPort() const;
    size_t getMaxBodySize() const;
    const std::map<int, std::string>& getErrorPages() const;
    const std::vector<LocationConfig>& getLocationBlock() const;

    // setters
    void setHost(const std::string& host);
    void setPort(int port);
    void setMaxBodySize(size_t max_body_size);
    // error_page 404 /errors/404.html; (one entry at a time)
    void addErrorPage(int code, const std::string& path);
    // location /upload { ... } (one block at a time)
    void addLocationBlock(const LocationConfig& location);
};

#endif