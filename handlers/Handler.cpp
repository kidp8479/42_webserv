#include "Handler.hpp"

#include "../logger/Logger.hpp"

/**
 * @brief Stub: returns static hello world response.
 * @note Will be replaced by Pauline's full handler implementation.
 */
void Handler::run(const Request& request, const LocationConfig& location,
                  const ServerConfig& server, Response& response) {
    LOG_INFO() << BR_CYN "[Handler] " << request.getMethod() << " "
               << request.getTarget() << RESET;
    HandlerContext handler_context = {request, location, server, response};

    // if request parsing has marked isError true, sendError will either look
    // for an existing error page on the disk or if not, hardcode a minimal http
    // page via setRaw
    // then sendError returns, run returns and we are back into Client
    if (request.isError()) {
        sendError(request.getErrorCode(), request.getErrorMessage(),
                  handler_context);
        return;
    }

    // first - check for 501 error
    const std::string& request_method = request.getMethod();
    if (request_method != "GET" && request_method != "POST" &&
        request_method != "DELETE") {
        LOG_WARNING() << "[Handler] 501 - method not implemented: "
                      << request_method;
        sendError(HttpConstants::kNotImplemented, handler_context);
        return;
    }

    // second - check for 405 error, look for request_method inside
    // allowed_method vector
    const std::vector<std::string> allowed_method = location.getMethods();
    std::vector<std::string>::const_iterator it;
    if (std::find(allowed_method.begin(), allowed_method.end(),
                  request_method) == allowed_method.end()) {
        LOG_WARNING() << "[Handler] 405 - method not allowed: "
                      << request_method;
        sendError(HttpConstants::kMethodNotAllowed, handler_context);
        return;
    }

    // third - dispatcher for right private method

    // stub: same hello world as before, keeps server testable
    response.setRaw(
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 11\r\n"
        "\r\n"
        "Hello World");
}

void Handler::handleReturn(HandlerContext& handler_context) {
    (void)handler_context;
}
void Handler::handleCgiInterpreters(HandlerContext& handler_context) {
    (void)handler_context;
}
void Handler::handleUpload(HandlerContext& handler_context) {
    (void)handler_context;
}
void Handler::handleStatic(HandlerContext& handler_context) {
    (void)handler_context;
}

void Handler::sendError(HttpConstants::HttpError error,
                        HandlerContext& handler_context) {
    sendError(error.code, error.reason, handler_context);
}

void Handler::sendError(int code, const std::string& reason,
                        HandlerContext& handler_context) {
    LOG_DEBUG() << "[Handler] sending error " << GRN << code << " " << reason
                << RESET;
    // to add : look for existing error pages on disk
    // return it
    // it it does not exists : minimal hardcoded html response

    std::string body =
        "<html><body><h1>" + toString(code) + reason + "</h1></body></html>";
    std::string response =
        "HTTP/1.1 " + toString(code) + " " + reason + "\r\n" +
        "Content-Type: text/html\r\n" +
        "Content-Length: " + toString(static_cast<int>(body.size())) + "\r\n" +
        "\r\n" + body;
    handler_context.response.setRaw(response);
}

std::string Handler::toString(int code) {
    std::ostringstream oss;
    oss << code;
    std::string converted_code = oss.str();

    return converted_code;
}