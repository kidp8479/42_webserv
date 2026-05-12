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

const HttpError kBadRequest = {400, "Bad Request"};
const HttpError kBodyTooLarge = {413, "Content Too Large"};
const HttpError kURITooLong = {414, "URI Too Long"};
const HttpError kHeaderTooLarge = {431, "Request Header Fields Too Large"};
const HttpError kHTTPNotSupported = {505, "HTTP Version Not Supported"};
}  // namespace HttpConstants

#endif