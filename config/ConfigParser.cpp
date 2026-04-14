#include "ConfigParser.hpp"

ConfigParser::ConfigParser() {
}

ConfigParser::~ConfigParser() {
}

Config ConfigParser::parse(const std::string& file_path) {
    Config config;

    LOG_DEBUG() << "ConfigParser: starting parsing phase 1 - tokenizing "
                << file_path;
    ConfigTokenizer tokenizer(file_path);
    LOG_DEBUG() << "ConfigParser: parsing phase 1 done - "
                << tokenizer.getTokenList().size() << " tokens extracted";

    LOG_DEBUG() << "ConfigParser: parsing phase 2 to be coded";
    // ConfigBuilder builder(tokenizer.getTokenList()); => phase 2
    LOG_DEBUG() << "ConfigParser: parsing phase 3 to be coded";
    // ConfigValidator validator(config); => phase 3

    return config;
}