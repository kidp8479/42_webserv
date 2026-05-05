#ifndef HTTP_CONSTANTS_HPP
#define HTTP_CONSTANTS_HPP

#include <cstddef>

namespace HttpConstants {
const size_t kDefaultMaxBodySize = 1048576;  // 1MB
const size_t kDefaultMaxHeaderSize = 8192;   // 8KB
}  // namespace HttpConstants

#endif