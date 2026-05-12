#ifndef HANDLER_HPP
#define HANDLER_HPP

#include <algorithm>
#include <sstream>

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

    // helpers for run()
    static bool requestIsError(HandlerContext& handler_context);
    static bool methodNotImplementedCheck(HandlerContext& handler_context);
    static bool methodNotAllowedCheck(HandlerContext& handler_context);
    static bool locationBlockDiscriminantCheck(HandlerContext& handler_context);

    // helpers for dispatch()
    static void handleReturn(HandlerContext& handler_context);
    static void handleCgiInterpreters(HandlerContext& handler_context);
    static void handleUpload(HandlerContext& handler_context);
    static void handleStatic(HandlerContext& handler_context);

    // error handling
    static void sendError(HttpConstants::HttpError error,
                          HandlerContext& handler_context);
    // overload, need the detailed parameters sometimes depending on context
    static void sendError(int code, const std::string& reason,
                          HandlerContext& handler_context);

    static std::string toString(int code);
};

#endif
