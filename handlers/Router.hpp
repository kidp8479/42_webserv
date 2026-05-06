#ifndef ROUTER_HPP
#define ROUTER_HPP
#include "../config/Config.hpp"
#include "../config/LocationConfig.hpp"
#include "../http/Request.hpp"

class Router {
public:
    explicit Router(const Config& config);
    const LocationConfig& resolve(const Request& request) const;
private:
    const Config& config_;
};
#endif
