#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include <string>

#include "Config.hpp"
#include "ConfigTokenizer.hpp"

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