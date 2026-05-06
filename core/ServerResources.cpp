#include "ServerResources.hpp"

ServerResources::ServerResources(const Config& config) :
	config_(config),
	router_(config_)
{}

const Router& ServerResources::router() const {
	return router_;
}
