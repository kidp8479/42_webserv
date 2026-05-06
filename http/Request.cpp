#include "Request.hpp"
#include "../utils/HttpConstants.hpp"
#include "../logger/Logger.hpp"

#include <iostream>
#include <sstream>

/*****************************************************************************
 *                                  REQUEST                                  *
 *****************************************************************************/

 /**
 * @brief Default Constructor.
 */
Request::Request() :
max_header_size_(HttpConstants::kDefaultMaxHeaderSize),
max_body_size_(HttpConstants::kDefaultMaxBodySize)
{
	clearData();
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
	raw_ = other.raw_;
	method_ = other.method_;
	target_ = other.target_;
	protocol_ = other.protocol_;
	headers_ = other.headers_;
	body_ = other.body_;

	max_header_size_ = other.max_header_size_;
	max_body_size_ = other.max_body_size_;

	complete_ = other.complete_;
	error_ = other.error_;
	error_code_ = other.error_code_;
	error_message_ = other.error_message_;
	keep_alive_ = other.keep_alive_;

	allow_empty_start_ = other.allow_empty_start_;
	at_start_line_ = other.at_start_line_;
	at_body_ = other.at_body_;
	return (*this);
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

static bool	findWhitespace(std::string s) {
	for (std::string::iterator s_it = s.begin(); s_it != s.end(); s_it++) {
		if (std::isspace(s_it[0]))
			return (true);
	}
	return (false);
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


/********************************* Getters **********************************/

 /**
 * @brief Get Request Method.
 */
std::string Request::getMethod() const {
	return (method_);
}

 /**
 * @brief Get Request Target.
 */
std::string Request::getTarget() const {
	return (target_);
}

 /**
 * @brief Get Path from Target.
 */
std::string Request::getPath() const {
	return (target_.substr(0, target_.find('?')));
}

 /**
 * @brief Get Query from Target if it exists.
 */
std::string Request::getQuery() const {
	size_t	query_pos = target_.find('?');
	if (query_pos != std::string::npos)
		return (target_.substr(query_pos + 1));
	return ("");
}

 /**
 * @brief Get Request Protocol.
 */
std::string Request::getProtocol() const {
	return (protocol_);
}

 /**
 * @brief Get Request Body.
 */
std::string Request::getBody() const {
	return (body_);
}

 /**
 * @brief Get Header Value corresponding to key.
 * @param key Header name to look for (case-insensitive).
 */
std::string Request::getHeaderValue(const std::string key) const {
	std::string	key_lower(key);
	return (headers_.at(setToLower(key_lower)));
}

 /**
 * @brief Get constant reference to Request Headers.
 */
const std::map<std::string, std::string>& Request::getHeaders() const {
	return (headers_);
}

 /**
 * @brief Check if request is complete.
 */
bool Request::isComplete() const {
	return (complete_);
}

 /**
 * @brief Check if Request returned an error.
 */
bool Request::isError() const {
	return (error_);
}

 /**
 * @brief Get error code.
 */
int Request::getErrorCode() const {
	return (error_code_);
}

 /**
 * @brief Get error message.
 */
std::string Request::getErrorMessage() const {
	return (error_message_);
}

 /**
 * @brief Check if a connection should be kept alive after request.
 */
bool Request::shouldKeepAlive() const {
	return (keep_alive_);
}


/********************************* Setters **********************************/

 /**
 * @brief Append string to raw string, then parse it.
 * @param data String to append to raw string.
 * @param len Number of characters to append.
 */
void Request::append(const char* data, size_t len) {
	raw_.append(data, len);
	parseStartLine();
	parseHeaders();
	parseBody();
}

 /**
 * @brief Wipe out all data with the exception of the raw message string
 */
void Request::clearData() {
	method_.clear();
	target_.clear();
	protocol_.clear();
	headers_.clear();
	body_.clear();

	complete_ = false;
	error_ = false;
	error_code_ = 0;
	error_message_.clear();
	keep_alive_ = false;

	allow_empty_start_ = true;
	at_start_line_ = true;
	at_body_ = false;
}

 /**
 * @brief Reset data except for raw, then parse raw
 */
void Request::resetData() {
	clearData();
	parseStartLine();
	parseHeaders();
	parseBody();
}

void Request::setMaxHeaderSize(size_t max_header_size) {
	max_header_size_ = max_header_size;
}

void Request::setMaxBodySize(size_t max_body_size) {
	max_body_size_ = max_body_size;
}

 /**
 * @brief Sets error to true along with error message.
 * @param int Error code to set error_code_ to.
 * @param message Error message to set error_message_ to.
 */
void Request::setError(int code, std::string message) {
	LOG_DEBUG() << "Request error: " << code << " " << message;
	error_ = true;
	error_code_ = code;
	error_message_ = message;
	if (code == 400)
		complete_ = true;
}

 /**
 * @brief Sets complete_ to true and adjusts keep_alive_.
 */
void Request::setComplete() {
	complete_ = true;
	if (protocol_ == "HTTP/1.1")
		keep_alive_ = true;
	if (headers_.count("connection") > 0) {
		if (headers_.at("connection") == "keep-alive")
			keep_alive_ = true;
		else if (headers_.at("connection") == "close")
			keep_alive_ = false;
	}
}


/********************************* Parsing **********************************/

 /**
 * @brief Parse the start line.
 */
void Request::parseStartLine() {
	if (complete_)
		return;

	while (at_start_line_ && raw_.find('\n') != std::string::npos) {
		std::string	line = raw_.substr(0, raw_.find('\n'));
		raw_.erase(0, raw_.find('\n') + 1);
		removeCR(line);
		
		if (line.empty()) {
			/*Allow one empty line before start*/
			if (allow_empty_start_)
				allow_empty_start_ = false;
			else
				return (setError(400, "Bad Request"));
		}
		else {
			/*Parsing request start line*/
			std::istringstream	line_stream(line);
			line_stream >> method_ >> target_ >> protocol_;

			/*Check for missing or malformed tokens*/
			if (method_.empty() || target_.empty() || protocol_.empty())
				return (setError(400, "Bad Request"));

			at_start_line_ = false;
		}
	}
}

/**
 * @brief Parse the header fields.
 */
void Request::parseHeaders() {
	if (complete_ || at_start_line_)
		return ;

	while (!at_body_ && raw_.find('\n') != std::string::npos) {
		std::string	line = raw_.substr(0, raw_.find('\n'));
		raw_.erase(0, raw_.find('\n') + 1);
		removeCR(line);

		if (line.empty()) {
			at_body_ = true;
			return ;
		}

		if (line.find(':') == std::string::npos)
			return (setError(400, "Bad Request"));
		std::string	name = line.substr(0, line.find(':'));
		if (findWhitespace(name))
			return (setError(400, "Bad Request"));
		setToLower(name);

		std::string	value = line.substr(line.find(':') + 1);
		trim(value);
		/*If header already exists, append value in comma-separated list*/
		if (headers_.count(name) > 0 && !headers_.at(name).empty())
			value = headers_.at(name) + ", " + value;
		if (value.size() > max_header_size_)
			setError(431, "Request Header Fields Too Large");
		headers_[name] = value;
	}
}

/**
 * @brief Parse the body.
 */
void Request::parseBody() {
	if (complete_ || at_start_line_ || !at_body_)
		return ;

	/*Check if a header indicates a body exists*/
	if (headers_.count("transfer-encoding") > 0
		&& headers_.at("transfer-encoding") == "chunked") {
		parseBodyChunked();
	}
	else if (headers_.count("content-length") > 0)
		parseBodyContentLen(headers_.at("content-length"));
	else {
		LOG_INFO() << "Request: fully parsed without message body";
		return (setComplete());
	}
}

 /**
 * @brief Parse body using Content-Length.
 * @param len value in Content-Length header.
 */
void Request::parseBodyContentLen(std::string len) {
	if (!isOnlyDigits(len))
		return (setError(400, "Bad Request"));

	size_t				len_value = 0;
	std::istringstream	len_stream(len);

	len_stream >> len_value;
	if (len_stream.fail())
		return (setError(400, "Bad Request"));
	if (len_value > max_body_size_)
		setError(413, "Content Too Large");

	/*Appending raw content to body*/
	while (body_.size() < len_value && !raw_.empty()) {
		body_ += raw_[0];
		raw_.erase(0, 1);
	}
	if (body_.size() != len_value)
		return ;//Reached end of stream - incomplete body
	LOG_INFO() << "Request: fully parsed "
	"using Content-Length for message body";
	return (setComplete());
}

 /**
 * @brief Parse body using Transfer-Encoding chunked method.
 */
void Request::parseBodyChunked() {
	std::string	chunk_size, content;

	while (raw_.find("\r\n") != std::string::npos) {
		std::string	size_line = raw_.substr(0, raw_.find("\r\n"));
		removeCR(size_line);

		if (!isOnlyHexDigits(size_line))
			return (setError(400, "Bad Request"));

		size_t				len_value;
		std::istringstream	len_stream(size_line);

		/*Convert hexadecimal chunk size to size_t*/
		len_stream >> std::hex >> len_value;
		if (len_stream.fail())
			return (setError(400, "Bad Request"));
		if (body_.size() + len_value > max_body_size_)
			setError(413, "Content Too Large");

		/*Check if chunk properly ends in CRLF*/
		size_t	chunk_end = raw_.find("\r\n") + len_value + 2;
		if (raw_.size() <= chunk_end)
			return ; //chunk too small - incomplete
		if (raw_.size() - chunk_end == 1 && raw_[chunk_end] == '\r')
			return ; //raw ends in "\r" - incomplete
		if (raw_.compare(chunk_end, 2, "\r\n") != 0)
			return (setError(400, "Bad Request"));
		
		raw_.erase(0, raw_.find("\r\n") + 2);
		
		/*Append chunk to body*/
		std::string	chunk_data;
		while (chunk_data.size() < len_value) {
			chunk_data += raw_[0];
			raw_.erase(0, 1);
		}
		body_ += chunk_data;

		/*Erase chunk CRF*/
		raw_.erase(0, 2);

		if (len_value == 0) {
			/*Reached null chunk - body parsing finished*/
			LOG_INFO() << "Request: fully parsed "
			"with chunked encoding for message body";
			return (setComplete());
		}
	}
}
