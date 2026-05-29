#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <sstream>
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
 */
class Response {
public:
    Response();
    ~Response();
    Response(const Response& other);
    Response& operator=(const Response& other);

    void buildError(int code, const std::string& reason);
    void buildError(HttpConstants::HttpError error);

    const std::string& getRaw() const;
    std::vector<std::string> getCookieList() const;

    void reset();

    void setStatus(int code, const std::string& reason);
    void setStatus(HttpConstants::HttpError error);
    void setHeader(const std::string& key, const std::string& value);
    void setBody(const std::string& body);

    void setRaw(const std::string& raw);

private:
    std::string raw_;
    std::string status_;
    std::string headers_;
    std::string body_;

    std::vector<std::string> cookies_;

    void updateRaw();
    void addToCookie(std::string cookie_str);
};

#endif