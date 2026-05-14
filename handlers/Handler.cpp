#include "Handler.hpp"

// yes you are allowed to do that :D this namespace is used to have access to a
// map of mime types needed to fill Content-Type info for Response, used as
// constants in the handler. In cpp98 you can't init a map with {"data:data"}
// hence the function
namespace {
const size_t kReadBufferSize = 4096;
// default fallback when mime type is unknown
const std::string kMimeTypeFallback = "application/octet-stream";

std::map<std::string, std::string> initMimeTypes() {
    std::map<std::string, std::string> mime_types;
    mime_types[".html"] = "text/html";
    mime_types[".css"] = "text/css";
    mime_types[".js"] = "application/javascript";
    mime_types[".json"] = "application/json";
    mime_types[".png"] = "image/png";
    mime_types[".jpg"] = "image/jpeg";
    mime_types[".jpeg"] = "image/jpeg";
    mime_types[".gif"] = "image/gif";
    mime_types[".ico"] = "image/x-icon";
    mime_types[".txt"] = "text/plain";
    mime_types[".pdf"] = "application/pdf";
    return mime_types;
}
const std::map<std::string, std::string> kMimeTypes = initMimeTypes();
}  // namespace

/**
 * @brief Entry point for request handling. Runs pre-dispatch checks in order,
 * then delegates to the appropriate handler based on the location block type.
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

    dispatch(handler_context);
}

bool Handler::requestIsError(HandlerContext& handler_context) {
    // first - check for request parsing flagged error
    // if request parsing has marked isError true, sendError will either look
    // for an existing error page on the disk or if not, hardcode a minimal http
    // page via setRaw
    // then sendError returns, run returns, Client sends the error response
    if (handler_context.request.isError()) {
        LOG_WARNING() << "[Handler] " << handler_context.request.getErrorCode()
                      << ": " << handler_context.request.getErrorMessage();
        sendError(handler_context.request.getErrorCode(),
                  handler_context.request.getErrorMessage(), handler_context);
        return true;
    }
    return false;
}

bool Handler::methodNotImplementedCheck(HandlerContext& handler_context) {
    // second - check for 501 error, is it an implemented method ? don't even
    // start to treat if if not
    const std::string& request_method = handler_context.request.getMethod();

    if (request_method != "GET" && request_method != "POST" &&
        request_method != "DELETE") {
        LOG_WARNING() << "[Handler] 501 - method not implemented: "
                      << request_method;
        sendError(HttpConstants::kNotImplemented, handler_context);
        return true;
    }
    return false;
}

bool Handler::methodNotAllowedCheck(HandlerContext& handler_context) {
    // third - check for 405 error, look for request_method inside
    // allowed_method vector, if it's not there, don't start to treat it
    const std::string& request_method = handler_context.request.getMethod();
    const std::vector<std::string> allowed_method =
        handler_context.location.getMethods();

    if (std::find(allowed_method.begin(), allowed_method.end(),
                  request_method) == allowed_method.end()) {
        LOG_WARNING() << "[Handler] 405 - method not allowed: "
                      << request_method;
        sendError(HttpConstants::kMethodNotAllowed, handler_context);
        return true;
    }
    return false;
}

bool Handler::locationBlockDiscriminantCheck(HandlerContext& handler_context) {
    // fourth - count for discriminants, meaning what define the "type" of a
    // location block, if more than 1 is set : too ambiguous to resolve for
    // user, if we don't reject here, we will encounter weird behaviors later
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
        LOG_WARNING() << "[Handler] 500 - internal server error "
                      << "- ambiguous location block: multiple discriminants "
                         "set, cannot resolve properly";
        sendError(HttpConstants::kInternalServerError, handler_context);
        return true;
    }
    return false;
}

void Handler::dispatch(HandlerContext& handler_context) {
    // at this step, it is guaranteed to have only ONE location block type
    // possible, we can now dispatch to the right logical branch path
    if (handler_context.location.getReturnCode() !=
        LocationConfig::kNoRedirect) {
        LOG_DEBUG() << BR_YEL "[Handler] return location block detected"
                    << RESET;
        handleReturn(handler_context);
    } else if (!handler_context.location.getCgiInterpreters().empty()) {
        LOG_DEBUG() << BR_YEL "[Handler] CGI location block detected" << RESET;
        handleCgiInterpreters(handler_context);
    } else if (!handler_context.location.getUploadPath().empty()) {
        LOG_DEBUG() << BR_YEL "[Handler] upload location block detected"
                    << RESET;
        handleUpload(handler_context);
    } else {
        LOG_DEBUG() << BR_YEL
            "[Handler] serve static files location block detected"
                    << RESET;
        handleStatic(handler_context);
    }
}

// TODO: implement redirect with Location header and return code
void Handler::handleReturn(HandlerContext& handler_context) {
    // this is a super minimal response
    // setRaw will be replaced by real Response setters (code/body/header) when
    // available
    handler_context.response.setRaw("HTTP/1.1 301 Moved Permanently\r\n\r\n");
}

// TODO: implement CGI execution (fork/execve)
void Handler::handleCgiInterpreters(HandlerContext& handler_context) {
    // this is a super minimal response
    // setRaw will be replaced by real Response setters (code/body/header) when
    // available
    handler_context.response.setRaw("HTTP/1.1 200 OK\r\n\r\n");
}

// TODO: implement file upload handling
void Handler::handleUpload(HandlerContext& handler_context) {
    // this is a super minimal response
    // setRaw will be replaced by real Response setters (code/body/header) when
    // available
    handler_context.response.setRaw("HTTP/1.1 200 OK\r\n\r\n");
}

// TODO: implement static file serving
void Handler::handleStatic(HandlerContext& handler_context) {
    // build full_path = root + uri
    // stat(full_path) :
    //   if -1 => 404 Not Found
    //   if 0 + S_ISREG => serve the file
    //   if 0 + S_ISDIR :
    //     try full_path + "/" + index
    //     if index exists  serve the file
    //     else if directory_listing on => generate and serve directory listing
    //     else => 403 Forbidden

    const std::string full_path =
        handler_context.location.getRoot() + handler_context.request.getPath();
    LOG_DEBUG() << "[Handler] full path (root + uri) is: " << GRN << full_path
                << RESET;

    struct stat get_info;
    if (stat(full_path.c_str(), &get_info) != 0) {
        LOG_WARNING() << "[Handler] 404 - path not found: " << RED << full_path
                      << RESET;
        sendError(HttpConstants::kNotFound, handler_context);
        return;
    }
    if (S_ISDIR(get_info.st_mode)) {
        std::string path_to_serve =
            full_path + "/" + handler_context.location.getIndex();
        LOG_DEBUG() << "[Handler] directory detected, trying index: " << GRN
                    << path_to_serve << RESET;

        // index not found: fall back to directory listing or 403
        if (stat(path_to_serve.c_str(), &get_info) != 0) {
            if (handler_context.location.getDirectoryListing()) {
                generateDirectoryListing(full_path, handler_context);
            } else {
                LOG_WARNING()
                    << "[Handler] 403 - directory listing off, no index: "
                    << RED << full_path << RESET;
                sendError(HttpConstants::kForbidden, handler_context);
            }
        } else if (S_ISREG(get_info.st_mode)) {
            serveFile(path_to_serve, handler_context);
        } else {
            LOG_WARNING() << "[Handler] 403 - index is not a regular file: "
                          << RED << path_to_serve << RESET;
            sendError(HttpConstants::kForbidden, handler_context);
        }
    } else if (S_ISREG(get_info.st_mode)) {
        LOG_DEBUG() << "[Handler] file detected, serving: " << GRN << full_path
                    << RESET;
        serveFile(full_path, handler_context);
    }
}

void Handler::sendError(HttpConstants::HttpError error,
                        HandlerContext& handler_context) {
    sendError(error.code, error.reason, handler_context);
}

void Handler::sendError(int code, const std::string& reason,
                        HandlerContext& handler_context) {
    LOG_WARNING() << "[Handler] sending error " << code << " " << reason;

    // look for existing error pages on disk and return it
    const std::map<int, std::string>& error_pages =
        handler_context.server.getErrorPages();
    const std::map<int, std::string>::const_iterator it =
        error_pages.find(code);
    if (it != error_pages.end()) {
        struct stat get_info;
        if (stat(it->second.c_str(), &get_info) == 0 &&
            S_ISREG(get_info.st_mode)) {
            LOG_DEBUG() << "[Handler] using custom error page: " << GRN
                        << it->second << RESET;
            serveFile(it->second, handler_context);
            return;
        }
        LOG_DEBUG() << "[Handler] custom error page not found on disk: " << RED
                    << it->second << RESET << " - serving fallback error page";
    }
    // it it does not exists : launch minimal hardcoded html response
    // replace this part with Charlie's Response setter when available
    std::string body = "<html><body><h1>" + toString(code) + " " + reason +
                       "</h1></body></html>";
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

std::string Handler::getFileMimeType(const std::string& path) {
    std::string extension;
    size_t dot_pos = path.rfind('.');
    if (dot_pos != std::string::npos) {
        extension = path.substr(dot_pos);
    }

    std::map<std::string, std::string>::const_iterator it =
        kMimeTypes.find(extension);
    if (it != kMimeTypes.end()) {
        return it->second;
    }
    return kMimeTypeFallback;
}

void Handler::serveFile(const std::string& path,
                        HandlerContext& handler_context) {
    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        LOG_WARNING()
            << "[Handler] 500 - internal server error - could not open file: "
            << RED << path << RESET;
        sendError(HttpConstants::kInternalServerError, handler_context);
        return;
    }

    char buffer[kReadBufferSize];
    std::string body;
    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer, kReadBufferSize)) > 0) {
        body.append(buffer, bytes_read);
    }
    if (bytes_read == -1) {
        close(fd);
        LOG_WARNING()
            << "[Handler] 500 - internal server error - read error on file: "
            << RED << path << RESET;
        sendError(HttpConstants::kInternalServerError, handler_context);
        return;
    }
    close(fd);

    const std::string& mime_type = getFileMimeType(path);
    // replace this part with real setters for Response
    std::string response =
        "HTTP/1.1 " + toString(static_cast<int>(HttpConstants::kOK.code)) +
        " " + HttpConstants::kOK.reason + "\r\n" +
        "Content-Type: " + mime_type + "\r\n" +
        "Content-Length: " + toString(static_cast<int>(body.size())) + "\r\n" +
        "\r\n" + body;
    handler_context.response.setRaw(response);
    LOG_INFO() << BR_CYN "[Handler] " << path << " served successfully"
               << RESET;
}

// generates an HTML page listing the contents of a directory (autoindex)
void Handler::generateDirectoryListing(const std::string& path,
                                       HandlerContext& handler_context) {
    DIR* directory = opendir(path.c_str());
    if (directory == NULL) {
        LOG_WARNING() << "[Handler] 500 - internal server error - could not "
                         "open directory: "
                      << RED << path << RESET;
        sendError(HttpConstants::kInternalServerError, handler_context);
        return;
    }

    // struct dirent is defined in <dirent.h>, d_name contains the entry name
    // (file or directory)
    struct dirent* entry;
    std::string directory_list;
    while ((entry = readdir(directory)) != NULL) {
        std::string name = entry->d_name;
        // we don' want people to access "." or ".."
        if (name == "." || name == "..") {
            continue;
        }
        directory_list += "<li><a href=\"" + name + "\">" + name + "</a></li>";
    }
    closedir(directory);

    std::string body =
        "<html><body><ul>" + directory_list + "</ul></body></html>";
    std::string response =
        "HTTP/1.1 " + toString(static_cast<int>(HttpConstants::kOK.code)) +
        " " + HttpConstants::kOK.reason + "\r\n" +
        "Content-Type: text/html\r\n" +
        "Content-Length: " + toString(static_cast<int>(body.size())) + "\r\n" +
        "\r\n" + body;
    handler_context.response.setRaw(response);
    LOG_INFO() << BR_CYN "[Handler] directory listing of " << path
               << " served successfully" << RESET;
}