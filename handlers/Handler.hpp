#ifndef HANDLER_HPP
#define HANDLER_HPP

#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <map>
#include <sstream>

#include "../config/LocationConfig.hpp"
#include "../config/ServerConfig.hpp"
#include "../http/Request.hpp"
#include "../http/Response.hpp"
#include "../logger/Logger.hpp"
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
    static void dispatch(HandlerContext& handler_context);

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

    // utils
    static std::string toString(int code);
    static std::string getFileMimeType(const std::string& path);
    static void serveFile(const std::string& path,
                          HandlerContext& handler_context);
    static void generateDirectoryListing(const std::string& path,
                                         HandlerContext& handler_context);
};

#endif
