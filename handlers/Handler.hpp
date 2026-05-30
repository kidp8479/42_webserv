#ifndef HANDLER_HPP
#define HANDLER_HPP

#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <cstdio>
#include <ctime>
#include <map>
#include <sstream>

#include "../config/LocationConfig.hpp"
#include "../config/ServerConfig.hpp"
#include "../http/Request.hpp"
#include "../http/Response.hpp"
#include "../logger/Logger.hpp"
#include "../utils/HttpConstants.hpp"

class Client;
class EventLoop;

class Handler {
public:
    static bool run(const Request& request, const LocationConfig& location,
                    Client& client);
    static void applyCgiResponse(const std::string& raw, Response& response);

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
        EventLoop& loop;
        Client& client;
    };

    // helpers for run()
    static bool requestIsError(HandlerContext& handler_context);
    static bool methodNotImplementedCheck(HandlerContext& handler_context);
    static bool methodNotAllowedCheck(HandlerContext& handler_context);
    static bool locationBlockDiscriminantCheck(HandlerContext& handler_context);
    static bool dispatch(HandlerContext& handler_context);

    // helpers for dispatch()
    static void handleReturn(HandlerContext& handler_context);
    static bool handleCgiInterpreters(HandlerContext& handler_context);
    static void handleUpload(HandlerContext& handler_context);
    static void handleStatic(HandlerContext& handler_context);

    // helpers for handleStatic()
    static void resolveDirectory(const std::string& full_path,
                                 HandlerContext& handler_context);
    static void deleteFile(const std::string& full_path,
                           HandlerContext& handler_context);
    static void serveFile(
        const std::string& path, HandlerContext& handler_context,
        int code = HttpConstants::kOK.code,
        const std::string& reason = HttpConstants::kOK.reason);
    static void generateDirectoryListing(const std::string& path,
                                         HandlerContext& handler_context);

    // error handling
    static void sendError(HttpConstants::HttpError error,
                          HandlerContext& handler_context);
    // overload, need the detailed parameters sometimes depending on context
    static void sendError(int code, const std::string& reason,
                          HandlerContext& handler_context);

    // utils
    static std::string toString(int code);
    static std::string toString(size_t n);
    static std::string getFileMimeType(const std::string& path);

    // cookie utils
    static std::string newSessionID();
    static void handleCookieSession(HandlerContext& handler_context);
};

#endif
