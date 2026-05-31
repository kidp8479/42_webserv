#include "Handler.hpp"

#include "../core/Client.hpp"
#include "../core/EventLoop.hpp"
#include "CgiSpawner.hpp"

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
               << " - target: " << request.getTarget() << RESET;

    HandlerContext handler_context = {request,
                                      location,
                                      client.getServerConfig(),
                                      client.getResponse(),
                                      client.getLoop(),
                                      client};

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
    if (response_ready && handler_context.request.getMethod() == "HEAD") {
        handler_context.response.setBody("");
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
    // only handle cookie session for the /cookie-session/ route and its
    // sub-paths deliberately excludes /cookie-session (no trailing slash) which
    // triggers a 301 redirect - creating a session there would cause the
    // browser to send the cookie on the redirect follow, skipping the "hello
    // stranger" first visit
    const std::string& path = handler_context.request.getPath();
    if (path.find("/cookie-session/") == 0)
        handleCookieSession(handler_context);

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

    handler_context.response.setStatus(return_code, reason);
    handler_context.response.setHeader("Location", return_url);
    handler_context.response.setHeader("Content-Length", "0");
    LOG_INFO() << BR_CYN "[Handler] redirect " << return_code << " -> "
               << return_url << RESET;
}

/**
 * @brief Handles CGI location blocks.
 * handleCgiInterpreters: forks, registers CgiProcess, returns false
 * Handler's job ends here - CgiProcess takes over
 */
bool Handler::handleCgiInterpreters(HandlerContext& handler_context) {
    // TODO: implement fork/execve/pipe
    try {
        CgiSpawner spawner(handler_context.loop);

        if (!spawner.spawn(handler_context.request, handler_context.location,
                           handler_context.client)) {
            LOG_WARNING() << "[Handler] CGI spawn failed";
            sendError(HttpConstants::kInternalServerError, handler_context);
            return true;  // error response is ready
        }
        return false;
    } catch (const std::exception& e) {
        LOG_ERROR() << "[Handler] CGI exception: " << e.what();
        sendError(HttpConstants::kNotImplemented, handler_context);
        return true;
    }
}

void Handler::applyCgiResponse(const std::string& raw, Response& response) {
    LOG_DEBUG() << "[CGI] raw size=" << raw.size();
    LOG_DEBUG() << "[CGI] raw preview:\n" << raw.substr(0, 200);

    size_t separator = raw.find("\r\n\r\n");
    size_t body_offset = 4;

    if (separator == std::string::npos) {
        separator = raw.find("\n\n");
        body_offset = 2;
    }
    std::string header_block;
    std::string body;

    if (separator != std::string::npos) {
        header_block = raw.substr(0, separator);
        body = raw.substr(separator + body_offset);
    } else {
        body = raw;
    }
    int status_code = 200;
    std::string reason = "OK";

    response.reset();
    LOG_DEBUG() << "[CGI] parsing headers...";

    std::istringstream stream(header_block);
    std::string line;

    while (std::getline(stream, line)) {
        if (!line.empty() && line[line.size() - 1] == '\r') {
            line.erase(line.size() - 1);
        }
        LOG_DEBUG() << "[CGI] header line: " << line;

        size_t colon = line.find(':');
        if (colon == std::string::npos) {
            continue;
        }
        std::string key = line.substr(0, colon);
        std::string value = line.substr(colon + 1);
        while (!value.empty() && value[0] == ' ') {
            value.erase(0, 1);
        }
        LOG_DEBUG() << "[CGI] header key=" << key << " value=" << value;
        if (key == "Status") {
            std::istringstream status_stream(value);
            status_stream >> status_code;
            std::getline(status_stream, reason);
            if (!reason.empty() && reason[0] == ' ') {
                reason.erase(0, 1);
            }
            LOG_DEBUG() << "[CGI] parsed Status=" << status_code
                        << " reason=" << reason;
        } else {
            response.setHeader(key, value);
        }
    }
    response.setStatus(status_code, reason);
    if (body.size() > 0) {
        response.setHeader("Content-Length", Handler::toString(body.size()));
    } else {
        response.setHeader("Content-Length", "0");
    }
    response.setBody(body);
    LOG_DEBUG() << "[CGI] FINAL RAW RESPONSE:\n"
                << response.getRaw().substr(0, 300);
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
    handler_context.response.setStatus(HttpConstants::kCreated);
    handler_context.response.setHeader("Content-Length", "0");
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
        deleteFile(full_path, handler_context);
        return;
    }

    if (S_ISDIR(get_info.st_mode)) {
        if (path[path.size() - 1] != '/') {
            LOG_INFO() << BR_CYN
                "[Handler] 301 - directory trailing slash redirect: "
                       << path << " -> " << path << "/" << RESET;
            handler_context.response.setStatus(
                HttpConstants::kMovedPermanently);
            handler_context.response.setHeader("Location", path + "/");
            handler_context.response.setHeader("Content-Length", "0");
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
    handler_context.response.setStatus(HttpConstants::kNoContent);
    handler_context.response.setHeader("Content-Length", "0");
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
    std::string body = "<html><body><h1>" + toString(code) + " " + reason +
                       "</h1></body></html>";
    handler_context.response.setStatus(code, reason);
    handler_context.response.setHeader("Content-Type", "text/html");
    handler_context.response.setHeader("Content-Length", toString(body.size()));
    handler_context.response.setBody(body);
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
        handler_context.response.setStatus(HttpConstants::kInternalServerError);
        handler_context.response.setHeader("Content-Type", "text/html");
        handler_context.response.setHeader("Content-Length",
                                           toString(err_body.size()));
        handler_context.response.setBody(err_body);
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
        handler_context.response.setStatus(HttpConstants::kInternalServerError);
        handler_context.response.setHeader("Content-Type", "text/html");
        handler_context.response.setHeader("Content-Length",
                                           toString(err_body.size()));
        handler_context.response.setBody(err_body);
        return;
    }
    close(fd);

    const std::string& mime_type = getFileMimeType(path);
    handler_context.response.setStatus(code, reason);
    handler_context.response.setHeader("Content-Type", mime_type);
    handler_context.response.setHeader("Content-Length", toString(body.size()));
    handler_context.response.setBody(body);
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
    handler_context.response.setStatus(HttpConstants::kOK);
    handler_context.response.setHeader("Content-Type", "text/html");
    handler_context.response.setHeader("Content-Length", toString(body.size()));
    handler_context.response.setBody(body);
    LOG_INFO() << BR_CYN "[Handler] directory listing of " << path
               << " served successfully" << RESET;
}

