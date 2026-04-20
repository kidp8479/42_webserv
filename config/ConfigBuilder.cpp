#include "ConfigBuilder.hpp"

/**
 * @brief Constructs a ConfigBuilder object.
 *
 * @note: initialize index_ to 0 et tokens_list_ to NULL (pointer to a vector of
 * Token), object starts in a clean state.
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
 * @throws std::runtime_error always
 */
void ConfigBuilder::configError(const std::string& msg) const {
    std::string full = "Config: " + msg;
    LOG_ERROR() << full;
    throw std::runtime_error(full);
}

/**
 * @brief Throws a configError for an unknown directive, including its name and
 * line number.
 *
 * @param current_token The unrecognized token
 * @throws std::runtime_error always
 */
void ConfigBuilder::unknownDirectiveError(const Token& current_token) {
    std::ostringstream oss;
    oss << current_token.line;
    configError("unknown directive \"" + current_token.value + "\" on line " +
                oss.str());
}

/**
 * @brief Returns a const reference to the current token.
 *
 * @return const Token& The token at index_
 * @note Exists to avoid repeating (*tokens_list_)[index_] everywhere in the
 * class.
 */
const Token& ConfigBuilder::currentToken() const {
    return (*tokens_list_)[index_];
}

/**
 * @brief Throws if index_ is past the end of the token list.
 *
 * @param context Description of where we are, included in the error message
 * @throws std::runtime_error if index_ >= tokens_list_->size()
 */
void ConfigBuilder::checkBounds(const std::string& context) {
    if (index_ >= tokens_list_->size()) {
        configError("unexpected end of file " + context);
    }
}

/**
 * @brief Verifies the current token is ";" and advances past it.
 *
 * @throws std::runtime_error if end of file or current token is not ";"
 */
void ConfigBuilder::expectSemicolon() {
    checkBounds("expected \";\"");
    const Token& current_token = currentToken();
    if (current_token.value != ";") {
        std::ostringstream oss;
        oss << current_token.line;
        configError("missing \";\" at line " + oss.str());
    }
    index_++;
}

/**
 * @brief Verifies the current token is "{" and advances past it.
 *
 * @throws std::runtime_error if current token is not "{"
 */
void ConfigBuilder::expectOpenBrace() {
    const Token& open_brace = currentToken();
    if (open_brace.value != "{") {
        std::ostringstream oss;
        oss << open_brace.line;
        configError("unexpected token \"" + open_brace.value + "\" on line " +
                    oss.str() + ", expected \"{\"");
    }
    index_++;  // advance past "{"
}

/**
 * @brief Converts a string to int via istringstream.
 *
 * @param s The string to convert
 * @return int The converted value
 * @throws std::runtime_error if the string is not a valid integer
 */
int ConfigBuilder::toInt(const std::string& s) const {
    std::istringstream iss(s);
    int result;
    if (!(iss >> result)) {
        configError("invalid integer value: \"" + s + "\"");
    }
    return result;
}

/**
 * @brief Converts a string to size_t via istringstream.
 *
 * @param s The string to convert
 * @return size_t The converted value
 * @throws std::runtime_error if the string is not a valid size
 */
size_t ConfigBuilder::toSizeT(const std::string& s) const {
    std::istringstream iss(s);
    size_t result;
    if (!(iss >> result)) {
        configError("invalid size value: \"" + s + "\"");
    }
    return result;
}

/**
 * @brief Entry point. Consumes the token list and returns a filled Config
 * object.
 *
 * @param raw_tokens The token list produced by ConfigTokenizer
 * @return Config The fully parsed config object
 * @throws std::runtime_error on any sequence error
 */
