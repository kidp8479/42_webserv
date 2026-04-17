#include "ConfigBuilder.hpp"

#include <sstream>

ConfigBuilder::ConfigBuilder() : index_(0), tokens_list_(NULL) {
}

ConfigBuilder::~ConfigBuilder() {
}

/**
 * @brief Logs an error and throws a std::runtime_error with a "Config: "
 * prefix.
 *
 * @param msg The error message (without the "Config: " prefix)
 * @throws std::runtime_error Always
 */
void ConfigBuilder::configError(const std::string& msg) const {
    std::string full = "Config: " + msg;
    LOG_ERROR() << full;
    throw std::runtime_error(full);
}

Config ConfigBuilder::build(const std::vector<Token>& raw_tokens) {
    Config config;
    index_ = 0;
    tokens_list_ = &raw_tokens;

    while (index_ < tokens_list_->size()) {
        if ((*tokens_list_)[index_].value != "server") {
            // size_t can't be concatenated with std::string directly
            // convert via ostringstream first
            std::ostringstream oss;
            oss << (*tokens_list_)[index_].line;
            configError("unexpected token \"" + (*tokens_list_)[index_].value +
                        "\" on line " + oss.str() + ", expected \"server\"");
        } else {
            config.addServerBlock(parseServerBlock());
        }
        LOG_DEBUG() << "config object successfully filled.";
    }
    return (config);
}

ServerConfig ConfigBuilder::parseServerBlock() {
    ServerConfig server_block;

    // advance index 1 spot to move from "server" token
    index_++;

    // check if next token is "{"
    if ((*tokens_list_)[index_].value != "{") {
        std::ostringstream oss;
        oss << (*tokens_list_)[index_].line;
        configError("unexpected token \"" + (*tokens_list_)[index_].value +
                    "\" on line " + oss.str() + ", expected \"{\"");
    }

    index_++;  // advance past "{"

    // go throught the list of tokens, looping untill we find "}" and with
    // boundaries checks in case we don't find it
    while (index_ < tokens_list_->size() &&
           (*tokens_list_)[index_].value != "}") {
        // TODO
        index_++;
    }
    // index reached end of vector without finding "}", block is unclosed
    if (index_ >= tokens_list_->size())
        configError("unclosed server block, expected \"}\"");
    // found "}", advance past it
    index_++;
    return server_block;
}