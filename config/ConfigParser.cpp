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

    LOG_INFO() << BR_CYN "Config: starting to parse " << file_path << RESET;
    LOG_DEBUG() << BR_YEL "ConfigParser: parsing phase 1 - tokenizing "
                << file_path << RESET;
    ConfigTokenizer tokenizer(file_path);

    LOG_DEBUG() << BR_YEL
        "ConfigParser: parsing phase 2 - filling object with raw tokens" RESET;
    ConfigBuilder builder;
    config = builder.build(tokenizer.getTokenList());

    LOG_DEBUG() << BR_YEL "ConfigParser: validating Config object - WIP" RESET;
    LOG_INFO() << BR_CYN "Config: " << file_path << " fully parsed" << RESET;

    return config;
}