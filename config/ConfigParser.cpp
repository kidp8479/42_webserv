#include "ConfigParser.hpp"

ConfigParser::ConfigParser() {
}

ConfigParser::~ConfigParser() {
}

Config ConfigParser::parse(const std::string& file_path) {
    Config config;

    LOG_DEBUG() << "ConfigParser: parsing phase 1 - tokenizing " << file_path;
    ConfigTokenizer tokenizer(file_path);
    LOG_DEBUG() << "ConfigParser: parsing phase 1 done - "
                << tokenizer.getTokenList().size() << " tokens extracted";

    LOG_DEBUG()
        << "ConfigParser: parsing phase 2 - filling object with raw tokens";
    ConfigBuilder builder;
    config = builder.build(tokenizer.getTokenList());

    LOG_DEBUG() << "ConfigParser: parsing phase 3 to be coded";
    // ConfigValidator validator(config); => phase 3

    return config;
}