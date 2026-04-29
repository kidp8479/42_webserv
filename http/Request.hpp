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
		static const size_t kDefaultMaxHeaderSize = 8192;	// 8KB
		static const size_t kDefaultMaxBodySize = 1048576;	// 1MB

		/* Orthodox Canonical Form */
		Request();
		~Request();
		Request(const Request& other);
		Request &operator=(const Request& other);

		/* Getters */
		HttpMethod							getMethod() const;
		std::string							getTarget() const;
		std::string							getProtocol() const;
		std::string							getBody() const;
		std::map<std::string, std::string>	getHeaders() const;

		/* Setters */
		void	append(const char* data, size_t len);
		void	clearData();
		void	setMaxHeaderSize(size_t max_header_size);
		void	setMaxBodySize(size_t max_body_size);

		/* Checkers */
		bool	isComplete() const;

		/* Parsing */
		void	parseMessage();
	
	private:
		std::string							raw_;
		HttpMethod							method_;
		std::string							target_;
		std::string							protocol_;
		std::map<std::string, std::string>	headers_;
		std::string							body_;

		size_t								max_header_size_;
		size_t								max_body_size_;

		bool								complete_;
		bool								error_;
		std::string							error_code_;

		/* Private parsing tools */
		void	parseBodyCL(std::istringstream& raw_ss, std::string len);
		void	parseBodyChunked(std::istringstream& raw_ss);
		void	setError(std::string flag);
};

#endif
