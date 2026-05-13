#include "Response.hpp"

Response::Response() : raw_("") {
}

/**
 * @brief Builds a temporary HTTP response from a request.
 *
 * Generates a minimal valid HTTP/1.1 response used for early server testing.
 * The response is currently static and does not depend on request content.
 *
 * @note This is a placeholder implementation used to unblock server
 *       development and end-to-end socket testing.
 *       It will be replaced by a full HTTP response generator in the
 *       dedicated HTTP layer (Charlie’s part).
 *
 * @param request Parsed HTTP request (currently unused in this stub)
 */
void Response::buildFrom(const Request& request) {
    if (request.isError()) {
        buildError(request.getErrorCode(), request.getErrorMessage());
        return;
    }

    // minimal valid HTTP response
    raw_ =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 11\r\n"
        "\r\n"
        "Hello World";
}

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
    return raw_;
}

void Response::reset() {
    raw_.clear();
}

// handler will need this
void Response::setRaw(const std::string& raw) {
    raw_ = raw;
}
