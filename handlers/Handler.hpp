#ifndef HANDLER_HPP
#define HANDLER_HPP

#include "../config/LocationConfig.hpp"
#include "../config/ServerConfig.hpp"
#include "../http/Request.hpp"
#include "../http/Response.hpp"
#include "../utils/HttpConstants.hpp"

class Handler {
public:
    static void run(const Request& request, const LocationConfig& location,
                    const ServerConfig& server, Response& response);

private:
    Handler();
    ~Handler();
    Handler(const Handler& copy);
    Handler& operator=(const Handler& other);

    struct HandlerContext {
        const Request& request;
        const LocationConfig& location;
        const ServerConfig& server;
        Response& response;
    };

    static void handleReturn(HandlerContext& handler_context);
    static void handleCgiInterpreters(HandlerContext& handler_context);
    static void handleUpload(HandlerContext& handler_context);
    static void handleStatic(HandlerContext& handler_context);
    static void sendError(int code, HandlerContext& handler_context);
};

#endif
