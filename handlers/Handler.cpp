#include "Handler.hpp"

/**
 * @brief Stub: returns static hello world response.
 * @note Will be replaced by Pauline's full handler implementation.
 */
void Handler::run(const Request& request,
                  const LocationConfig& loc,
                  Response& response) {
    (void)request;
    (void)loc;
    // stub: same hello world as before, keeps server testable
    response.setRaw(
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 11\r\n"
        "\r\n"
        "Hello World"
    );
}
