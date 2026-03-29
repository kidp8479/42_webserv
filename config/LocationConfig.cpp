#include "LocationConfig.hpp"

LocationConfig::LocationConfig() {
}

LocationConfig::LocationConfig(const LocationConfig& copy) {
}

LocationConfig& LocationConfig::operator=(const LocationConfig& other) {
}

LocationConfig::~LocationConfig() {
}

// getters
const std::string& LocationConfig::getPath() const {
}

const std::vector<std::string>& LocationConfig::getMethods() const {
}

const std::string& LocationConfig::getRoot() const {
}

const std::string& LocationConfig::getIndex() const {
}

bool LocationConfig::getDirectoryListing() const {
}

const std::string& LocationConfig::getUploadPath() const {
}

const std::vector<std::string>& LocationConfig::getCgiExtensions() const {
}

int LocationConfig::getReturnCode() const {
}

const std::string& LocationConfig::getReturnUrl() const {
}

// setters
void LocationConfig::setPath(const std::string& path) {
}

void LocationConfig::setMethods(const std::vector<std::string>& methods) {
}

void LocationConfig::setRoot(const std::string& root) {
}

void LocationConfig::setIndex(const std::string& index) {
}

void LocationConfig::setDirectoryListing(bool directory_listing) {
}

void LocationConfig::setUploadPath(const std::string& upload_path) {
}

void LocationConfig::setCgiExtensions(
    const std::vector<std::string>& cgi_extensions) {
}

void LocationConfig::setReturnCode(int return_code) {
}

void LocationConfig::setReturnUrl(const std::string& return_url) {
}
