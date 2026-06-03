#include "Response.hpp"

#include <cctype>
#include <sstream>
#include <vector>

/*****************************************************************************
 *                                  RESPONSE                                 *
 *****************************************************************************/

/**
 * @brief Default Constructor.
 */
Response::Response() : has_cookies_(false) {
}

/**
 * @brief Default Destructor.
 */
Response::~Response() {
}

/**
 * @brief Copy Constructor.
 * @param other Response to copy.
 */
Response::Response(const Response& other) {
    *this = other;
}

/**
 * @brief Response '=' operator overload.
 * @param other Response to copy.
 */
Response& Response::operator=(const Response& other) {
    if (this != &other) {
        raw_ = other.raw_;
        status_ = other.status_;
        headers_ = other.headers_;
        body_ = other.body_;
        has_cookies_ = other.has_cookies_;
        cookie_jar_ = other.cookie_jar_;
    }
    return (*this);
}

/********************************** Utils ***********************************/

static std::string setToLower(std::string s) {
    for (std::string::iterator s_it = s.begin(); s_it != s.end(); s_it++) {
        s_it[0] = static_cast<char>(
            std::tolower(static_cast<unsigned char>(s_it[0])));
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

static std::vector<std::string> listCookie(const std::string s) {
    std::string val(s), element;
    std::vector<std::string> val_vect;

    while (!val.empty()) {
        size_t comma_pos = val.find(";");
        element = val.substr(0, comma_pos);
        val_vect.push_back(trim(element));
        val.erase(0, comma_pos);
        if (comma_pos != std::string::npos)
            val.erase(0, 1);
    }
    return (val_vect);
}

/********************************* Builders *********************************/

/**
 * @brief Generate error page response from error code and error reason
 */
void Response::buildError(int code, const std::string& reason) {
    std::ostringstream status_stream, header_stream, body_stream;

    status_stream << "HTTP/1.1 " << code << " " << reason << "\r\n";
    status_ = status_stream.str();
    body_stream << "<html><body><h1>" << code << " " << reason
                << "</h1></body></html>";
    body_ = body_stream.str();
    header_stream << "Content-Length: " << body_.size() << "\r\n";
    headers_ = header_stream.str();

    updateRaw();
}

/**
 * @brief Convenience overload to generate error page from HttpError struct.
 */
void Response::buildError(HttpConstants::HttpError error) {
    buildError(error.code, error.reason);
}

/********************************* Getters **********************************/

/**
 * @brief Retrieves the raw HTTP response string.
 *
 * Provides the fully constructed HTTP response ready to be sent over
 * the network.
 *
 * @return Constant reference to the internal raw HTTP response buffer
 */
const std::string& Response::getRaw() const {
    return (raw_);
}

/**
 * @brief Check if response contains valid Set-Cookie values.
 * @return True if cookies exist, false otherwise
 */
bool Response::hasCookies() const {
    return (has_cookies_);
}

/**
 * @brief Creates a copy of the cookie map.
 * @return Copy of cookie map
 */
const std::map<std::string, std::string>& Response::getCookieList() const {
    return (cookie_jar_);
}

/********************************* Setters **********************************/

/**
 * @brief Wipes all data from response
 */
void Response::reset() {
    raw_.clear();
    status_.clear();
    headers_.clear();
    body_.clear();
    header_map_.clear();
    has_cookies_ = false;
    cookie_jar_.clear();
}

/**
 * @brief Overrides raw with string passed as argument.
 */
void Response::setRaw(const std::string& raw) {
    raw_ = raw;
}

/**
 * @brief Sets the response's status line and updates raw.
 */
void Response::setStatus(int code, const std::string& reason) {
    std::ostringstream code_stream;
    code_stream << "HTTP/1.1" << " " << code;
    if (!reason.empty())
        code_stream << " " << reason;
    code_stream << "\r\n";
    status_ = code_stream.str();
    updateRaw();
}

/**
 * @brief Convenience overload to set status using an HttpError struct.
 */
void Response::setStatus(HttpConstants::HttpError error) {
    setStatus(error.code, error.reason);
}

/**
 * @brief Adds a header value to response's header string and updates raw.
 */
void Response::setHeader(const std::string& key, const std::string& value) {
    std::string low_key = setToLower(key);
    if (low_key == "set-cookie")
        addToCookie(value);
    else {
        bool key_match = false;
        std::map<std::string, std::string>::iterator h_it;
        h_it = header_map_.begin();
        while (!key_match && h_it != header_map_.end()) {
            if (setToLower(h_it->first) == low_key) {
                h_it->second = value;
                key_match = true;
            }
            h_it++;
        }
        if (!key_match)
            header_map_[key] = value;
    }

    headers_.clear();
    std::map<std::string, std::string>::iterator h_it;
    for (h_it = header_map_.begin(); h_it != header_map_.end(); h_it++) {
        headers_ += h_it->first + ": " + h_it->second + "\r\n";
    }
    std::map<std::string, std::string>::iterator c_it;
    for (c_it = cookie_jar_.begin(); c_it != cookie_jar_.end(); c_it++) {
        headers_ += "Set-Cookie: ";
        headers_ += c_it->first + "=" + c_it->second + "\r\n";
    }
    updateRaw();
}

/**
 * @brief Sets the response's body and updates raw.
 */
void Response::setBody(const std::string& body) {
    body_ = body;
    updateRaw();
}

/**
 * @brief Updates raw based on status, headers, and body
 */
void Response::updateRaw() {
    raw_ = status_ + headers_ + "\r\n" + body_;
}

/**
 * @brief Parses a set-cookie string for a valid "name=value"
 */
void Response::addToCookie(std::string cookie_str) {
    std::vector<std::string> cookie_list = listCookie(cookie_str);
    if (cookie_list.empty() || cookie_list[0].empty())
        return;
    size_t equals_pos = cookie_list[0].find("=");
    if (equals_pos == std::string::npos)
        return;
    std::string name = cookie_list[0].substr(0, equals_pos);
    std::string value = cookie_list[0].substr(equals_pos + 1);
    trim(name);
    trim(value);
    if (name.empty())
        return;

    cookie_jar_[name] = value;
    has_cookies_ = true;
}
