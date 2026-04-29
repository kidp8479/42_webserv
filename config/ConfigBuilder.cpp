#include "ConfigBuilder.hpp"

namespace {
const size_t BYTES_PER_KB = 1024;
const size_t BYTES_PER_MB = 1048576;
const size_t BYTES_PER_GB = 1073741824;
}  // namespace

/**
 * @brief Constructs a ConfigBuilder object.
 *
 * @note Initializes index_ to 0 and tokens_list_ to NULL (pointer to a vector
 * of Token), object starts in a clean state.
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
 * @brief Logs an error with token context and throws a std::runtime_error.
 *
 * @param token The token that caused the error (provides value and line number)
 * @param msg Description of the error
 * @throws std::runtime_error always
 * @note Overload of configError(const std::string&). Use this version when a
 * token is available, it automatically includes the token value and line number
 * in the message. Use the string-only version when no token is available (ex:
 * end of file, conversion errors).
 */
void ConfigBuilder::configError(const Token& token,
                                const std::string& msg) const {
    std::ostringstream oss;
    oss << token.line;
    configError("unexpected token \"" + token.value + "\" on line " +
                oss.str() + ", " + msg);
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
void ConfigBuilder::checkBounds(const std::string& context) const {
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
        configError(current_token, "missing \";\"");
    }
    index_++;
}

/**
 * @brief Verifies the current token is "{" and advances past it.
 *
 * @throws std::runtime_error if end of file or current token is not "{"
 */
void ConfigBuilder::expectOpenBrace() {
    checkBounds("expected \"{\"");
    const Token& open_brace = currentToken();
    if (open_brace.value != "{") {
        configError(open_brace, "expected \"{\"");
    }
    index_++;
}

/**
 * @brief Converts a string to int via istringstream.
 *
 * @param s The string to convert
 * @return int The converted value
 * @throws std::runtime_error if the string is not a valid integer
 * @note Rejects strings with trailing characters (ex: "8080abc") by attempting
 * a second extraction after the number.
 */
int ConfigBuilder::toInt(const std::string& s) const {
    std::istringstream iss(s);
    std::string leftover;
    int result;

    if (!(iss >> result)) {
        configError("invalid integer value: \"" + s + "\"");
    }
    if (iss >> leftover) {
        configError("malformed directive value");
    }
    return result;
}

/**
 * @brief Converts a string to size_t via istringstream.
 *
 * @param s The string to convert
 * @return size_t The converted value
 * @throws std::runtime_error if the string is not a valid size
 * @note Rejects strings with trailing characters (ex: "8080abc") by attempting
 * a second extraction after the number.
 */
