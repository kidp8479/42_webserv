#include "ConfigBuilder.hpp"

/**
 * @brief Constructs a ConfigBuilder object.
 *
 * @note: initialize index_ to 0 et tokens_list_ to NULL (pointer to a vector of
 * Token)
 */
ConfigBuilder::ConfigBuilder() : index_(0), tokens_list_(NULL) {
}

/**
 * @brief Destroys the ConfigBuilder.
 */
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

void ConfigBuilder::unknownDirectiveError(const Token& current_token) {
    std::ostringstream oss;
    oss << current_token.line;
    configError("unknown directive \"" + current_token.value + "\" on line " +
                oss.str());
}

void ConfigBuilder::checkBounds(const std::string& context) {
    if (index_ >= tokens_list_->size()) {
        configError("unexpected end of file " + context);
    }
}

void ConfigBuilder::expectSemicolon() {
    checkBounds("expected \";\"");
    const Token& current_token = (*tokens_list_)[index_];
    if (current_token.value != ";") {
        std::ostringstream oss;
        oss << current_token.line;
        configError("missing \";\" at line " + oss.str());
    }
    index_++;
}

void ConfigBuilder::expectOpenBrace() {
    const Token& open_brace = (*tokens_list_)[index_];
    if (open_brace.value != "{") {
        std::ostringstream oss;
        oss << open_brace.line;
        configError("unexpected token \"" + open_brace.value + "\" on line " +
                    oss.str() + ", expected \"{\"");
    }
    index_++;  // advance past "{"
}

int ConfigBuilder::toInt(const std::string& s) const {
    std::istringstream iss(s);
    int result;
    if (!(iss >> result)) {
        configError("invalid integer value: \"" + s + "\"");
    }
    return result;
}

size_t ConfigBuilder::toSizeT(const std::string& s) const {
    std::istringstream iss(s);
    size_t result;
    if (!(iss >> result)) {
        configError("invalid size value: \"" + s + "\"");
    }
    return result;
}

Config ConfigBuilder::build(const std::vector<Token>& raw_tokens) {
    Config config;
    index_ = 0;
    tokens_list_ = &raw_tokens;

    while (index_ < tokens_list_->size()) {
        const Token& current_token = (*tokens_list_)[index_];
        if (current_token.value != "server") {
            std::ostringstream oss;
            oss << current_token.line;
            configError("unexpected token \"" + current_token.value +
                        "\" on line " + oss.str() + ", expected \"server\"");
        }
        config.addServerBlock(parseServerBlock());
    }
    LOG_DEBUG() << "ConfigBuilder: config object successfully filled";
    return config;
}

ServerConfig ConfigBuilder::parseServerBlock() {
    ServerConfig server_block;

    index_++;  // advance past "server"
    checkBounds("after \"server\", expected \"{\"");
    expectOpenBrace();

    // loop through directives until "}" or end of file
    while (index_ < tokens_list_->size() &&
           (*tokens_list_)[index_].value != "}") {
        const Token& current_token = (*tokens_list_)[index_];
        if (current_token.value == "listen") {
            parseListen(server_block);
        } else if (current_token.value == "client_max_body_size") {
            parseClientBodySize(server_block);
        } else if (current_token.value == "error_page") {
            parseErrorPage(server_block);
        } else if (current_token.value == "location") {
            parseLocationBlock(server_block);
        } else {
            unknownDirectiveError(current_token);
        }
    }
    if (index_ >= tokens_list_->size()) {
        configError("unclosed server block, expected \"}\"");
    }
    index_++;  // advance past "}"
    return server_block;
}

void ConfigBuilder::parseListen(ServerConfig& server_block) {
    index_++;  // advance past "listen"
    checkBounds("after \"listen\"");

    const Token& current_token = (*tokens_list_)[index_];
    size_t delimiter_pos = current_token.value.find(":");
    if (delimiter_pos == std::string::npos) {
        configError("invalid listen value \"" + current_token.value +
                    "\", expected \"host:port\"");
    }
    server_block.setHost(current_token.value.substr(0, delimiter_pos));
    server_block.setPort(toInt(current_token.value.substr(delimiter_pos + 1)));
    index_++;  // advance to ";"
    expectSemicolon();
}

void ConfigBuilder::parseClientBodySize(ServerConfig& server_block) {
    index_++;  // advance past "client_max_body_size"
    checkBounds("after \"client_max_body_size\"");

    const Token& current_token = (*tokens_list_)[index_];
    size_t unit_pos = current_token.value.find_first_of("KkMmGg");
    size_t byte_size;
    if (unit_pos == std::string::npos) {
        // no unit — treat entire value as bytes, like nginx
        byte_size = toSizeT(current_token.value);
    } else {
        size_t raw_size = toSizeT(current_token.value.substr(0, unit_pos));
        std::string unit = current_token.value.substr(unit_pos);
        if (unit == "k" || unit == "K") {
            byte_size = raw_size * BYTES_PER_KB;
        } else if (unit == "m" || unit == "M") {
            byte_size = raw_size * BYTES_PER_MB;
        } else {
            byte_size = raw_size * BYTES_PER_GB;  // "g" or "G"
        }
    }
    server_block.setMaxBodySize(byte_size);
    index_++;  // advance to ";"
    expectSemicolon();
}

void ConfigBuilder::parseErrorPage(ServerConfig& server_block) {
    index_++;  // advance past "error_page"
    checkBounds("after \"error_page\"");

    int code = toInt((*tokens_list_)[index_].value);
    index_++;  // advance to path
    checkBounds("after error_page code");

    const std::string& path = (*tokens_list_)[index_].value;
    server_block.addErrorPage(code, path);
    index_++;  // advance to ";"
    expectSemicolon();
}

void ConfigBuilder::parseLocationBlock(ServerConfig& server_block) {
    LocationConfig location_block;

    index_++;  // advance past "location"
    checkBounds("after \"location\", expected path");

    location_block.setPath((*tokens_list_)[index_].value);
    index_++;  // advance to "{"
    checkBounds("after location path, expected \"{\"");
    expectOpenBrace();

    // TODO: parse location directives - if/else first - lookup table maybe next
    while (index_ < tokens_list_->size() &&
           (*tokens_list_)[index_].value != "}") {
        const Token& current_token = (*tokens_list_)[index_];

        if (current_token.value == "methods") {
        } else if (current_token.value == "root") {
        } else if (current_token.value == "index") {
        } else if (current_token.value == "autoindex") {
        } else if (current_token.value == "upload_path") {
        } else if (current_token.value == "cgi") {
        } else if (current_token.value == "return") {
        } else {
            unknownDirectiveError(current_token);
        }
    }
    if (index_ >= tokens_list_->size()) {
        configError("unclosed location block, expected \"}\"");
    }
    index_++;  // advance past "}"
    server_block.addLocationBlock(location_block);
}

void ConfigBuilder::parseMethods(LocationConfig& location_block) {
}

void ConfigBuilder::parseRoot(LocationConfig& location_block) {
}

void ConfigBuilder::parseIndex(LocationConfig& location_block) {
}

void ConfigBuilder::parseAutoIndex(LocationConfig& location_block) {
}

void ConfigBuilder::parseUploadPath(LocationConfig& location_block) {
}

void ConfigBuilder::parseCGI(LocationConfig& location_block) {
}

void ConfigBuilder::parseReturn(LocationConfig& location_block) {
}