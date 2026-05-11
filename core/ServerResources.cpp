#include "ServerResources.hpp"

ServerResources::ServerResources(const ServerConfig& server_config)
    : server_config_(server_config), router_(server_config_) {
}

ServerResources::ServerResources(const ServerResources& other)
    : server_config_(other.server_config_),
      router_(server_config_)  // rebind here so no dangling ptr
{
}

ServerResources::~ServerResources() {
}

const Router& ServerResources::getRouter() const {
    return router_;
}

const ServerConfig& ServerResources::getServerConfig() const {
    return server_config_;
}
