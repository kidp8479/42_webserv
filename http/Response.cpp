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
    (void)request;

    // minimal valid HTTP response
    raw_ =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 11\r\n"
        "\r\n"
        "Hello World";
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