size_t ConfigBuilder::toSizeT(const std::string& s) const {
    std::istringstream iss(s);
    std::string leftover;
    size_t result;

    if (!(iss >> result)) {
        configError("invalid size value: \"" + s + "\"");
    }
    if (iss >> leftover) {
        configError("malformed directive value");
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

    LOG_DEBUG() << BR_CYN "ConfigBuilder: starting build, "
                << tokens_list_->size() << " tokens" << RESET;

    while (index_ < tokens_list_->size()) {
        const Token& current_token = currentToken();
        if (current_token.value != "server") {
            configError(current_token, "expected \"server\"");
        }
        config.addServerBlock(parseServerBlock());
    }
    if (config.getServerBlock().empty()) {
        configError("config file contains no server block");
    }
    size_t total_locations = 0;
    for (size_t i = 0; i < config.getServerBlock().size(); i++) {
        total_locations += config.getServerBlock()[i].getLocationBlock().size();
    }
    LOG_INFO() << BR_CYN "ConfigBuilder: build complete - "
               << config.getServerBlock().size() << " server block(s), "
               << total_locations << " location block(s)" << RESET;
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
    bool max_body_size_seen = false;

    index_++;
    LOG_DEBUG() << BR_CYN "ConfigBuilder: parsing server block" << RESET;
    expectOpenBrace();

    while (index_ < tokens_list_->size() && currentToken().value != "}") {
        const Token& current_token = currentToken();
        if (current_token.value == "listen") {
            parseListen(server_block);
        } else if (current_token.value == "client_max_body_size") {
            parseClientBodySize(server_block, max_body_size_seen);
        } else if (current_token.value == "error_page") {
            parseErrorPage(server_block);
        } else if (current_token.value == "location") {
            server_block.addLocationBlock(parseLocationBlock());
        } else {
            configError(current_token, "unknown directive.");
        }
    }
    if (index_ >= tokens_list_->size()) {
        configError("unclosed server block, expected \"}\"");
    }
    index_++;
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
    index_++;
    checkBounds("after \"listen\"");

    const Token& current_token = currentToken();
    if (server_block.getPort() != ServerConfig::kPortNotSet) {
        configError(current_token, "duplicate \"listen\" directive");
    }

    size_t delimiter_pos = current_token.value.find(":");
    if (delimiter_pos == std::string::npos) {
        configError(current_token, "expected \"host:port\"");
    }

    server_block.setHost(current_token.value.substr(0, delimiter_pos));
    server_block.setPort(toInt(current_token.value.substr(delimiter_pos + 1)));
    LOG_DEBUG() << "ConfigBuilder: listen -> " << GRN << server_block.getHost()
                << ":" << server_block.getPort() << RESET;

    index_++;
    expectSemicolon();
}

/**
 * @brief Parses a client_max_body_size directive and sets the max body size in
 * bytes.
 *
 * @param server_block The ServerConfig to fill
 * @param seen Flag tracking whether this directive has already been parsed in
 * this server block. Passed by reference from parseServerBlock.
 * @throws std::runtime_error if the value is missing or not a valid number
 * @note Accepts an optional unit suffix: K/k (kilobytes), M/m (megabytes), G/g
 * (gigabytes). No suffix is treated as raw bytes, consistent with nginx
 * behavior.
 * @note Parses: client_max_body_size 10M;
 */
void ConfigBuilder::parseClientBodySize(ServerConfig& server_block,
                                        bool& seen) {
    index_++;
    checkBounds("after \"client_max_body_size\"");

    if (seen) {
        configError(currentToken(),
                    "duplicate \"client_max_body_size\" directive");
    }
    seen = true;

    const std::string& raw = currentToken().value;

    size_t i = 0;
    while (i < raw.size() && std::isdigit(raw[i])) {
        i++;
    }
    if (i == 0) {
        configError("invalid client_max_body_size value \"" + raw +
                    "\", value must start with a digit");
    }
    if (i < raw.size() && (raw.size() != i + 1 ||
                           (raw[i] != 'K' && raw[i] != 'k' && raw[i] != 'M' &&
                            raw[i] != 'm' && raw[i] != 'G' && raw[i] != 'g'))) {
        configError("invalid client_max_body_size value \"" + raw +
                    "\", expected K/M/G suffix or no suffix");
    }

    size_t numeric = toSizeT(raw.substr(0, i));
    size_t byte_size;
    if (i == raw.size()) {
        byte_size = numeric;
    } else if (raw[i] == 'K' || raw[i] == 'k') {
        if (numeric > std::numeric_limits<size_t>::max() / BYTES_PER_KB) {
            configError("client_max_body_size value overflows");
        }
        byte_size = numeric * BYTES_PER_KB;
    } else if (raw[i] == 'M' || raw[i] == 'm') {
        if (numeric > std::numeric_limits<size_t>::max() / BYTES_PER_MB) {
            configError("client_max_body_size value overflows");
        }
        byte_size = numeric * BYTES_PER_MB;
    } else {
        if (numeric > std::numeric_limits<size_t>::max() / BYTES_PER_GB) {
            configError("client_max_body_size value overflows");
        }
        byte_size = numeric * BYTES_PER_GB;
    }

    server_block.setMaxBodySize(byte_size);
    LOG_DEBUG() << "ConfigBuilder: client_max_body_size -> " << GRN << byte_size
                << " bytes" << RESET;

    index_++;
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
    index_++;
    checkBounds("after \"error_page\"");

    const Token& code_token = currentToken();
    int code = toInt(code_token.value);
    if (server_block.getErrorPages().find(code) !=
        server_block.getErrorPages().end()) {
        configError(code_token, "duplicate error_page code");
    }

    index_++;
    checkBounds("after error_page code");

    const std::string& path = currentToken().value;
    server_block.addErrorPage(code, path);
    LOG_DEBUG() << "ConfigBuilder: error_page -> " << GRN << code << " " << path
                << RESET;

    index_++;
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
    bool autoindex_seen = false;

    index_++;
    checkBounds("after \"location\", expected path");

    location_block.setPath(currentToken().value);
    LOG_DEBUG() << BR_CYN "ConfigBuilder: parsing location block \""
                << location_block.getPath() << "\"" << RESET;
    index_++;
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
            parseAutoIndex(location_block, autoindex_seen);
        } else if (current_token.value == "upload_path") {
            parseUploadPath(location_block);
        } else if (current_token.value == "cgi") {
            parseCGI(location_block);
        } else if (current_token.value == "return") {
            parseReturn(location_block);
        } else {
            configError(current_token, "unknown directive.");
        }
    }
    if (index_ >= tokens_list_->size()) {
        configError("unclosed location block, expected \"}\"");
    }
    index_++;
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

    if (!location_block.getMethods().empty()) {
        configError(currentToken(), "duplicate \"methods\" directive");
    }
    std::vector<std::string> collect_methods;
    while (index_ < tokens_list_->size() && currentToken().value != ";") {
        const Token& current_token = currentToken();
        if (current_token.value == "GET" || current_token.value == "POST" ||
            current_token.value == "DELETE") {
            if (std::find(collect_methods.begin(), collect_methods.end(),
                          current_token.value) != collect_methods.end()) {
                configError(current_token, "duplicate method");
            }
            collect_methods.push_back(current_token.value);
        } else {
            configError(current_token,
                        "\"methods\" accepts GET POST DELETE only.");
        }
        index_++;
    }
    if (index_ >= tokens_list_->size()) {
        configError("unclosed methods directive, expected \";\"");
    }

    if (collect_methods.empty()) {
        configError(
            "\"methods\" directive requires at least one method (GET POST "
            "DELETE)");
    }
    for (size_t i = 0; i < collect_methods.size(); i++) {
        LOG_DEBUG() << "ConfigBuilder: methods -> " << GRN << collect_methods[i]
                    << RESET;
    }
    location_block.setMethods(collect_methods);

    expectSemicolon();
}

