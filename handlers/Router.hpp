#ifndef ROUTER_HPP
#define ROUTER_HPP

#include "../config/LocationConfig.hpp"
#include "../config/ServerConfig.hpp"

class Router {
public:
    Router(const ServerConfig& server_config);
    ~Router();
    const LocationConfig& resolve(const std::string& uri) const;

private:
    Router(const Router& copy);
    Router& operator=(const Router& other);

    const ServerConfig& server_config_;
};
#endif
