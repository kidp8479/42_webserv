#include "Response.hpp"

#include <sstream>

Response::Response() {
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
    header_stream << "Content-Length: " << body_.size() << "\r\n"
                  << "Connection: close\r\n";
    headers_ = header_stream.str();

    updateRaw();
}

/**
 * @brief Conveniance overload to generate error page from HttpError struct
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

/********************************* Setters **********************************/

/**
 * @brief Wipes all data from response
 */
void Response::reset() {
    raw_.clear();
    status_.clear();
    headers_.clear();
    body_.clear();
}

/**
 * @brief Overrirdes raw with string passed as argument
 */
void Response::setRaw(const std::string& raw) {
    raw_ = raw;
}

/**
 * @brief Sets the reponse's status line and updates raw
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
 * @brief Conveniance overload to set status using an HttpError struct
 */
void Response::setStatus(HttpConstants::HttpError error) {
    setStatus(error.code, error.reason);
}

/**
 * @brief Adds a header value to reponse's header string and updates raw
 */
void Response::setHeader(const std::string& key, const std::string& value) {
    headers_ += key + ": " + value + "\r\n";
    updateRaw();
}

/**
 * @brief Sets the reponse's body and updates raw
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
