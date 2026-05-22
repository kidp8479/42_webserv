#include "Response.hpp"

#include <sstream>

Response::Response() {
}

/********************************** Utils ***********************************/

static std::string setToLower(std::string& s) {
    for (std::string::iterator s_it = s.begin(); s_it != s.end(); s_it++) {
        s_it[0] = std::tolower(s_it[0]);
    }
    return (s);
}

/********************************* Builders *********************************/

void Response::buildError(int code, const std::string& reason) {
    std::ostringstream body;

    body << "<html><body><h1>" << code << " " << reason
         << "</h1></body></html>";

    std::string body_str = body.str();
    std::ostringstream response;
    response << "HTTP/1.1 " << code << " " << reason << "\r\n"
             << "Content-Length: " << body_str.size() << "\r\n"
             << "Connection: close\r\n"
             << "\r\n"
             << body_str;

    raw_ = response.str();
}

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

/********************************* Setters **********************************/

void Response::reset() {
    raw_.clear();
    status_.clear();
    headers_.clear();
    body_.clear();
}

// handler uses this for now to unblock dev time, but this call will be replaced
// by the 3 setters above when ready and can be deleted in the end
void Response::setRaw(const std::string& raw) {
    raw_ = raw;
}

void Response::setStatus(int code, const std::string& reason = "") {
    std::ostringstream code_stream;
    code_stream << "HTTP/1.1" << " " << code;
    if (!reason.empty())
        code_stream << " " << reason;
    code_stream << "\r\n";
    status_ = code_stream.str();
    updateRaw();
}

void Response::setStatus(HttpConstants::HttpError error) {
    setStatus(error.code, error.reason);
}

void Response::setHeader(const std::string& key, const std::string& value) {
    headers_ += key + ": " + value + "\r\n";
    updateRaw();
}

void Response::setBody(const std::string& body) {
    body_ = body;
    updateRaw();
}

void Response::updateRaw() {
    raw_ = status_ + headers_ + "\r\n" + body_;
}
