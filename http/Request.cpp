#include "Request.hpp"
#include <iostream>
#include <sstream>

/*****************************************************************************
 *                                  REQUEST                                  *
 *****************************************************************************/

 /**
 * @brief Default Constructor.
 */
Request::Request() :
method_(kNone),
max_header_size_(kDefaultMaxHeaderSize),
max_body_size_(kDefaultMaxBodySize),
complete_(false),
error_(false) {
}

 /**
 * @brief Default Destructor.
 */
Request::~Request() {
}

 /**
 * @brief Copy Constructor.
 * @param other Request to copy.
 */
Request::Request(const Request& other) {
	*this = other;
}

 /**
 * @brief Request '=' operator overload.
 * @param other Request to copy.
 */
Request	&Request::operator=(const Request& other) {
	this->raw_ = other.raw_;
	this->method_ = other.method_;
	this->target_ = other.target_;
	this->protocol_ = other.protocol_;
	this->headers_ = other.headers_;
	this->body_ = other.body_;
	this->max_header_size_ = other.max_header_size_;
	this->max_body_size_ = other.max_body_size_;
	this->complete_ = other.complete_;
	this->error_ = other.error_;
	return (*this);
}


/********************************* Getters **********************************/
 /**
 * @brief Get Request Method.
 */
HttpMethod Request::getMethod() const {
	return(this->method_);
}

 /**
 * @brief Get Request Target.
 */
std::string Request::getTarget() const {
	return(this->target_);
}

 /**
 * @brief Get Request Protocol.
 */
std::string Request::getProtocol() const {
	return(this->protocol_);
}

 /**
 * @brief Get Request Body.
 */
std::string Request::getBody() const {
	return(this->body_);
}

 /**
 * @brief Get Request Headers.
 */
std::map<std::string, std::string> Request::getHeaders() const {
	return(this->headers_);
}

/********************************* Setters **********************************/
 /**
 * @brief Append string to raw string.
 * @param data String to append to raw string.
 * @param len Number of characters to append.
 */
void Request::append(const char* data, size_t len) {
	this->raw_.append(data, len);
}

 /**
 * @brief Wipe out all data with the exception of the raw message string
 */
void Request::clearData() {
	this->method_ = kNone;
	this->target_.clear();
	this->protocol_.clear();
	this->headers_.clear();
	this->body_.clear();

	this->complete_ = false;
	this->error_ = false;
}

void Request::setMaxHeaderSize(size_t max_header_size) {
	this->max_header_size_ = max_header_size;
}

void Request::setMaxBodySize(size_t max_body_size) {
	this->max_body_size_ = max_body_size;
}

/****************************** Parsing Utils *******************************/
static std::string	setToLower(std::string& s) {
	for (std::string::iterator s_it = s.begin(); s_it != s.end(); s_it++) {
		s_it[0] = std::tolower(s_it[0]);
	}
	return (s);
}

static std::string	trimL(std::string& s, const char* t = " \t\n\r\f\v") {
	s.erase(0, s.find_first_not_of(t));
	return (s);
}

static std::string	trimR(std::string& s, const char* t = " \t\n\r\f\v") {
	s.erase(s.find_last_not_of(t) + 1);
	return (s);
}

static std::string	trim(std::string& s, const char* t = " \t\n\r\f\v") {
	trimL(s, t);
	return (trimR(s, t));
}

static std::string	removeCR(std::string& s) {
	if (!s.empty() && s.at(s.size() - 1) == '\r')
		s.erase(s.size() - 1);
	return (s);
}

static bool	isOnlyDigits(std::string s) {
	for (std::string::iterator s_it = s.begin(); s_it != s.end(); s_it++) {
		if (!std::isdigit(s_it[0]))
			return (false);
	}
	return (true);
}

static bool isOnlyHexDigits(std::string s) {
	for (std::string::iterator s_it = s.begin(); s_it != s.end(); s_it++) {
		if (!std::isdigit(s_it[0]) && !(std::tolower(s_it[0]) >= 'a'
			&& std::tolower(s_it[0]) >= 'f'))
			return (false);
	}
	return (true);
}


/********************************* Checkers *********************************/
 /**
 * @brief Check if request is complete.
 * @return true if the raw string constitutes a complete message
 */
