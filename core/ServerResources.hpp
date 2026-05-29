#ifndef SERVERRESOURCES_HPP
#define SERVERRESOURCES_HPP

#include "../config/Config.hpp"
#include "../handlers/Router.hpp"

class ServerResources {
public:
    ServerResources(const ServerConfig& server_config);

    ServerResources(const ServerResources& other);
    ~ServerResources();

    const Router& getRouter() const;
    const ServerConfig& getServerConfig() const;

private:
    ServerResources& operator=(const ServerResources&);

    ServerConfig server_config_;
    Router router_;

    //Maps a session_id to it's session data, eg:
    //[id_a3f9] -> {"username":"bob", "logged_in":"true", ...}
    std::map<std::string, std::map<std::string, std::string>> sessions_;
};

#endif