/**
 * @brief Parses a root directive and sets the root path.
 *
 * @param location_block The LocationConfig to fill
 * @throws std::runtime_error if value is missing or ";" is absent
 * @note Parses: root /www/html;
 */
void ConfigBuilder::parseRoot(LocationConfig& location_block) {
    index_++;
    checkBounds("after \"root\", expected path");

    if (!location_block.getRoot().empty()) {
        configError(currentToken(), "duplicate \"root\" directive");
    }
    location_block.setRoot(currentToken().value);
    LOG_DEBUG() << "ConfigBuilder: root -> " << GRN << location_block.getRoot()
                << RESET;

    index_++;
    expectSemicolon();
}

/**
 * @brief Parses an index directive and sets the default index file.
 *
 * @param location_block The LocationConfig to fill
 * @throws std::runtime_error if value is missing or ";" is absent
 * @note Parses: index index.html;
 */
void ConfigBuilder::parseIndex(LocationConfig& location_block) {
    index_++;
    checkBounds("after \"index\", expected path");

    if (!location_block.getIndex().empty()) {
        configError(currentToken(), "duplicate \"index\" directive");
    }
    location_block.setIndex(currentToken().value);
    LOG_DEBUG() << "ConfigBuilder: index -> " << GRN
                << location_block.getIndex() << RESET;

    index_++;
    expectSemicolon();
}

/**
 * @brief Parses an autoindex directive and enables or disables directory
 * listing.
 *
 * @param location_block The LocationConfig to fill
 * @param seen Flag tracking whether this directive has already been parsed in
 * this location block. Passed by reference from parseLocationBlock.
 * @throws std::runtime_error if the value is not "on" or "off"
 * @note Parses: autoindex on;
 */
