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
 * @note This is currently a temporary stub implementation used to unblock
 *       server development.
 *       It will be replaced and fully implemented by the HTTP/parsing layer
 *       (Charlie’s part) in a later stage of the project.
 */
class Response {
public:
    Response();

    void buildFrom(const Request& request);
    void buildError(int code, const std::string& reason);
    void buildError(HttpConstants::HttpError error);

    const std::string& getRaw() const;
    void reset();

    // what Pauline needs for the handler:
    // void setStatus(int code, const std::string& reason);
    // void setHeader(const std::string& key, const std::string& value);
    // void setBody(const std::string& body);
    void setStatus(int code, const std::string& reason = "");
    void setStatus(HttpConstants::HttpError error);
    void setHeader(const std::string& key, const std::string& value);
    void setBody(const std::string& body);

    // will be obsolete when the 3 above methods are implemented, could be
    // deleted in the end
    void setRaw(const std::string& raw);

private:
    std::string raw_;
    // + whatever members you need to store the infos for the above methods
    // needed
    std::string status_;
    std::string headers_;
    std::string body_;

    void updateRaw();
};

#endif