Config ConfigBuilder::build(const std::vector<Token>& raw_tokens) {
    Config config;
    index_ = 0;
    tokens_list_ = &raw_tokens;

    while (index_ < tokens_list_->size()) {
        const Token& current_token = currentToken();
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

/**
 * @brief Parses one server { } block and returns a filled ServerConfig.
 *
 * @return ServerConfig The parsed server block
 * @throws std::runtime_error on missing braces, unclosed block, or unknown
 * directive
 */
ServerConfig ConfigBuilder::parseServerBlock() {
    ServerConfig server_block;

    index_++;  // advance past "server"
    checkBounds("after \"server\", expected \"{\"");
    expectOpenBrace();

    // loop through directives until "}" or end of file
    while (index_ < tokens_list_->size() && currentToken().value != "}") {
        const Token& current_token = currentToken();
        if (current_token.value == "listen") {
            parseListen(server_block);
        } else if (current_token.value == "client_max_body_size") {
            parseClientBodySize(server_block);
        } else if (current_token.value == "error_page") {
            parseErrorPage(server_block);
        } else if (current_token.value == "location") {
            server_block.addLocationBlock(parseLocationBlock());
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

/**
 * @brief Parses a listen directive and sets host and port on the server block.
 *
 * @param server_block The ServerConfig to fill
 * @throws std::runtime_error if the value is missing or has no ":" separator
 * @note Parses: listen 127.0.0.1:8080;
 */
void ConfigBuilder::parseListen(ServerConfig& server_block) {
    index_++;  // advance past "listen"
    checkBounds("after \"listen\"");

    const Token& current_token = currentToken();
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

/**
 * @brief Parses a client_max_body_size directive and sets the max body size in
 * bytes.
 *
 * @param server_block The ServerConfig to fill
 * @throws std::runtime_error if the value is missing or not a valid number
 * @note Accepts an optional unit suffix: K/k (kilobytes), M/m (megabytes), G/g
 * (gigabytes). No suffix is treated as raw bytes, consistent with nginx
 * behavior.
 * @note Parses: client_max_body_size 10M;
 */
void ConfigBuilder::parseClientBodySize(ServerConfig& server_block) {
    index_++;  // advance past "client_max_body_size"
    checkBounds("after \"client_max_body_size\"");

    const Token& current_token = currentToken();
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

/**
 * @brief Parses an error_page directive and stores the code/path pair.
 *
 * @param server_block The ServerConfig to fill
 * @throws std::runtime_error if the code or path is missing
 * @note Parses: error_page 404 /errors/404.html;
 */
void ConfigBuilder::parseErrorPage(ServerConfig& server_block) {
    index_++;  // advance past "error_page"
    checkBounds("after \"error_page\"");

    int code = toInt(currentToken().value);

    index_++;  // advance to path
    checkBounds("after error_page code");

    const std::string& path = currentToken().value;
    server_block.addErrorPage(code, path);

    index_++;  // advance to ";"
    expectSemicolon();
}

/**
 * @brief Parses one location { } block and returns a filled LocationConfig.
 *
 * @return LocationConfig The parsed location block
 * @throws std::runtime_error on missing braces, unclosed block, or unknown
 * directive
 * @note Parses: location /path { ... }
 */
LocationConfig ConfigBuilder::parseLocationBlock() {
    LocationConfig location_block;

    index_++;  // advance past "location"
    checkBounds("after \"location\", expected path");

    location_block.setPath(currentToken().value);
    index_++;  // advance to "{"
    checkBounds("after location path, expected \"{\"");
    expectOpenBrace();

    while (index_ < tokens_list_->size() && currentToken().value != "}") {
        const Token& current_token = currentToken();

        if (current_token.value == "methods") {
            parseMethods(location_block);
        } else if (current_token.value == "root") {
            parseRoot(location_block);
        } else if (current_token.value == "index") {
            parseIndex(location_block);
        } else if (current_token.value == "autoindex") {
            parseAutoIndex(location_block);
        } else if (current_token.value == "upload_path") {
            parseUploadPath(location_block);
        } else if (current_token.value == "cgi") {
            parseCGI(location_block);
        } else if (current_token.value == "return") {
            parseReturn(location_block);
        } else {
            unknownDirectiveError(current_token);
        }
    }
    if (index_ >= tokens_list_->size()) {
        configError("unclosed location block, expected \"}\"");
    }
    index_++;  // advance past "}"
    return location_block;
}

/**
 * @brief Parses a methods directive and sets the allowed HTTP methods.
 *
 * @param location_block The LocationConfig to fill
 * @throws std::runtime_error if a token is not GET, POST, or DELETE, or if ";"
 * is missing
 * @note Parses: methods GET POST DELETE;
 */
void ConfigBuilder::parseMethods(LocationConfig& location_block) {
    index_++;
    checkBounds("after \"methods\", expected GET and/or POST and/or DELETE");

    std::vector<std::string> collect_methods;
    while (index_ < tokens_list_->size() && currentToken().value != ";") {
        if (currentToken().value == "GET" || currentToken().value == "POST" ||
            currentToken().value == "DELETE") {
            collect_methods.push_back(currentToken().value);
        } else {
            configError(
                "unexpected token, \"methods\" accepts GET POST DELETE only.");
        }
        index_++;
    }
    if (index_ >= tokens_list_->size()) {
        configError("unclosed methods directive, expected \";\"");
    }

    location_block.setMethods(collect_methods);

    expectSemicolon();
}

/**
 * @brief Parses a root directive and sets the root path.
 *
 * @param location_block The LocationConfig to fill
 * @note Parses: root www/html;
 */
void ConfigBuilder::parseRoot(LocationConfig& location_block) {
    index_++;
    checkBounds("after \"root\", expected path");

    location_block.setRoot(currentToken().value);

    index_++;  // advance to ";"
    expectSemicolon();
}

/**
 * @brief Parses an index directive and sets the default index file.
 *
 * @param location_block The LocationConfig to fill
 * @note Parses: index index.html;
 */
void ConfigBuilder::parseIndex(LocationConfig& location_block) {
    index_++;
    checkBounds("after \"index\", expected path");

    location_block.setIndex(currentToken().value);

    index_++;  // advance to ";"
    expectSemicolon();
}

/**
 * @brief Parses an autoindex directive and enables or disables directory
 * listing.
 *
 * @param location_block The LocationConfig to fill
 * @throws std::runtime_error if the value is not "on" or "off"
 * @note Parses: autoindex on;
 */
void ConfigBuilder::parseAutoIndex(LocationConfig& location_block) {
    index_++;
    bool directory_listing = false;
    checkBounds("after \"autoindex\", expected on or off");

    if (currentToken().value == "off") {
        directory_listing = false;
    } else if (currentToken().value == "on") {
        directory_listing = true;
    } else {
        configError("unexpected token, \"autoindex\" is either on or off.");
    }

    location_block.setDirectoryListing(directory_listing);

    index_++;  // advance to ";"
    expectSemicolon();
}

/**
 * @brief Parses an upload_path directive and sets the upload directory.
 *
 * @param location_block The LocationConfig to fill
 * @note Parses: upload_path www/uploads;
 */
void ConfigBuilder::parseUploadPath(LocationConfig& location_block) {
    index_++;
    checkBounds("after \"upload_path\", expected path");

    location_block.setUploadPath(currentToken().value);

    index_++;  // advance to ";"
    expectSemicolon();
}

void ConfigBuilder::parseCGI(LocationConfig& location_block) {
    index_++;
    checkBounds("after \"cgi\", expected extension + path to binary");

    std::string cgi_extension = currentToken().value;
    index_++;
    checkBounds("after cgi extention (ex:\".php\") expexted binary path");
    std::string cgi_binary_path = currentToken().value;

    location_block.addCgiInterpreter(cgi_extension, cgi_binary_path);

    index_++;  // advance to ";"
    expectSemicolon();
}

void ConfigBuilder::parseReturn(LocationConfig& location_block) {
    (void)location_block;
    index_++;
    checkBounds("after \"return\", code + path");

    index_++;  // advance to ";"
    expectSemicolon();
}