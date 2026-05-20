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

void Response::reset() {
    raw_.clear();
}

// what Pauline needs for handler:
// void setStatus(int code, const std::string& reason);
// void setHeader(const std::string& key, const std::string& value);
// void setBody(const std::string& body);

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
    status_ = code_stream.str();
    updateRaw();
}

void Response::setStatus(HttpConstants::HttpError error) {
    setStatus(error.code, error.reason);
}

void Response::setHeader(const std::string& key, const std::string& value) {
    std::string key_lower(key);
    setToLower(key_lower);
    if (key_lower == "set-cookie") {
        cookies_.push_back(value);
    }
    else if (headers_.count(key_lower) == 0)
        headers_[key_lower] = value;
    else {
        std::string new_val = headers_[key_lower] + ", " + value;
        headers_[key_lower] = new_val;
    }
    updateRaw();
}

void Response::setBody(const std::string& body) {
    body_ = body;
    updateRaw();
}

void Response::updateRaw() {
    std::string raw_new;
    raw_new += status_ + "\r\n";
    std::map<std::string, std::string>::const_iterator h_it, h_ite;
    h_ite = headers_.end();
    for (h_it = headers_.begin(); h_it != h_ite; h_it++) {
        raw_new += h_it->first + ": " + h_it->second + "\r\n";
    }
    std::vector<std::string>::const_iterator c_it, c_ite;
    c_ite = cookies_.end();
    for (c_it = cookies_.begin(); c_it != c_ite; c_it++) {
        raw_new += "Set-Cookie: " + c_it[0] + "\r\n";
    }
    raw_new += "\r\n" + body_;
    raw_ = raw_new;
}
