#ifndef SERVERRESOURCES_HPP
#define SERVERRESOURCES_HPP

#include "../config/Config.hpp"
#include "../handlers/Router.hpp"

class ServerResources {
public:
	ServerResources(const ServerConfig& config);
	const Router& router() const;
	const ServerConfig& config() const;

private:
	ServerConfig config_;
	Router router_;
};

#endif
