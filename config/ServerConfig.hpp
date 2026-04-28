#ifndef SERVER_CONFIG_HPP
#define SERVER_CONFIG_HPP

#include <map>
#include <string>
#include <vector>

#include "LocationConfig.hpp"

/**
 * @brief Holds the parsed content of one server { } block.
 *
 * @note This is NOT the Server class that handles sockets and connections.
 * Pure data container, no validation logic.
 */
class ServerConfig {
public:
    static const int kPortNotSet = -1;
    static const size_t kDefaultMaxBodySize = 1048576;

    ServerConfig();
    ServerConfig(const ServerConfig& copy);
    ServerConfig& operator=(const ServerConfig& other);
    ~ServerConfig();

    const std::string& getHost() const;
    int getPort() const;
    size_t getMaxBodySize() const;
    const std::map<int, std::string>& getErrorPages() const;
    const std::vector<LocationConfig>& getLocationBlock() const;

    void setHost(const std::string& host);
    void setPort(int port);
    void setMaxBodySize(size_t max_body_size);
    void addErrorPage(int code, const std::string& path);
    void addLocationBlock(const LocationConfig& location);

private:
    std::string host_;
    int port_;
    size_t max_body_size_;
    std::map<int, std::string> error_pages_;
    std::vector<LocationConfig> location_block_;
};

#endif
