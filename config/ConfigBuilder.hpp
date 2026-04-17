#ifndef CONFIG_BUILDER_HPP
#define CONFIG_BUILDER_HPP

#include <string>
#include <vector>

#include "../logger/Logger.hpp"
#include "Config.hpp"
#include "ConfigTokenizer.hpp"

class ConfigBuilder {
public:
    ConfigBuilder();
    ~ConfigBuilder();

    Config build(const std::vector<Token>& raw_tokens);

private:
    ConfigBuilder(const ConfigBuilder& copy);
    ConfigBuilder& operator=(const ConfigBuilder& other);

    void configError(const std::string& msg) const;
    void expectSemicolon();
    int toInt(const std::string& s) const;
    size_t toSizeT(const std::string& s) const;

    ServerConfig parseServerBlock();
    void parseListen(ServerConfig& server_block);
    void parseClientBodySize(ServerConfig& server_block);

    size_t index_;
    const std::vector<Token>* tokens_list_;
};

#endif