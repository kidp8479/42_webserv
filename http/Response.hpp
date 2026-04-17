#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>
#include "Request.hpp"

class Response {
public:
	Response();

	// what this function should do :
	// read parsed request data (method, path, etc.), decide what to return,
	// construct a valid HTTP response string
	// and store the results in raw_:
	void				buildFrom(const Request& request);
	
	// this function will allow me to get the results
	const std::string&	getRaw() const;

private:
	std::string raw_;
};

#endif

