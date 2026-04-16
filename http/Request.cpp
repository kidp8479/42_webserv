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
 * @brief Constructor.
 * @param raw Full request message as a string.
 */
Request::Request(std::string raw)
	: raw_(raw) {
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
 * @return true if the request message is complete
 */
bool Request::isComplete() const {
	return (false); //WIP!
}


/********************************* Setters **********************************/
 /**
 * @brief Append string to raw string.
 * @param data String to append to raw string.
 * @param len Number of characters to append.
 */
void Request::append(const char* data, size_t len) {
	for (size_t i = 0; i < len && data[i] != '\0'; i++)
		this->raw_ += data[i];
}

 /**
 * @brief Set raw string.
 * @param raw String to set raw string to.
 */
void Request::setRaw(std::string raw) {
	this->raw_ = raw;
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

	while (std::getline(rawStream, line)) {
		std::istringstream	lineStream(line);

		if (atStartLine) {
			/*Parsing request start line*/
			std::string	strMethod;
			std::string	methodCheck[3] = {"GET", "POST", "DELETE"};
			HttpMethod	methodSet[3] = {kGet, kPost, kDelete};

			this->method_ = kNone;
			std::getline(lineStream, strMethod, ' ');
			for (size_t i = 0; i < 3 && this->method_ == kNone; i++) {
				if (strMethod == methodCheck[i])
					this->method_ = methodSet[i];
			}
			std::getline(lineStream, this->target_, ' ');
			std::getline(lineStream, this->protocol_, ' ');
			atStartLine = false;
		}
		else if (!atBody) {
			/*Parsing headers*/
			if (line != "") {
				std::string	name, value;
				
				std::getline(lineStream, name, ':');
				std::getline(lineStream, value);
				value.erase(0, value.find_first_not_of(' '));
				this->headers_[name] = value;
				std::cout << "Header \'" << name << "' of value " << value << std::endl;
			}
			else
				atBody = true;
		}
		else {
			/*Parsing body*/
			if (!this->body_.empty())
				this->body_ += '\n';
			this->body_ += line;
		}
	}
}