#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include <string>

#include "Config.hpp"
#include "ConfigBuilder.hpp"
#include "ConfigTokenizer.hpp"

/**
 * @brief Orchestrates the three config parsing phases.
 *
 * Delegates to ConfigTokenizer (phase 1), ConfigBuilder (phase 2),
 * and ConfigValidator (phase 3) to produce a fully validated Config object.
 *
 * @note Copy and assignment are disabled: this class is not meant to be copied.
 */
class ConfigParser {
public:
    ConfigParser();
    ~ConfigParser();

    Config parse(const std::string& file_path);

private:
    ConfigParser(const ConfigParser& copy);
    ConfigParser& operator=(const ConfigParser& other);
};

#endif