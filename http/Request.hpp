#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>

/**
 * @class Request
 * @brief Manages request parsing and request data storage
 *
 * WIP Description
 */
class Request {
	public:
		/* Orthodox Canonical Form */
		Request();
		~Request();
		Request(const Request& other);
		Request &operator=(const Request& other);

		/* List of accepted http methods */
		enum HttpMethod {
			NONE = 0,
			GET = 1,
			POST = 2,
			DELETE = 3
		};

		/* Getters */
		HttpMethod getMethod();

		/* Setters */
		void	readMessage(int fd);
	
	private:
		std::string	request_message_;
		HttpMethod	request_method_;
		std::string	request_target_;
		std::string	request_protocol_;
};

#endif