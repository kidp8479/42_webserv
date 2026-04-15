#include "Request.hpp"

Request::Request() {}

Request::~Request() {}

void Request::append(const char* data, size_t len) {
    raw_.append(data, len);
}
