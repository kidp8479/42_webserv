#ifndef SERVERRESOURCES_HPP
#define SERVERRESOURCES_HPP

#include "../config/Config.hpp"
#include "../handlers/Router.hpp"

class ServerResources {
public:
	ServerResources(const Config& config);
	const Router& router() const;

private:
	Config config_;
	Router router_;
};

#endif
