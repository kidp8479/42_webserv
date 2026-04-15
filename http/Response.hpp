#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>

class Request; // fwd declaration

class Response {
public:
	Response();

	void	buildFrom(const Request& request);
	const std::string& getRaw() const;

private:
	std::string raw_;
};

#endif

