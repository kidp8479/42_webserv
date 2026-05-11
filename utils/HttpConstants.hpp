#ifndef HTTP_CONSTANTS_HPP
#define HTTP_CONSTANTS_HPP

#include <cstddef>

namespace HttpConstants {
typedef struct HttpError {
   int code;
   const char* reason;
}   HttpError_t;

const size_t kDefaultMaxBodySize = 1048576;  // 1MB
const size_t kDefaultMaxHeaderSize = 8192;   // 8KB
const size_t kDefaultMaxURISize = 8192;      // 8KB

const HttpError_t kBadRequest = {400, "Bad Request"};
const HttpError_t kBodyTooLarge = {413, "Content Too Large"};
const HttpError_t kURITooLong = {414, "URI Too Long"};
const HttpError_t kHeaderTooLarge = {431, "Request Header Fields Too Large"};
}  // namespace HttpConstants

#endif