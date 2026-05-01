#ifndef CONFIG_BUILDER_HPP
#define CONFIG_BUILDER_HPP

#include <algorithm>
#include <cctype>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include "../logger/Logger.hpp"
#include "Config.hpp"
#include "ConfigTokenizer.hpp"
#include "LocationConfig.hpp"

/**
 * @brief Phase 2 of the config parsing pipeline. Converts a flat token list
 * produced by ConfigTokenizer into a fully populated Config object.
 *
 * Handles structural errors: token sequence order, missing semicolons or
 * braces, unknown directives, and duplicate directives within a block.
 * Semantic validation (port range, valid host, duplicate host:port) is
 * deferred to ConfigValidator (phase 3).
 *
 * @throws std::runtime_error on the first structural error found.
 *
 * @note Copy and assignment are disabled: this class is not meant to be copied.
 */
class ConfigBuilder {
public:
    ConfigBuilder();
    ~ConfigBuilder();

    Config build(const std::vector<Token>& raw_tokens);

private:
    ConfigBuilder(const ConfigBuilder& copy);
    ConfigBuilder& operator=(const ConfigBuilder& other);

    void configError(const std::string& msg) const;
    void configError(const Token& token, const std::string& msg) const;

    const Token& currentToken() const;
    void checkBounds(const std::string& context) const;
    void expectSemicolon();
    void expectOpenBrace();
    int toInt(const std::string& s) const;
    size_t toSizeT(const std::string& s) const;

    ServerConfig parseServerBlock();
    void parseListen(ServerConfig& server_block);
    void parseClientBodySize(ServerConfig& server_block, bool& seen);
    void parseErrorPage(ServerConfig& server_block);

    LocationConfig parseLocationBlock();
    void parseMethods(LocationConfig& location_block);
    void parseRoot(LocationConfig& location_block);
    void parseIndex(LocationConfig& location_block);
    void parseAutoIndex(LocationConfig& location_block, bool& seen);
    void parseUploadPath(LocationConfig& location_block);
    void parseCGI(LocationConfig& location_block);
    void parseReturn(LocationConfig& location_block);

    size_t index_;
    const std::vector<Token>* tokens_list_;
};

#endif