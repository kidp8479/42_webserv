#ifndef CONFIG_BUILDER_HPP
#define CONFIG_BUILDER_HPP

#define BYTES_PER_KB 1024
#define BYTES_PER_MB 1048576
#define BYTES_PER_GB 1073741824

#include <sstream>
#include <string>
#include <vector>

#include "../logger/Logger.hpp"
#include "Config.hpp"
#include "ConfigTokenizer.hpp"
#include "LocationConfig.hpp"

class ConfigBuilder {
public:
    ConfigBuilder();
    ~ConfigBuilder();

    Config build(const std::vector<Token>& raw_tokens);

private:
    ConfigBuilder(const ConfigBuilder& copy);
    ConfigBuilder& operator=(const ConfigBuilder& other);

    void configError(const std::string& msg) const;
    void unknownDirectiveError(const Token& current_token);

    const Token& currentToken() const;
    void checkBounds(const std::string& context);
    void expectSemicolon();
    void expectOpenBrace();
    int toInt(const std::string& s) const;
    size_t toSizeT(const std::string& s) const;

    ServerConfig parseServerBlock();
    void parseListen(ServerConfig& server_block);
    void parseClientBodySize(ServerConfig& server_block);
    void parseErrorPage(ServerConfig& server_block);

    LocationConfig parseLocationBlock();
    void parseMethods(LocationConfig& location_block);
    void parseRoot(LocationConfig& location_block);
    void parseIndex(LocationConfig& location_block);
    void parseAutoIndex(LocationConfig& location_block);
    void parseUploadPath(LocationConfig& location_block);
    void parseCGI(LocationConfig& location_block);
    void parseReturn(LocationConfig& location_block);

    size_t index_;
    const std::vector<Token>* tokens_list_;
};

#endif