/**
 * @brief Generates a unique cookie session ID.
 * @return Hex string combining current timestamp and a random value.
 */
std::string Handler::newSessionID() {
    std::ostringstream ss;
    // combine timestamp + rand() for basic uniqueness within the same second
    // rand() is seeded at startup via srand(time(NULL)) in main.cpp
    ss << std::hex << time(NULL) << rand();
    return ss.str();
}

/**
 * @brief Manages cookie session lifecycle for the /cookie-session route.
 *
 * On each request: looks for a session_id cookie in the incoming request.
 * If absent or unknown, generates a new session ID, registers it in
 * ServerResources::sessions_, and sends Set-Cookie: session_id to the browser.
 * Always increments the visit_count for this session and sends it back via
 * Set-Cookie: visit_count so the page JS can read and display it.
 *
 * @note Only called for paths matching /cookie-session/ - see dispatch().
 * @note visit_count is stored server-side in sessions_ and mirrored as a
 *       cookie so static HTML can read it via document.cookie without CGI.
 */
void Handler::handleCookieSession(HandlerContext& handler_context) {
    // read all cookies from the incoming request (returns empty map if no
    // Cookie header)
    const std::map<std::string, std::string> cookies =
        handler_context.request.getCookieList();

    // look for our session cookie - operator[] don't work on const map, needed
    // to use find() instead
    std::string session_id;
    std::map<std::string, std::string>::const_iterator it =
        cookies.find("session_id");
    if (it != cookies.end())
        session_id = it->second;  // found: session_id holds the client's
                                  // existing session ID

    // if no session_id cookie or session doesn't exist in the store, create a
    // new one and send it back to the browser via Set-Cookie so it is stored
    // and returned on future requests
    if (session_id.empty() ||
        !handler_context.client.getResources().hasSession(session_id)) {
        session_id = newSessionID();
        handler_context.client.getResources().createSession(session_id);
        // Set-Cookie tells the browser to store this ID and send it back on
        // every subsequent request
        // Path=/ ensures the browser replaces any existing session_id cookie
        handler_context.response.setHeader(
            "Set-Cookie", "session_id=" + session_id + "; Path=/");
        LOG_INFO() << BR_CYN
            "[Handler] cookie session created and sent to browser: "
                   << session_id << RESET;
    } else {
        // session already exists, browser sent us a valid session_id, nothing
        // to create
        LOG_DEBUG() << BR_YEL "[Handler] cookie session resumed: " << session_id
                    << RESET;
    }

    // increment visit counter in server-side session data
    std::map<std::string, std::string>& session =
        handler_context.client.getResources().getOrCreateSession(session_id);
    int count = 0;
    if (!session["visit_count"].empty())
        count = std::atoi(session["visit_count"].c_str());
    count++;
    session["visit_count"] = toString(count);
    // send visit_count as a cookie so the browser JS can read and display it
    handler_context.response.setHeader(
        "Set-Cookie", "visit_count=" + toString(count) + "; Path=/");
    LOG_DEBUG() << BR_YEL "[Handler] cookie session visit count: " << count
                << RESET;
}