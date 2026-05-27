#include "Request.hpp"

#include <cctype>
#include <iostream>
#include <sstream>
#include <vector>

#include "../logger/Logger.hpp"

/*****************************************************************************
 *                                  REQUEST                                  *
 *****************************************************************************/

/**
 * @brief Default Constructor.
 */
Request::Request()
    : max_uri_size_(HttpConstants::kDefaultMaxURISize),
      max_header_size_(HttpConstants::kDefaultMaxHeaderSize),
      max_body_size_(HttpConstants::kDefaultMaxBodySize),
      complete_(false),
      error_(false),
      error_code_(0),
      allow_empty_start_(true),
      at_start_line_(true),
      at_body_(false) {
}

/**
 * @brief Default Destructor.
 */
Request::~Request() {
}

/**
 * @brief Copy Constructor.
 * @param other Request to copy.
 */
Request::Request(const Request& other) {
    *this = other;
}

/**
 * @brief Request '=' operator overload.
 * @param other Request to copy.
 */
Request& Request::operator=(const Request& other) {
    if (this != &other) {
        raw_ = other.raw_;
        method_ = other.method_;
        target_ = other.target_;
        protocol_ = other.protocol_;
        headers_ = other.headers_;
        body_ = other.body_;

        max_header_size_ = other.max_header_size_;
        max_body_size_ = other.max_body_size_;
        max_uri_size_ = other.max_uri_size_;

        complete_ = other.complete_;
        error_ = other.error_;
        error_code_ = other.error_code_;
        error_message_ = other.error_message_;

        allow_empty_start_ = other.allow_empty_start_;
        at_start_line_ = other.at_start_line_;
        at_body_ = other.at_body_;
    }
    return (*this);
}

/****************************** Parsing Utils *******************************/

static std::string setToLower(std::string& s) {
    for (std::string::iterator s_it = s.begin(); s_it != s.end(); s_it++) {
        s_it[0] = std::tolower(s_it[0]);
    }
    return (s);
}

static std::string trimL(std::string& s, const char* t = " \t\n\r\f\v") {
    s.erase(0, s.find_first_not_of(t));
    return (s);
}

static std::string trimR(std::string& s, const char* t = " \t\n\r\f\v") {
    s.erase(s.find_last_not_of(t) + 1);
    return (s);
}

static std::string trim(std::string& s, const char* t = " \t\n\r\f\v") {
    trimL(s, t);
    return (trimR(s, t));
}

static std::string removeCR(std::string& s) {
    if (!s.empty() && s.at(s.size() - 1) == '\r')
        s.erase(s.size() - 1);
    return (s);
}

static bool findWhitespace(std::string s) {
    for (std::string::iterator s_it = s.begin(); s_it != s.end(); s_it++) {
        if (std::isspace(s_it[0]))
            return (true);
    }
    return (false);
}

static bool isOnlyDigits(std::string s) {
    for (std::string::iterator s_it = s.begin(); s_it != s.end(); s_it++) {
        if (!std::isdigit(s_it[0]))
            return (false);
    }
    return (true);
}

static bool isOnlyHexDigits(std::string s) {
    for (std::string::iterator s_it = s.begin(); s_it != s.end(); s_it++) {
        if (!std::isdigit(s_it[0]) &&
            !(std::tolower(s_it[0]) >= 'a' && std::tolower(s_it[0]) <= 'f'))
            return (false);
    }
    return (true);
}

static std::vector<std::string> listHeaders(const std::string s) {
    std::string val(s), element;
    std::vector<std::string> val_vect;

    while (!val.empty()) {
        size_t comma_pos = val.find(",");
        element = val.substr(0, comma_pos);
        val_vect.push_back(trim(element));
        val.erase(0, comma_pos);
        if (comma_pos != std::string::npos)
            val.erase(0, 1);
    }
    return (val_vect);
}

static bool checkTargetSyntax(std::string target) {
    if (target.empty())
        return (false);
    if (target[0] != '/')
        return (false);
    std::string::iterator s_it = target.begin();
    while (++s_it != target.end()) {
        char c = s_it[0];
        if (c <= 32 || c == 127)
            return (false);
    }
    return (true);
}

/********************************* Getters **********************************/

/**
 * @brief Get Request Method.
 */
std::string Request::getMethod() const {
    return (method_);
}

/**
 * @brief Get Request Target.
 */
std::string Request::getTarget() const {
    return (target_);
}

/**
 * @brief Get Path from Target.
 */
std::string Request::getPath() const {
    return (target_.substr(0, target_.find('?')));
}

/**
 * @brief Get Query from Target if it exists.
 */
std::string Request::getQuery() const {
    size_t query_pos = target_.find('?');
    if (query_pos != std::string::npos)
        return (target_.substr(query_pos + 1));
    return ("");
}

/**
 * @brief Get Request Protocol.
 */
std::string Request::getProtocol() const {
    return (protocol_);
}

/**
 * @brief Get Request Body.
 */
