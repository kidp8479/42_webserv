#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>

#include "Request.hpp"

/**
 * @brief HTTP response builder and container.
 *
 * Responsible for generating a raw HTTP response string from a parsed request.
 *
 * The response is stored internally as a fully-formed HTTP message and can
 * be retrieved for transmission over the network.
 *
 * @note This is currently a temporary stub implementation used to unblock
 *       server development.
 *       It will be replaced and fully implemented by the HTTP/parsing layer
 *       (Charlie’s part) in a later stage of the project.
 */
class Response {
public:
    Response();

    // what this function should do :
    // read parsed request data (method, path, etc.), decide what to return,
    // construct a valid HTTP response string
    // and store the results in raw_:
    void buildFrom(const Request& request);

    // this function will allow me to get the results
    const std::string& getRaw() const;

private:
    std::string raw_;
};

#endif
