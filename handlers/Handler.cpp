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
 * @brief Handles an HTTP request through validation and dispatch.
 * Runs pre-dispatch checks (error, method, routing). If all pass,
 * delegates processing to the appropriate handler.
 * HEAD requests reuse GET response but strip the body after dispatch.
 * @return true if a response is ready to be sent, false otherwise.
 */
bool Handler::run(const Request& request, const LocationConfig& location,
                  Client& client) {
    LOG_INFO() << BR_CYN "[Handler] method: " << request.getMethod()
               << " -  target: " << request.getTarget() << RESET;

    HandlerContext handler_context = {request, location, client};

    if (requestIsError(handler_context)) {
        return true;
    }
    if (methodNotImplementedCheck(handler_context)) {
        return true;
    }
    if (methodNotAllowedCheck(handler_context)) {
        return true;
    }
    if (locationBlockDiscriminantCheck(handler_context)) {
        return true;
    }

    bool response_ready = dispatch(handler_context);

    // HEAD: same response as GET but no body, Content-Length stays untouched.
    // String manipulation on raw response is temporary, replace with
    // response.setBody("") once Charlie's API lands.
    if (response_ready && handler_context.request.getMethod() == "HEAD") {
        const std::string& raw = handler_context.client.getResponse().getRaw();
        size_t header_end = raw.find("\r\n\r\n");
        if (header_end != std::string::npos) {
            handler_context.client.getResponse().setRaw(raw.substr(0, header_end + 4));
        }
    }
	return response_ready;
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
 * @brief Checks if the request method is one of the implemented methods.
 * @return true if the method is not GET, POST, DELETE, or HEAD (501 sent)
 */
bool Handler::methodNotImplementedCheck(HandlerContext& handler_context) {
    const std::string& request_method = handler_context.request.getMethod();

    if (request_method != "GET" && request_method != "POST" &&
        request_method != "DELETE" && request_method != "HEAD") {
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
 * @note HEAD is not listed in config files. We follow nginx: if GET is allowed
 *       on this location, HEAD is too. So we just check GET on HEAD's behalf.
 */
bool Handler::methodNotAllowedCheck(HandlerContext& handler_context) {
    const std::string& request_method = handler_context.request.getMethod();
    const std::vector<std::string>& allowed_method =
        handler_context.location.getMethods();

    // redirect blocks have no methods directive, any method is allowed
    if (handler_context.location.getReturnCode() !=
        LocationConfig::kNoRedirect) {
        return false;
    }

    // HEAD works wherever GET works, check GET instead
    const std::string& check_method =
        (request_method == "HEAD") ? std::string("GET") : request_method;

    if (std::find(allowed_method.begin(), allowed_method.end(), check_method) ==
        allowed_method.end()) {
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
 * @brief Dispatches request to the correct handler based on location config.
 * Routing priority:
 * - Return/redirect rule
 * - CGI interpreter handling
 * - Upload handling
 * - Static file serving (default)
 * @return true if response handling completed or is in progress.
 */
bool Handler::dispatch(HandlerContext& handler_context) {
    if (handler_context.location.getReturnCode() !=
        LocationConfig::kNoRedirect) {
        LOG_DEBUG() << BR_YEL "[Handler] return location block detected"
                    << RESET;
        handleReturn(handler_context);
		return true;
    } else if (!handler_context.location.getCgiInterpreters().empty()) {
        LOG_DEBUG() << BR_YEL "[Handler] CGI location block detected" << RESET;
        return handleCgiInterpreters(handler_context);
    } else if (!handler_context.location.getUploadPath().empty()) {
        LOG_DEBUG() << BR_YEL "[Handler] upload location block detected"
                    << RESET;
        handleUpload(handler_context);
		return true;
    } else {
        LOG_DEBUG() << BR_YEL
            "[Handler] serve static files location block detected"
                    << RESET;
        handleStatic(handler_context);
		return true;
    }
}

/**
 * @brief Handles redirect location blocks. Sends a redirect response.
 * @note Builds a minimal response: status line + Location header + empty body.
 *       The server's job stops here. The client (browser, curl -L) reads the
 *       3xx code, picks up the Location header, and fires a new request on its
 *       own. ConfigValidator guarantees code is in [300-399].
 */
void Handler::handleReturn(HandlerContext& handler_context) {
    int return_code = handler_context.location.getReturnCode();
    const std::string& return_url = handler_context.location.getReturnUrl();

    std::string reason;
    if (return_code == HttpConstants::kMovedPermanently.code) {
        reason = HttpConstants::kMovedPermanently.reason;
    } else if (return_code == HttpConstants::kFound.code) {
        reason = HttpConstants::kFound.reason;
    } else if (return_code == HttpConstants::kTemporaryRedirect.code) {
        reason = HttpConstants::kTemporaryRedirect.reason;
    } else if (return_code == HttpConstants::kPermanentRedirect.code) {
        reason = HttpConstants::kPermanentRedirect.reason;
    } else {
        reason = "Redirect";
    }

    // replace setRaw() with Charlie's setStatus/setHeader/setBody when
    // available
    std::string response = "HTTP/1.1 " + toString(return_code) + " " + reason +
                           "\r\n" + "Location: " + return_url + "\r\n" +
                           "Content-Length: 0\r\n" + "\r\n";
    handler_context.client.getResponse().setRaw(response);
    LOG_INFO() << BR_CYN "[Handler] redirect " << return_code << " -> "
               << return_url << RESET;
}

/**
 * @brief Handles CGI location blocks.
 * handleCgiInterpreters: forks, registers CgiProcess, returns false
 * Handler's job ends here — CgiProcess takes over
 */
bool Handler::handleCgiInterpreters(HandlerContext& handler_context) {
    // TODO: implement fork/execve/pipe
    sendError(HttpConstants::kNotImplemented, handler_context);
}

/**
 * @brief Handles upload location blocks. Writes the request body to disk.
 * @note Filename extracted from URI if present, falls back to
 *       "uploaded_file_<timestamp>" if the URI has no filename part.
 *       Returns 500 if the file cannot be created or written.
 */
void Handler::handleUpload(HandlerContext& handler_context) {
    const std::string& upload_path = handler_context.location.getUploadPath();
    const std::string& path = handler_context.request.getPath();
    const std::string& body = handler_context.request.getBody();
    std::string file_name;
    std::string full_upload_path;
    size_t last_slash = path.rfind('/');

    // if the path is empty or if it had no '/' ad a delimiter or if the last
    // '/' is at the end of the name, construct a fallback name "uploaded_file_"
    // + timestamp of creation so that the name stays unique
    if (last_slash == std::string::npos || path.empty() ||
        last_slash + 1 >= path.size()) {
        file_name =
            "uploaded_file_" + toString(static_cast<size_t>(time(NULL)));
    }
    // if there is a valid name to extract, construct normally
    else {
        file_name = path.substr(last_slash + 1);
    }

    full_upload_path = upload_path + "/" + file_name;
    LOG_DEBUG() << "[Handler] built full_upload_path is " << GRN
                << full_upload_path << RESET;

    // open in create mode with right permission to write
    int open_fd =
        open(full_upload_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (open_fd < 0) {
        LOG_WARNING()
            << "[Handler] 500 - internal server error - could not create file: "
            << RED << full_upload_path << RESET;
        sendError(HttpConstants::kInternalServerError, handler_context);
        return;
    }

    int bytes_written = write(open_fd, body.c_str(), body.size());
    if (bytes_written < 0 || static_cast<size_t>(bytes_written) !=
                                 handler_context.request.getBody().size()) {
        close(open_fd);
        LOG_WARNING() << "[Handler] 500 - internal server error - could not "
                         "write to file: "
                      << RED << full_upload_path << RESET;
        sendError(HttpConstants::kInternalServerError, handler_context);
        return;
    }
    close(open_fd);
    std::string response = "HTTP/1.1 " +
                           toString(HttpConstants::kCreated.code) + " " +
                           HttpConstants::kCreated.reason + "\r\n" +
                           "Content-Length: 0\r\n" + "\r\n";
    handler_context.client.getResponse().setRaw(response);
    LOG_INFO() << BR_CYN "[Handler] uploaded " << file_name << " to "
               << full_upload_path << RESET;
}

/**
 * @brief Handles static file location blocks.
 * @note Flow:
 *       1. POST with no upload_path => .conf writer error => 500
 *       2. Reject paths containing ".." (directory traversal) => 403
 *       3. Build full_path = root + URI, stat() it => 404 if not found
 *       4. DELETE: regular file => deleteFile(), anything else => 403
 *       5. GET/HEAD: directory => resolveDirectory() (index, autoindex, or 403)
 *                    regular file => serveFile()
 *                    anything else (symlink, device...) => 403
 *       HEAD body stripping happens in run() after dispatch, not here.
 */
void Handler::handleStatic(HandlerContext& handler_context) {
    const std::string& path = handler_context.request.getPath();

    // POST on a static block means upload_path was not set: .conf writer error
    if (handler_context.request.getMethod() == "POST") {
        LOG_WARNING()
            << "[Handler] 500 - POST on static block with no upload_path";
        sendError(HttpConstants::kInternalServerError, handler_context);
        return;
    }

    // reject path with ".." it is a vector of attack
    if (path.find("..") != std::string::npos) {
        LOG_WARNING() << "[Handler] 403 - directory traversal attempt: " << RED
                      << path << RESET;
        sendError(HttpConstants::kForbidden, handler_context);
        return;
    }

    const std::string full_path = handler_context.location.getRoot() + path;
    LOG_DEBUG() << "[Handler] full path (root + uri) is: " << GRN << full_path
                << RESET;

    struct stat get_info;
    if (stat(full_path.c_str(), &get_info) != 0) {
        LOG_WARNING() << "[Handler] 404 - path not found: " << RED << full_path
                      << RESET;
        sendError(HttpConstants::kNotFound, handler_context);
        return;
    }

    if (handler_context.request.getMethod() == "DELETE") {
        if (!S_ISREG(get_info.st_mode)) {
            LOG_WARNING() << "[Handler] 403 - DELETE on non-regular file: "
                          << RED << full_path << RESET;
            sendError(HttpConstants::kForbidden, handler_context);
            return;
        }
        deleteFile(full_path, handler_context);  // TODO
        return;
    }

    if (S_ISDIR(get_info.st_mode)) {
        if (path[path.size() - 1] != '/') {
            LOG_INFO() << BR_CYN
                "[Handler] 301 - directory trailing slash redirect: "
                       << path << " -> " << path << "/" << RESET;
            std::string response =
                "HTTP/1.1 301 Moved Permanently\r\n"
                "Location: " +
                path +
                "/\r\n"
                "Content-Length: 0\r\n\r\n";
            handler_context.client.getResponse().setRaw(response);
            return;
        }
        resolveDirectory(full_path, handler_context);
    } else if (S_ISREG(get_info.st_mode)) {
        LOG_DEBUG() << "[Handler] file detected, serving: " << GRN << full_path
                    << RESET;
        serveFile(full_path, handler_context);
    } else {
        LOG_WARNING() << "[Handler] 403 - not a regular file or directory: "
                      << RED << full_path << RESET;
        sendError(HttpConstants::kForbidden, handler_context);
    }
}

/**
 * @brief Resolves a directory path: tries the index file first, falls back to
 * directory listing or 403 if listing is off.
 * @note Empty index string skips directly to listing/403 without stat()-ing
 *       the directory itself (which would always succeed).
 */
void Handler::resolveDirectory(const std::string& full_path,
                               HandlerContext& handler_context) {
    const std::string& index = handler_context.location.getIndex();
    if (index.empty()) {
        if (handler_context.location.getDirectoryListing()) {
            generateDirectoryListing(full_path, handler_context);
        } else {
            LOG_WARNING() << "[Handler] 403 - directory listing off, no index: "
                          << RED << full_path << RESET;
            sendError(HttpConstants::kForbidden, handler_context);
        }
        return;
    }

    std::string path_to_serve =
        full_path + (full_path[full_path.size() - 1] == '/' ? "" : "/") + index;
    LOG_DEBUG() << "[Handler] directory detected, trying index: " << GRN
                << path_to_serve << RESET;

    struct stat get_info;
    if (stat(path_to_serve.c_str(), &get_info) != 0) {
        if (handler_context.location.getDirectoryListing()) {
            generateDirectoryListing(full_path, handler_context);
        } else {
            LOG_WARNING() << "[Handler] 403 - directory listing off, no index: "
                          << RED << full_path << RESET;
            sendError(HttpConstants::kForbidden, handler_context);
        }
    } else if (S_ISREG(get_info.st_mode)) {
        serveFile(path_to_serve, handler_context);
    } else {
        LOG_WARNING() << "[Handler] 403 - index is not a regular file: " << RED
                      << path_to_serve << RESET;
        sendError(HttpConstants::kForbidden, handler_context);
    }
}

/**
 * @brief Deletes a regular file from disk and returns 204 No Content.
 * @note Caller guarantees full_path is a regular file (S_ISREG checked in
 *       handleStatic). Returns 500 if std::remove() fails.
 */
void Handler::deleteFile(const std::string& full_path,
                         HandlerContext& handler_context) {
    if (std::remove(full_path.c_str()) != 0) {
        LOG_WARNING() << "[Handler] 500 - internal server error "
                      << "-  failed to delete resource " << RED << full_path
                      << RESET;

        sendError(HttpConstants::kInternalServerError, handler_context);
        return;
    }
    // replace setRaw() with Charlie's setStatus/setHeader/setBody when
    // available
    std::string response = "HTTP/1.1 " +
                           toString(HttpConstants::kNoContent.code) + " " +
                           HttpConstants::kNoContent.reason + "\r\n" +
                           "Content-Length: 0\r\n" + "\r\n";
    handler_context.client.getResponse().setRaw(response);
    LOG_INFO() << BR_CYN "[Handler] deleted resource : " << full_path << RESET;
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
            serveFile(it->second, handler_context, code, reason);
            return;
        }
        LOG_DEBUG() << "[Handler] custom error page not found on disk: " << RED
                    << it->second << RESET << " - serving fallback error page";
    }
    // no custom page found: build minimal hardcoded HTML response
    // replace setRaw() calls with Charlie's setStatus/setHeader/setBody when
    // available
    std::string body = "<html><body><h1>" + toString(code) + " " + reason +
                       "</h1></body></html>";
    std::string response = "HTTP/1.1 " + toString(code) + " " + reason +
                           "\r\n" + "Content-Type: text/html\r\n" +
                           "Content-Length: " + toString(body.size()) + "\r\n" +
                           "\r\n" + body;
    handler_context.client.getResponse().setRaw(response);
}

std::string Handler::toString(int code) {
    std::ostringstream oss;
    oss << code;
    return oss.str();
}

std::string Handler::toString(size_t code) {
    std::ostringstream oss;
    oss << code;
    return oss.str();
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
 * @brief Opens, reads, and serves a regular file as an HTTP response.
 * @param path   Absolute or relative path to the file to serve
 * @param code   HTTP status code (default 200 for normal file serving)
 * @param reason HTTP reason phrase (default "OK")
 * @note code/reason default to 200 OK for regular static file serving.
 *       sendError() passes the actual error code so custom error pages are
 *       served with the correct status line instead of a misleading 200.
 *       Detects Content-Type from the file extension via getFileMimeType().
 *       Sends 500 if the file cannot be opened or read.
 */
void Handler::serveFile(const std::string& path,
                        HandlerContext& handler_context, int code,
                        const std::string& reason) {
    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        LOG_WARNING()
            << "[Handler] 500 - internal server error - could not open file: "
            << RED << path << RESET;
        // no sendError() here: if this file IS the custom 500 error page and
        // open fails, calling sendError() would call serveFile() again => loop
        std::string err_body =
            "<html><body><h1>500 Internal Server Error</h1></body></html>";
        handler_context.client.getResponse().setRaw(
            "HTTP/1.1 500 Internal Server Error\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: " +
            toString(err_body.size()) + "\r\n\r\n" + err_body);
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
        // same recursion guard as above
        std::string err_body =
            "<html><body><h1>500 Internal Server Error</h1></body></html>";
        handler_context.client.getResponse().setRaw(
            "HTTP/1.1 500 Internal Server Error\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: " +
            toString(err_body.size()) + "\r\n\r\n" + err_body);
        return;
    }
    close(fd);

    const std::string& mime_type = getFileMimeType(path);
    // replace setRaw() with Charlie's setStatus/setHeader/setBody when
    // available
    std::string response = "HTTP/1.1 " + toString(code) + " " + reason +
                           "\r\n" + "Content-Type: " + mime_type + "\r\n" +
                           "Content-Length: " + toString(body.size()) + "\r\n" +
                           "\r\n" + body;
    handler_context.client.getResponse().setRaw(response);
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
    // DIR* is basically FILE* but for directories, opendir() opens the stream
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
        // skip "." and ".." entries
        if (name == "." || name == "..") {
            continue;
        }
        // <li> is a list item, <a href="name"> makes it a clickable link to
        // that entry
        directory_list += "<li><a href=\"" + name + "\">" + name + "</a></li>";
    }
    closedir(directory);

    std::string body =
        "<html><body><ul>" + directory_list + "</ul></body></html>";
    // replace setRaw() with Charlie's setStatus/setHeader/setBody when
    // available
    std::string response =
        "HTTP/1.1 " + toString(static_cast<int>(HttpConstants::kOK.code)) +
        " " + HttpConstants::kOK.reason + "\r\n" +
        "Content-Type: text/html\r\n" +
        "Content-Length: " + toString(body.size()) + "\r\n" + "\r\n" + body;
    handler_context.client.getResponse().setRaw(response);
    LOG_INFO() << BR_CYN "[Handler] directory listing of " << path
               << " served successfully" << RESET;
}