std::string Request::getBody() const {
    return (body_);
}

/**
 * @brief Get Header Value corresponding to key.
 * @param key Header name to look for (case-insensitive).
 */
std::string Request::getHeaderValue(const std::string key) const {
    std::string key_lower(key);
    std::string value_get;
    if (headers_.count(setToLower(key_lower)) > 0) {
        std::map<std::string, std::string>::const_iterator c_it;
        c_it = headers_.find(key_lower);
        value_get = c_it->second;
    }
    return (value_get);
}

/**
 * @brief Get constant reference to Request Headers.
 */
const std::map<std::string, std::string>& Request::getHeaders() const {
    return (headers_);
}

/**
 * @brief Check if request is complete.
 */
bool Request::isComplete() const {
    return (complete_);
}

/**
 * @brief Check if Request returned an error.
 */
bool Request::isError() const {
    return (error_);
}

/**
 * @brief Get error code.
 */
int Request::getErrorCode() const {
    return (error_code_);
}

/**
 * @brief Get error message.
 */
std::string Request::getErrorMessage() const {
    return (error_message_);
}

/**
 * @brief Check if a connection should be kept alive after request.
 */
bool Request::shouldKeepAlive() const {
    if (error_)
        return (false);

    bool keep_alive = (protocol_ == "HTTP/1.1");

    if (headers_.count("connection") > 0) {
        std::map<std::string, std::string>::const_iterator c_it;
        c_it = headers_.find("connection");
        std::string connec_val = c_it->second;
        setToLower(connec_val);

        if (connec_val == "keep-alive")
            keep_alive = true;
        else if (connec_val == "close")
            keep_alive = false;
    }

    if (headers_.count("transfer-encoding") &&
        (headers_.count("content-length") > 0 || protocol_ != "HTTP/1.1"))
        keep_alive = false;

    return (keep_alive);
}

/********************************* Setters **********************************/

/**
 * @brief Append string to raw string, then parse it. Erases parsed sections.
 * @param data String to append to raw string.
 * @param len Number of characters to append.
 */
void Request::append(const char* data, size_t len) {
    if (raw_.size() + len > max_header_size_ + max_body_size_) {
        return (setError(HttpConstants::kBodyTooLarge));
    }
    raw_.append(data, len);
    parseStartLine();
    parseHeaders();
    parseBody();
}

/**
 * @brief Wipe out all data with the exception of the raw message string
 */
void Request::clearData() {
    method_.clear();
    target_.clear();
    protocol_.clear();
    headers_.clear();
    body_.clear();

    complete_ = false;
    error_ = false;
    error_code_ = 0;
    error_message_.clear();

    allow_empty_start_ = true;
    at_start_line_ = true;
    at_body_ = false;
}

/**
 * @brief Reset data except for raw, then parse raw
 */
void Request::resetData() {
    clearData();
    parseStartLine();
    parseHeaders();
    parseBody();
}

void Request::setMaxURISize(size_t max_uri_size) {
    max_uri_size_ = max_uri_size;
}

void Request::setMaxHeaderSize(size_t max_header_size) {
    max_header_size_ = max_header_size;
}

void Request::setMaxBodySize(size_t max_body_size) {
    max_body_size_ = max_body_size;
}

/**
 * @brief Sets error to true along with error message.
 * @param int Error code to set error_code_ to.
 * @param message Error message to set error_message_ to.
 */
void Request::setError(HttpConstants::HttpError http_error) {
    error_ = true;
    error_code_ = http_error.code;
    error_message_ = http_error.reason;
    complete_ = true;
    LOG_WARNING() << "[Request] error: " << RED << error_code_ << " "
                  << error_message_ << RESET;
}

/**
 * @brief Sets complete_ to true
 */
void Request::setComplete() {
    complete_ = true;
}

/********************************* Parsing **********************************/

/**
 * @brief Parse the start line.
 */
void Request::parseStartLine() {
    if (complete_)
        return;

    while (at_start_line_ && raw_.find('\n') != std::string::npos) {
        std::string line = raw_.substr(0, raw_.find('\n'));
        raw_.erase(0, raw_.find('\n') + 1);
        removeCR(line);

        if (line.empty()) {
            /*Allow one empty line before start*/
            if (allow_empty_start_)
                allow_empty_start_ = false;
            else
                return (setError(HttpConstants::kBadRequest));
        } else {
            /*Parsing request start line*/
            std::istringstream line_stream(line);
            std::string garbage;
            line_stream >> method_ >> target_ >> protocol_ >> garbage;

            /*Check for missing or malformed tokens*/
            if (method_.empty() || target_.empty() || protocol_.empty() ||
                !garbage.empty())
                return (setError(HttpConstants::kBadRequest));
            if (!checkTargetSyntax(target_))
                return (setError(HttpConstants::kBadRequest));
            if (target_.size() > max_uri_size_)
                return (setError(HttpConstants::kURITooLong));
            if (protocol_ != "HTTP/1.1" && protocol_ != "HTTP/1.0")
                return (setError(HttpConstants::kVersionNotSupported));

            at_start_line_ = false;
        }
    }
}

