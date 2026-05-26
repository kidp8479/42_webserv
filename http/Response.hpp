#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <sstream>
#include <string>
#include <vector>

#include "Request.hpp"

/**
 * @brief HTTP response builder and container.
 *
 * Responsible for generating a raw HTTP response string from a parsed request.
 *
 * The response is stored internally as a fully-formed HTTP message and can
 * be retrieved for transmission over the network.
 *
 */
class Response {
public:
    Response();

    void buildError(int code, const std::string& reason);
    void buildError(HttpConstants::HttpError error);

    const std::string& getRaw() const;
    void reset();

    void setStatus(int code, const std::string& reason);
    void setStatus(HttpConstants::HttpError error);
    void setHeader(const std::string& key, const std::string& value);
    void setBody(const std::string& body);

    // obsoleted by the above setters
    void setRaw(const std::string& raw);

private:
    std::string raw_;
    std::string status_;
    std::string headers_;
    std::string body_;

    void updateRaw();
};

#endif