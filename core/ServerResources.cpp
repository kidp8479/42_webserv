#include "ServerResources.hpp"

ServerResources::ServerResources(const ServerConfig& config) :
	config_(config),
	router_(config_)
{}

const Router& ServerResources::router() const {
	return router_;
}

const ServerConfig& ServerResources::config() const {
	return config_;
}
