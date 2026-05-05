#include "LocationConfig.hpp"

LocationConfig::LocationConfig()
    : directory_listing_(false),
      return_code_(kNoRedirect) {  // sentinel: handler checks != kNoRedirect
                                   // before applying redirect
}

LocationConfig::LocationConfig(const LocationConfig& copy)
    : path_(copy.path_),
      methods_(copy.methods_),
      root_(copy.root_),
      index_(copy.index_),
      directory_listing_(copy.directory_listing_),
      upload_path_(copy.upload_path_),
      cgi_interpreters_(copy.cgi_interpreters_),
      return_code_(copy.return_code_),
      return_url_(copy.return_url_) {
}

LocationConfig& LocationConfig::operator=(const LocationConfig& other) {
    if (this != &other) {
        path_ = other.path_;
        methods_ = other.methods_;
        root_ = other.root_;
        index_ = other.index_;
        directory_listing_ = other.directory_listing_;
        upload_path_ = other.upload_path_;
        cgi_interpreters_ = other.cgi_interpreters_;
        return_code_ = other.return_code_;
        return_url_ = other.return_url_;
    }
    return *this;
}

LocationConfig::~LocationConfig() {
}

const std::string& LocationConfig::getPath() const {
    return path_;
}

const std::vector<std::string>& LocationConfig::getMethods() const {
    return methods_;
}

const std::string& LocationConfig::getRoot() const {
    return root_;
}

const std::string& LocationConfig::getIndex() const {
    return index_;
}

bool LocationConfig::getDirectoryListing() const {
    return directory_listing_;
}

const std::string& LocationConfig::getUploadPath() const {
    return upload_path_;
}

const std::map<std::string, std::string>& LocationConfig::getCgiInterpreters()
    const {
    return cgi_interpreters_;
}

int LocationConfig::getReturnCode() const {
    return return_code_;
}

const std::string& LocationConfig::getReturnUrl() const {
    return return_url_;
}

void LocationConfig::setPath(const std::string& path) {
    path_ = path;
}

void LocationConfig::setMethods(const std::vector<std::string>& methods) {
    methods_ = methods;
}

void LocationConfig::setRoot(const std::string& root) {
    root_ = root;
}

void LocationConfig::setIndex(const std::string& index) {
    index_ = index;
}

void LocationConfig::setDirectoryListing(bool directory_listing) {
    directory_listing_ = directory_listing;
}

void LocationConfig::setUploadPath(const std::string& upload_path) {
    upload_path_ = upload_path;
}

void LocationConfig::addCgiInterpreter(const std::string& extension,
                                       const std::string& binary) {
    cgi_interpreters_[extension] = binary;
}

void LocationConfig::setReturnCode(int return_code) {
    return_code_ = return_code;
}

void LocationConfig::setReturnUrl(const std::string& return_url) {
    return_url_ = return_url;
}
