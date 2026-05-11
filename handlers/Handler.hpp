#ifndef HANDLER_HPP
#define HANDLER_HPP

#include "../config/LocationConfig.hpp"
#include "../config/ServerConfig.hpp"
#include "../http/Request.hpp"
#include "../http/Response.hpp"

class Handler {
public:
    static void run(const Request& request, const LocationConfig& location,
                    const ServerConfig& server, Response& response);

private:
    Handler();
    ~Handler();
    Handler(const Handler& copy);
    Handler& operator=(const Handler& other);

    static void handleReturn(const Request& request,
                             const LocationConfig& location,
                             const ServerConfig& server, Response& response);
    static void handleCgiInterpreters(const Request& request,
                                      const LocationConfig& location,
                                      const ServerConfig& server,
                                      Response& response);
    static void handleUpload(const Request& request,
                             const LocationConfig& location,
                             const ServerConfig& server, Response& response);
    static void handleStatic(const Request& request,
                             const LocationConfig& location,
                             const ServerConfig& server, Response& response);
    static void sendError(int code, const ServerConfig& server,
                          Response& response);
};

#endif
