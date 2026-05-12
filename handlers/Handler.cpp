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

    if (requestIsError(handler_context)) {
        return;
    }
    if (methodNotImplementedCheck(handler_context)) {
        return;
    }
    if (methodNotAllowedCheck(handler_context)) {
        return;
    }
    if (locationBlockDiscriminantCheck(handler_context)) {
        return;
    }

    disptach(handler_context);

    // stub: same hello world as before, keeps server testable
    response.setRaw(
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 11\r\n"
        "\r\n"
        "Hello World");
}

bool Handler::requestIsError(HandlerContext& handler_context) {
    // first - check for reauest parsing flagged error
    // if request parsing has marked isError true, sendError will either look
    // for an existing error page on the disk or if not, hardcode a minimal http
    // page via setRaw
    // then sendError returns, run returns and we are back into Client

    if (handler_context.request.isError()) {
        sendError(handler_context.request.getErrorCode(),
                  handler_context.request.getErrorMessage(), handler_context);

        LOG_WARNING() << "[Handler] " << handler_context.request.getErrorCode()
                      << ": " << handler_context.request.getErrorMessage();
        return true;
    }
    return false;
}

bool Handler::methodNotImplementedCheck(HandlerContext& handler_context) {
    // second - check for 501 error,
    const std::string& request_method = handler_context.request.getMethod();

    if (request_method != "GET" && request_method != "POST" &&
        request_method != "DELETE") {
        sendError(HttpConstants::kNotImplemented, handler_context);

        LOG_WARNING() << "[Handler] 501 - method not implemented: "
                      << request_method;
        return true;
    }
    return false;
}

bool Handler::methodNotAllowedCheck(HandlerContext& handler_context) {
    // third - check for 405 error, look for request_method inside
    // allowed_method vector
    const std::string& request_method = handler_context.request.getMethod();
    const std::vector<std::string> allowed_method =
        handler_context.location.getMethods();

    if (std::find(allowed_method.begin(), allowed_method.end(),
                  request_method) == allowed_method.end()) {
        sendError(HttpConstants::kMethodNotAllowed, handler_context);

        LOG_WARNING() << "[Handler] 405 - method not allowed: "
                      << request_method;
        return true;
    }
    return false;
}

bool Handler::locationBlockDiscriminantCheck(HandlerContext& handler_context) {
    // fourth - count for discriminant, meaning what define the "type" of a
    // location block, if more than 1 is set : too ambiguous to resolve for user
    size_t count_discriminant = 0;
    if (handler_context.location.getReturnCode() !=
        LocationConfig::kNoRedirect) {
        count_discriminant++;
    }
    if (!handler_context.location.getCgiInterpreters().empty()) {
        count_discriminant++;
    }
    if (!handler_context.location.getUploadPath().empty()) {
        count_discriminant++;
    }

    if (count_discriminant > 1) {
        sendError(HttpConstants::kInternalServerError, handler_context);

        LOG_WARNING() << "[Handler] 500 - internal server error "
                      << "- ambiguous location block: multiple discriminants "
                         "set, cannot resolve properly";
        return true;
    }
    return false;
}

void Handler::disptach(HandlerContext& handler_context) {
    // at this step, it is guaranteed to have only ONE location block type
    // possible we can now dispatch to the right path
    if (handler_context.location.getReturnCode() !=
        LocationConfig::kNoRedirect) {
        handleReturn(handler_context);
        LOG_DEBUG() << "[Handler] - return location block detected";
    } else if (!handler_context.location.getCgiInterpreters().empty()) {
        handleCgiInterpreters(handler_context);
        LOG_DEBUG() << "[Handler] - CGI location block detected";
    } else if (!handler_context.location.getUploadPath().empty()) {
        handleUpload(handler_context);
        LOG_DEBUG() << "[Handler] - upload location block detected";
    } else {
        handleStatic(handler_context);
        LOG_DEBUG() << "[Handler] - serve static files location block detected";
    }
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