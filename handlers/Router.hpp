#ifndef ROUTER_HPP
#define ROUTER_HPP

#include "../config/LocationConfig.hpp"
#include "../config/ServerConfig.hpp"
#include "../http/Request.hpp"

class Router {
public:
    explicit Router(const ServerConfig& server_config);
    const LocationConfig& resolve(const Request& request) const;

private:
    const ServerConfig& server_config_;
};
#endif
