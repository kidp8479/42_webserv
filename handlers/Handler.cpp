#include "Handler.hpp"

/**
 * @brief Stub: returns static hello world response.
 * @note Will be replaced by Pauline's full handler implementation.
 */
void Handler::run(const Request& request, const LocationConfig& location,
                  const ServerConfig& server, Response& response) {
    // this is called "aggregate initialization" - init members in the order of
    // declaration
    HandlerContext handler_context = {request, location, server, response};

    // if request parsing has marked isError true, return corresponding error
    // code
    if (request.isError()) {
        sendError(request.getErrorCode(), handler_context);
        return;
    }

    // stub: same hello world as before, keeps server testable
    response.setRaw(
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 11\r\n"
        "\r\n"
        "Hello World");
}
