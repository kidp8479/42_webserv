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

    void buildFrom(const Request& request);

    const std::string& getRaw() const;
	void reset();

private:
    std::string raw_;
};

#endif
