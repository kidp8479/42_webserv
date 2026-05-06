#ifndef ROUTER_HPP
#define ROUTER_HPP
#include "../config/Config.hpp"
#include "../config/LocationConfig.hpp"
#include "../http/Request.hpp"

class Router {
public:
    explicit Router(const ServerConfig& config_server);
    const LocationConfig& resolve(const Request& request) const;
private:
    const ServerConfig& config_server_;
};
#endif