/**
 * @brief Parse the header fields.
 */
void Request::parseHeaders() {
    if (complete_ || at_start_line_)
        return;

    while (!at_body_ && raw_.find('\n') != std::string::npos) {
        std::string line = raw_.substr(0, raw_.find('\n'));
        raw_.erase(0, raw_.find('\n') + 1);
        removeCR(line);

        if (line.empty()) {
            if (headers_.count("host") > 0) {
                if (listHeaders(headers_["host"]).size() > 1)
                    return (setError(HttpConstants::kBadRequest));
            } else if (protocol_ == "HTTP/1.1")
                return (setError(HttpConstants::kBadRequest));
            at_body_ = true;
            return;
        }

        if (line.find(':') == std::string::npos)
            return (setError(HttpConstants::kBadRequest));
        std::string name = line.substr(0, line.find(':'));
        if (findWhitespace(name))
            return (setError(HttpConstants::kBadRequest));
        setToLower(name);

        std::string value = line.substr(line.find(':') + 1);
        trim(value);
        /*If header already exists, append value in comma-separated list*/
        if (headers_.count(name) > 0 && !headers_[name].empty())
            value = headers_[name] + ", " + value;
        if (value.size() > max_header_size_)
            return (setError(HttpConstants::kHeaderTooLarge));
        headers_[name] = value;
    }
}

/**
 * @brief Parse the body.
 */
void Request::parseBody() {
    if (complete_ || at_start_line_ || !at_body_)
        return;

    /*Check if a header indicates a body exists*/
    if (headers_.count("transfer-encoding") > 0) {
        std::vector<std::string> encoding_list;
        encoding_list = listHeaders(headers_["transfer-encoding"]);
        if (!encoding_list.empty() && encoding_list.back() == "chunked")
            parseBodyChunked();
        else
            return (setError(HttpConstants::kBadRequest));
    } else if (headers_.count("content-length") > 0)
        parseBodyContentLen(headers_["content-length"]);
    else {
        LOG_INFO() << BR_CYN
            "[Request] fully parsed without message body" RESET;
        return (setComplete());
    }
}

/**
 * @brief Parse body using Content-Length.
 * @param len value in Content-Length header.
 */
void Request::parseBodyContentLen(std::string len) {
    if (!isOnlyDigits(len))
        return (setError(HttpConstants::kBadRequest));

    size_t len_value = 0;
    std::istringstream len_stream(len);

    len_stream >> len_value;
    if (len_stream.fail())
        return (setError(HttpConstants::kBadRequest));
    if (len_value > max_body_size_)
        return (setError(HttpConstants::kBodyTooLarge));

    /*Appending raw content to body*/
    size_t len_take = len_value - body_.size();
    if (raw_.size() < len_take)
        len_take = raw_.size();
    if (len_take > 0) {
        body_ += raw_.substr(0, len_take);
        raw_.erase(0, len_take);
    }
    if (body_.size() != len_value)
        return;  // Reached end of stream - incomplete body
    LOG_INFO() << BR_CYN
        "[Request] fully parsed "
        "using Content-Length for message body" RESET;
    return (setComplete());
}

/**
 * @brief Parse body using Transfer-Encoding chunked method.
 */
void Request::parseBodyChunked() {
    while (raw_.find("\r\n") != std::string::npos) {
        std::string size_line = raw_.substr(0, raw_.find("\r\n"));
        removeCR(size_line);

        if (!isOnlyHexDigits(size_line))
            return (setError(HttpConstants::kBadRequest));

        size_t len_value = 0;
        std::istringstream len_stream(size_line);

        /*Convert hexadecimal chunk size to size_t*/
        len_stream >> std::hex >> len_value;
        if (len_stream.fail())
            return (setError(HttpConstants::kBadRequest));
        if (len_value > max_body_size_ - body_.size())
            return (setError(HttpConstants::kBodyTooLarge));

        /*Check if chunk properly ends in CRLF*/
        size_t chunk_end = raw_.find("\r\n") + len_value + 2;
        if (raw_.size() <= chunk_end)
            return;  // chunk too small - incomplete
        if (raw_.size() - chunk_end == 1 && raw_[chunk_end] == '\r')
            return;  // raw ends in "\r" - incomplete
        if (raw_.compare(chunk_end, 2, "\r\n") != 0)
            return (setError(HttpConstants::kBadRequest));

        raw_.erase(0, raw_.find("\r\n") + 2);

        /*Append chunk to body*/
        std::string chunk_data = raw_.substr(0, len_value);
        body_ += chunk_data;

        /*Erase parsed chunk and CRF*/
        raw_.erase(0, len_value + 2);

        if (len_value == 0) {
            /*Reached null chunk - body parsing finished*/
            LOG_INFO() << BR_CYN
                "[Request] fully parsed "
                "with chunked encoding for message body" RESET;
            return (setComplete());
        }
    }
}
