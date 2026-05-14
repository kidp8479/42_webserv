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

/**
 * @brief Checks if the request was flagged as an error by the parser.
 * @return true if an error was detected and a response has been written
 * @note Catches 400, 413, 414, 431, 505 set by Request before Handler runs.
 */
bool Handler::requestIsError(HandlerContext& handler_context) {
    if (handler_context.request.isError()) {
        LOG_WARNING() << "[Handler] " << handler_context.request.getErrorCode()
                      << ": " << handler_context.request.getErrorMessage();
        sendError(handler_context.request.getErrorCode(),
                  handler_context.request.getErrorMessage(), handler_context);
        return true;
    }
    return false;
}

/**
 * @brief Checks if the request method is one of the three implemented methods.
 * @return true if the method is not GET, POST, or DELETE (501 sent)
 */
bool Handler::methodNotImplementedCheck(HandlerContext& handler_context) {
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

/**
 * @brief Checks if the request method is listed in the location's allowed
 * methods.
 * @return true if the method is not in the allowed list (405 sent)
 */
bool Handler::methodNotAllowedCheck(HandlerContext& handler_context) {
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

/**
 * @brief Checks that at most one discriminant field is set in the location
 * block.
 * @return true if multiple discriminants are set (500 sent)
 * @note Discriminants are returnCode, cgiInterpreters, and uploadPath. Having
 *       more than one set is a .conf writer error: Handler cannot resolve the
 *       ambiguity and returns 500 rather than guessing intent.
 */
bool Handler::locationBlockDiscriminantCheck(HandlerContext& handler_context) {
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

/**
 * @brief Routes to the appropriate handler based on the location block type.
 * @note Called only after all pre-dispatch checks pass, so exactly one
 *       discriminant is guaranteed to be set (or none, defaulting to static).
 */
void Handler::dispatch(HandlerContext& handler_context) {
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

/**
 * @brief Handles redirect location blocks. Sends a redirect response.
 * @note TODO: implement with real Location header using getReturnCode() +
 *       getReturnUrl(). Currently a stub.
 */
void Handler::handleReturn(HandlerContext& handler_context) {
    handler_context.response.setRaw("HTTP/1.1 301 Moved Permanently\r\n\r\n");
}

/**
 * @brief Handles CGI location blocks. Forks and executes the CGI script.
 * @note TODO: implement fork/execve/pipe. Currently a stub.
 */
void Handler::handleCgiInterpreters(HandlerContext& handler_context) {
    handler_context.response.setRaw("HTTP/1.1 200 OK\r\n\r\n");
}

/**
 * @brief Handles upload location blocks. Writes the request body to disk.
 * @note TODO: implement file write to getUploadPath(), return 201 Created.
 *       Currently a stub.
 */
void Handler::handleUpload(HandlerContext& handler_context) {
    handler_context.response.setRaw("HTTP/1.1 200 OK\r\n\r\n");
}

/**
 * @brief Handles static file location blocks. Serves files or directory
 * listings.
 * @note Resolves full_path = root + URI, then dispatches based on what stat()
 *       finds: regular file => serve it, directory => try index then listing,
 *       nothing => 404. Empty index string is handled explicitly to avoid
 *       stat()-ing the directory itself (which would always succeed).
 */
void Handler::handleStatic(HandlerContext& handler_context) {
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
        const std::string& index = handler_context.location.getIndex();
        if (index.empty()) {
            if (handler_context.location.getDirectoryListing()) {
                generateDirectoryListing(full_path, handler_context);
            } else {
                LOG_WARNING()
                    << "[Handler] 403 - directory listing off, no index: "
                    << RED << full_path << RESET;
                sendError(HttpConstants::kForbidden, handler_context);
            }
            return;
        }
        std::string path_to_serve = full_path + "/" + index;
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

/**
 * @brief Convenience overload: unpacks an HttpError struct and delegates.
 * @param error Struct containing the HTTP error code and reason string
 */
void Handler::sendError(HttpConstants::HttpError error,
                        HandlerContext& handler_context) {
    sendError(error.code, error.reason, handler_context);
}

/**
 * @brief Writes an error response. Looks up a custom error page first,
 *        falls back to a hardcoded minimal HTML page if none is found.
 * @param code    HTTP status code
 * @param reason  HTTP reason phrase
 * @note Does not throw. Writes directly into response and returns so Client
 *       can send it normally, equivalent to configError() in ConfigBuilder
 *       but without the exception.
 */
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

/**
 * @brief Returns the MIME type string for a given file path based on its
 * extension.
 * @param path File path (only the extension is used)
 * @return MIME type string, or kMimeTypeFallback if the extension is unknown
 */
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

/**
 * @brief Opens, reads, and serves a regular file as an HTTP 200 response.
 * @param path Absolute or relative path to the file to serve
 * @note Detects Content-Type from the file extension via getFileMimeType().
 *       Sends 500 if the file cannot be opened or read.
 */
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

/**
 * @brief Generates and serves an HTML page listing the contents of a directory.
 * @param path Path to the directory to list
 * @note Filters out "." and ".." entries. Sends 500 if opendir() fails.
 */
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