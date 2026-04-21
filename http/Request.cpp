#include "Request.hpp"
#include <iostream>
#include <sstream>

/*****************************************************************************
 *                                  REQUEST                                  *
 *****************************************************************************/

 /**
 * @brief Default Constructor.
 */
Request::Request() {
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
	return (*this);
}

 /**
 * @brief Default Destructor.
 */
Request::~Request() {
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


/********************************* Checkers *********************************/
 /**
 * @brief Check if request is complete.
 * @return true if the raw string constitutes a complete message
 */
bool Request::isComplete() const {
	if (this->raw_.find("\r\n\r\n") == std::string::npos)
		return (false);

	std::string			line, contentLen;
	std::istringstream	rawStream(this->raw_);
	bool				atBody = false;

	/*Search for Content-Length header in raw until empty line*/
	while (!atBody && std::getline(rawStream, line)) {
		if (!line.empty() && line.at(line.size() - 1) == '\r')
			line.erase(line.size() - 1);
		if (!line.empty())
		{
			std::string			headerName;
			std::istringstream	lineStream(line);

			std::getline(lineStream, headerName, ':');
			if (headerName == "Content-Length") {
				std::getline(lineStream, contentLen);
				contentLen.erase(0, contentLen.find_first_not_of(' '));
			}
		}
		else
			atBody = true;
	}

	if (!contentLen.empty()) {
		/*Content Length was found before empty line: get length as a size_t*/
		size_t				bodySize;
		std::istringstream	lenStream(contentLen);

		lenStream >> bodySize;
		if (!lenStream.fail()) {
			/*Content Length is a valid number: get body as a string*/
			std::string	tempBody;

			while (std::getline(rawStream, line)) {
				if (!tempBody.empty())
					tempBody += '\n';
				tempBody += line;
			}

			/*Compare sizes*/
			return (bodySize == tempBody.size());
		}
	}

	return (true);
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
}


/********************************* Parsing **********************************/

 /**
 * @brief Parse the stored raw string to extract request data.
 */
void Request::parseMessage() {
	std::string			line;
	std::istringstream	rawStream(this->raw_);
	bool				atStartLine = true;
	bool				atBody = false;

	this->clearData();
	while (std::getline(rawStream, line)) {
		if (!atBody) {
			if (!line.empty() && line.at(line.size() - 1) == '\r')
				line.erase(line.size() - 1);
			std::istringstream	lineStream(line);

			if (line.empty())
				atBody = true;
			else if (atStartLine) {
				/*Parsing request start line*/
				std::string	strMethod;
				std::string	methodCheck[3] = {"GET", "POST", "DELETE"};
				HttpMethod	methodSet[3] = {kGet, kPost, kDelete};

				lineStream >> strMethod >> this->target_ >> this->protocol_;
				for (size_t i = 0; i < 3 && this->method_ == kNone; i++) {
					if (strMethod == methodCheck[i])
						this->method_ = methodSet[i];
				}
				atStartLine = false;
			}
			else if (!atBody) {
				/*Parsing headers*/
				std::string	name, value;
				
				std::getline(lineStream, name, ':');
				std::getline(lineStream, value);
				value.erase(0, value.find_first_not_of(' '));
				this->headers_[name] = value;
			}
		}
		else {
			/*Parsing body*/
			if (!this->body_.empty())
				this->body_ += '\n';
			this->body_ += line;
		}
	}
}