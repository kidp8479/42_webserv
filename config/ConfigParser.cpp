#include "ConfigParser.hpp"

ConfigParser::ConfigParser() {
}

ConfigParser::~ConfigParser() {
}

/**
 * @brief Parses a .conf file and returns a validated Config object.
 *
 * @param file_path Path to the .conf file (absolute or relative)
 * @return Fully populated and validated Config object
 * @throws std::runtime_error If the file is invalid or contains errors
 */
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