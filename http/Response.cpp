#include "Response.hpp"

Response::Response() : raw_("") {}

void Response::buildFrom(const Request& request)
{
	(void)request;

	// minimal valid HTTP response
	raw_ =
		"HTTP/1.1 200 OK\r\n"
		"Content-Length: 11\r\n"
		"\r\n"
		"Hello World";
}

const std::string& Response::getRaw() const {
	return raw_;
}
