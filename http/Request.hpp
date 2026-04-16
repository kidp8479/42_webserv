#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>
#include <map>

/* List of accepted http methods */
enum HttpMethod {
	kNone = 0,
	kGet,
	kPost,
	kDelete,
};

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

		/* Constructors */
		Request(std::string raw);

		/* Getters */
		HttpMethod							getMethod() const;
		std::string							getTarget() const;
		std::string							getProtocol() const;
		std::string							getBody() const;
		std::map<std::string, std::string>	getHeaders() const;

		/* Checkers */
		bool								isComplete() const;

		/* Setters */
		void	append(const char* data, size_t len);
		void	setRaw(std::string raw);

		/* Parsing */
		void	parseMessage();
	
	private:
		std::string							raw_;
		HttpMethod							method_;
		std::string							target_;
		std::string							protocol_;
		std::map<std::string, std::string>	headers_;
		std::string							body_;
};

#endif