#ifndef SERVERRESOURCES_HPP
#define SERVERRESOURCES_HPP

#include "../config/Config.hpp"
#include "../handlers/Router.hpp"

class ServerResources {
public:
	ServerResources(const ServerConfig& server_config);

	ServerResources(const ServerResources& other);
	~ServerResources();

	const Router& router() const;
	const ServerConfig& serverConfig() const;

private:
	ServerResources& operator=(const ServerResources&);

	ServerConfig server_config_;
	Router router_;
};

#endif
