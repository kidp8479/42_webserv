#include "Request.hpp"

Request::Request() {
}

Request::~Request() {
}
/**
 * @brief Appends raw incoming data to the HTTP request buffer.
 *
 * Accumulates streamed socket data into an internal raw buffer.
 * This is required because HTTP requests may arrive in multiple chunks
 * over multiple recv() calls.
 *
 * @note This is a temporary minimal implementation used to support
 *       incremental request building in the server pipeline.
 *       Full parsing of HTTP semantics (method, headers, body) will be
 *       handled in the dedicated HTTP parsing layer (Charlie’s part).
 *
 * @param data Pointer to incoming raw data buffer
 * @param len Number of bytes to append
 */
void Request::append(const char* data, size_t len) {
    raw_.append(data, len);
}