void ConfigBuilder::parseAutoIndex(LocationConfig& location_block, bool& seen) {
    index_++;
    checkBounds("after \"autoindex\", expected on or off");

    if (seen) {
        configError(currentToken(), "duplicate \"autoindex\" directive");
    }
    seen = true;
    bool directory_listing = false;

    if (currentToken().value == "on") {
        directory_listing = true;
    } else if (currentToken().value != "off") {
        configError(currentToken(), "\"autoindex\" accepts on or off only.");
    }

    location_block.setDirectoryListing(directory_listing);
    LOG_DEBUG() << "ConfigBuilder: autoindex -> " << GRN
                << (directory_listing ? "on" : "off") << RESET;

    index_++;
    expectSemicolon();
}

/**
 * @brief Parses an upload_path directive and sets the upload directory.
 *
 * @param location_block The LocationConfig to fill
 * @throws std::runtime_error if value is missing or ";" is absent
 * @note Parses: upload_path /www/uploads;
 */
void ConfigBuilder::parseUploadPath(LocationConfig& location_block) {
    index_++;
    checkBounds("after \"upload_path\", expected path");

    if (!location_block.getUploadPath().empty()) {
        configError(currentToken(), "duplicate \"upload_path\" directive");
    }
    location_block.setUploadPath(currentToken().value);
    LOG_DEBUG() << "ConfigBuilder: upload_path -> " << GRN
                << location_block.getUploadPath() << RESET;

    index_++;
    expectSemicolon();
}

/**
 * @brief Parses a cgi directive and registers an extension/binary pair.
 *
 * @param location_block The LocationConfig to fill
 * @throws std::runtime_error if the extension or binary path is missing
 * @note Parses: cgi .php /usr/bin/php-cgi;
 * @note Can appear multiple times in one location block, each call adds one
 * entry.
 */
void ConfigBuilder::parseCGI(LocationConfig& location_block) {
    index_++;
    checkBounds("after \"cgi\", expected extension + path to binary");

    const Token& ext_token = currentToken();
    std::string cgi_extension = ext_token.value;
    if (cgi_extension.empty() || cgi_extension[0] != '.') {
        configError(ext_token,
                    "cgi extension must start with '.' (ex: \".php\")");
    }
    if (location_block.getCgiInterpreters().find(cgi_extension) !=
        location_block.getCgiInterpreters().end()) {
        configError(ext_token, "duplicate cgi extension");
    }
    index_++;
    checkBounds("after cgi extension (ex:\".php\") expected binary path");
    std::string cgi_binary_path = currentToken().value;

    location_block.addCgiInterpreter(cgi_extension, cgi_binary_path);
    LOG_DEBUG() << "ConfigBuilder: cgi -> " << GRN << cgi_extension << " => "
                << cgi_binary_path << RESET;

    index_++;
    expectSemicolon();
}

/**
 * @brief Parses a return directive and sets the redirect code and target URL.
 *
 * @param location_block The LocationConfig to fill
 * @throws std::runtime_error if the code or URL is missing
 * @note Parses: return 301 /;
 * @note return_code_ defaults to NO_REDIRECT (-1) if this directive is absent.
 * Presence is checked by ConfigValidator.
 */
void ConfigBuilder::parseReturn(LocationConfig& location_block) {
    index_++;
    checkBounds("after \"return\", expected code + path");

    if (location_block.getReturnCode() != LocationConfig::kNoRedirect) {
        configError(currentToken(), "duplicate \"return\" directive");
    }
    int return_code = toInt(currentToken().value);
    index_++;
    checkBounds("after \"return code\", expected path");
    std::string return_path = currentToken().value;

    location_block.setReturnCode(return_code);
    location_block.setReturnUrl(return_path);
    LOG_DEBUG() << "ConfigBuilder: return -> " << GRN << return_code << " "
                << return_path << RESET;

    index_++;
    expectSemicolon();
}