bool Request::isComplete() const {
	if (this->raw_.find("\r\n\r\n") == std::string::npos)
		return (false);

	std::string			line, content_len;
	std::istringstream	raw_stream(this->raw_);
	bool				at_body = false;

	/*Search for Content-Length header in raw until empty line*/
	while (!at_body && std::getline(raw_stream, line)) {
		removeCR(line);
		if (!line.empty())
		{
			std::string			header_name;
			std::istringstream	line_stream(line);

			std::getline(line_stream, header_name, ':');
			if (header_name == "Content-Length") {
				std::getline(line_stream, content_len);
				content_len.erase(0, content_len.find_first_not_of(' '));
			}
		}
		else
			at_body = true;
	}

	if (!content_len.empty()) {
		/*Content Length was found before empty line: get length as a size_t*/
		size_t				body_size;
		std::istringstream	len_stream(content_len);

		len_stream >> body_size;
		if (!len_stream.fail()) {
			/*Content Length is a valid number: get body as a string*/
			std::string	temp_body;

			while (std::getline(raw_stream, line)) {
				if (!temp_body.empty())
					temp_body += '\n';
				temp_body += line;
			}

			/*Compare sizes*/
			return (body_size <= temp_body.size());
		}
	}

	return (true);
}


/********************************* Parsing **********************************/
 /**
 * @brief Parse the stored raw string to extract request data.
 */
void Request::parseMessage() {
	std::string			line;
	std::istringstream	raw_stream(this->raw_);
	bool				allow_empty_start = true;
	bool				at_start_line = true;
	bool				at_body = false;

	this->clearData();
	while (!at_body && std::getline(raw_stream, line)) {
		removeCR(line);
		std::istringstream	line_stream(line);

		if (line.empty()) {
			if (at_start_line && allow_empty_start)
				allow_empty_start = false;
			else
				at_body = true;
		}
		else if (at_start_line) {
			/*Parsing request start line*/
			std::string	str_method;
			std::string	method_check[3] = {"GET", "POST", "DELETE"};
			HttpMethod	method_set[3] = {kGet, kPost, kDelete};

			line_stream >> str_method >> this->target_ >> this->protocol_;
			for (size_t i = 0; i < 3 && this->method_ == kNone; i++) {
				if (str_method == method_check[i])
					this->method_ = method_set[i];
			}
			at_start_line = false;
		}
		else {
			/*Parsing headers*/
			std::string	name, value;
			
			std::getline(line_stream, name, ':');
			setToLower(name);
			std::getline(line_stream, value);
			trim(value);
			if (value.size() > this->max_header_size_)
				value.erase(this->max_header_size_);
			this->headers_[name] = value;
		}
	}

	if (at_body) {
		/*Parsing body*/
		if (this->headers_.count("transfer-encoding") > 0
			&& this->headers_.at("transfer-encoding") == "chunked") {
			parseBodyChunked(raw_stream);
		}
		else if (this->headers_.count("content-length") > 0)
			parseBodyCL(raw_stream, this->headers_.at("content-length"));
	}
}

 /**
 * @brief Parse body using Content-Length.
 * @param raw_stream stream poitning to body start point.
 * @param len value in Content-Length header.
 */
void Request::parseBodyCL(std::istringstream& raw_ss, std::string len) {
	if (!isOnlyDigits(len)) {
		this->error_ = true;
		return ; //WIP
	}

	size_t				body_size = 0;
	size_t				len_value;
	std::istringstream	len_stream(len);

	len_stream >> len_value;
	if (!len_stream.fail())
		body_size = std::min(len_value, this->max_body_size_);

	char		c;
	std::string	content;
	while (content.size() < body_size && raw_ss.get(c))
		content += c;
	this->body_ += content;
}

 /**
 * @brief Parse body using Transfer-Encoding chunked method.
 * @param raw_stream stream pointing to body start point.
 */
void Request::parseBodyChunked(std::istringstream& raw_ss) {
	std::string	chunk_size, content;

	while (std::getline(raw_ss, chunk_size)) {
		removeCR(chunk_size);
		if (!isOnlyHexDigits(chunk_size)) {
			this->error_ = true;
			return ; //WIP
		}

		size_t				len_value;
		std::istringstream	len_stream(chunk_size);

		/*Convert hexadecimal chunk size to size_t*/
		len_stream >> std::hex >> len_value;
		if (!len_stream.fail()) {
			char		c;
			std::string	chunk_data, chunk_end;

			while (chunk_data.size() < len_value && raw_ss.get(c))
				chunk_data += c;

			/*Check for CR LF termination after chunk*/
			char	c_end = '\0';
			while (c_end != '\n') {
				if (!raw_ss.get(c_end) || (c_end != '\r' && c_end != '\n'))
					return ; //WIP should be error
			}

			content += chunk_data;
			if (len_value == 0) {
				/*Reached null chunk - body parsing finished*/
				if (content.size() > this->max_body_size_)
					content.erase(this->max_body_size_);
				this->body_ += content;
				return ;
			}
		}
	}
}
