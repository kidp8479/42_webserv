#ifndef HTTP_CONSTANTS_HPP
#define HTTP_CONSTANTS_HPP

#include <cstddef>

namespace HttpConstants {
struct HttpError {
    int code;
    const char* reason;
};

const size_t kDefaultMaxBodySize = 1048576;  // 1MB
const size_t kDefaultMaxHeaderSize = 8192;   // 8KB
const size_t kDefaultMaxURISize = 8192;      // 8KB

// 2xx status code - success
const HttpError kOK = {200, "OK"};
const HttpError kCreated = {201, "Created"};
const HttpError kNoContent = {204, "No Content"};

// 3xx status code - redirection
const HttpError kMovedPermanently = {301, "Moved Permanently"};
const HttpError kFound = {302, "Found"};
const HttpError kTemporaryRedirect = {307, "Temporary Redirect"};
const HttpError kPermanentRedirect = {308, "Permanent Redirect"};

// 4xx status code - client's side error response
const HttpError kBadRequest = {400, "Bad Request"};
const HttpError kForbidden = {403, "Forbidden"};
const HttpError kNotFound = {404, "Not Found"};
const HttpError kMethodNotAllowed = {405, "Method Not Allowed"};
const HttpError kBodyTooLarge = {413, "Content Too Large"};
const HttpError kURITooLong = {414, "URI Too Long"};
const HttpError kHeaderTooLarge = {431, "Request Header Fields Too Large"};

// 5xx status code - server's side error response
const HttpError kInternalServerError = {500, "Internal Server Error"};
const HttpError kNotImplemented = {501, "Not Implemented"};
const HttpError kVersionNotSupported = {505, "HTTP Version Not Supported"};
}  // namespace HttpConstants

#endif