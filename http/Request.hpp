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
		static const size_t kDefaultMaxHeaderSize = 8192;	// 8KB
		static const size_t kDefaultMaxBodySize = 1048576;	// 1MB

		/* Orthodox Canonical Form */

		Request();
		~Request();
		Request(const Request& other);
		Request &operator=(const Request& other);

		/* Getters */

		std::string							getMethod() const;
		std::string							getTarget() const;
		std::string							getProtocol() const;
		std::string							getBody() const;
		std::map<std::string, std::string>	getHeaders() const;

		bool								isComplete() const;
		bool								isError() const;
		std::string							getErrorCode() const;

		/* Setters */

		void	append(const char* data, size_t len);
		void	clearData();
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
		std::string							error_code_;

		bool								allow_empty_start_;
		bool								at_start_line_;
		bool								at_body_;

		/* Private parsing tools */
		void	parseStartLine();
		void	parseHeaders();
		void	parseBody();
		void	parseBodyContentLen(std::string len);
		void	parseBodyChunked();
		void	setError(std::string flag);
};

#endif
