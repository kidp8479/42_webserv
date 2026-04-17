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

void ConfigBuilder::expectSemicolon() {
    if ((*tokens_list_)[index_].value != ";") {
        std::ostringstream oss;
        oss << (*tokens_list_)[index_].line;
        configError("missing \";\" at line " + oss.str());
    }
    index_++;
}

int ConfigBuilder::toInt(const std::string& s) const {
    std::istringstream iss(s);
    int result;
    iss >> result;
    return result;
}

size_t ConfigBuilder::toSizeT(const std::string& s) const {
    std::istringstream iss(s);
    size_t result;
    iss >> result;
    return result;
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
        if ((*tokens_list_)[index_].value == "listen") {
            parseListen(server_block);
        } else if ((*tokens_list_)[index_].value == "client_max_body_size") {
            parseClientBodySize(server_block);

        } else if ((*tokens_list_)[index_].value == "error_page") {
            index_++;
        } else {
            std::ostringstream oss;
            oss << (*tokens_list_)[index_].line;
            configError("unexpected token \"" + (*tokens_list_)[index_].value +
                        "\" on line " + oss.str() + ", expected \"listen\"");
        }
        index_++;
    }
    // index reached end of vector without finding "}", block is unclosed
    if (index_ >= tokens_list_->size())
        configError("unclosed server block, expected \"}\"");
    // found "}", advance past it
    index_++;
    return server_block;
}

void ConfigBuilder::parseListen(ServerConfig& server_block) {
    index_++;  // advance past "listen"
    size_t delimiter_pos = (*tokens_list_)[index_].value.find(":");
    if (delimiter_pos == std::string::npos)
        configError("invalid listen value \"" + (*tokens_list_)[index_].value +
                    "\", expected \"host:port\"");
    server_block.setHost(
        (*tokens_list_)[index_].value.substr(0, delimiter_pos));
    server_block.setPort(
        toInt((*tokens_list_)[index_].value.substr(delimiter_pos + 1)));
    index_++;  // advance to ";"
    expectSemicolon();
}

void ConfigBuilder::parseClientBodySize(ServerConfig& server_block) {
    index_++;

    size_t unit_position =
        (*tokens_list_)[index_].value.find_first_of("KkMmGg");
    std::string body_size_value =
        (*tokens_list_)[index_].value.substr(0, unit_position);
    std::string body_size_unit =
        (*tokens_list_)[index_].value.substr(unit_position);

    size_t raw_size = toSizeT(body_size_value);
    size_t byte_size;
    if (body_size_unit == "k" || body_size_unit == "K")
        byte_size = raw_size * 1024;
    else if (body_size_unit == "m" || body_size_unit == "M")
        byte_size = raw_size * 1048576;
    else if (body_size_unit == "g" || body_size_unit == "G")
        byte_size = raw_size * 1073741824;
    else  // no unit — treat as bytes, like nginx
        byte_size = raw_size;
    server_block.setMaxBodySize(byte_size);
    index_++;  // advance to ";"
    expectSemicolon();
}