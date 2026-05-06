#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>
#include <map>

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

		/* Getters */

		std::string	getMethod() const;
		std::string	getTarget() const;
		std::string	getPath() const;
		std::string	getQuery() const;
		std::string	getProtocol() const;
		std::string	getBody() const;
		std::string	getHeaderValue(const std::string key) const;

		const std::map<std::string, std::string>&	getHeaders() const;

		bool		isComplete() const;
		bool		isError() const;
		int			getErrorCode() const;
		std::string	getErrorMessage() const;
		bool		shouldKeepAlive() const;

		/* Setters */

		void	append(const char* data, size_t len);
		void	clearData();
		void	resetData();
		void	setMaxHeaderSize(size_t max_header_size);
		void	setMaxBodySize(size_t max_body_size);
	
	private:
		std::string							raw_;
		std::string							method_;
		std::string							target_;
		std::string							protocol_;
		std::map<std::string, std::string>	headers_;
		std::string							body_;

		size_t								max_header_size_;
		size_t								max_body_size_;

		bool								complete_;
		bool								error_;
		int									error_code_;
		std::string							error_message_;
		bool								keep_alive_;

		bool								allow_empty_start_;
		bool								at_start_line_;
		bool								at_body_;

		/* Private parsing tools */
		void	parseStartLine();
		void	parseHeaders();
		void	parseBody();
		void	parseBodyContentLen(std::string len);
		void	parseBodyChunked();
		void	setError(int code, std::string message);
		void	setComplete();
};

#endif
