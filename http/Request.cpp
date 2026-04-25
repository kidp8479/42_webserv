#include "Request.hpp"
#include <sstream>

static const std::string HEADER_TERMINATOR = "\r\n\r\n";

Request::Request() {
}

Request::~Request() {
}
/**
 * @brief Appends raw incoming data to the HTTP request buffer.
 *
 * Accumulates streamed socket data into an internal raw buffer.
 * This is required because HTTP requests may arrive in multiple chunks
 * over multiple recv() calls.
 *
 * @note This is a temporary minimal implementation used to support
 *       incremental request building in the server pipeline.
 *       Full parsing of HTTP semantics (method, headers, body) will be
 *       handled in the dedicated HTTP parsing layer (Charlie’s part).
 *
 * @param data Pointer to incoming raw data buffer
 * @param len Number of bytes to append
 */
void Request::append(const char* data, size_t len) {
    raw_.append(data, len);
}


/* since content-length is case insensitive, i've decided to do a
 * comparison directly on the raw buffer instead of copying into a temp
 * buffer to normalize, because that is expensive and im poor.  So what
 * this function will do is convert the char to lower and then compare
 * with the needle, once it's found the position will be returned so that
 * we can extract the number
 *
 * note** returns postion AFTER needle is found, or npos if not found
 */
static size_t caseInsensitiveFind(const std::string& data, size_t start,
		size_t end, const std::string& needle) {
	
	for (size_t i = start; i + needle.size() <= end; i++) {
		size_t j;
		for (j = 0; j < needle.size(); j++) {
			if (std::tolower(data[i + j]) != needle[j])
				break ;
		}
		if (j == needle.size())
			return i + needle.size();
	}
	return std::string::npos;
}

static bool extractContentLength(const std::string& data,
		size_t start, size_t end, size_t& out_length) {
	//skip leading spaces
	while (start < end && std::isspace(data[start]))
		start++;
	//nothing left after spaces
	if (start == end)
		return false;
	//extract substring and put into stream
	std::string number = data.substr(start, end - start);
	std::istringstream iss(number);
	//store it into out_length
	iss >> out_length;
	if (iss.fail() || !iss.eof())
		return false;

	return true;
}

/* there is a possiblity that there is more than one content-lenght header.
 * as per the RFC, if both have the same length it's safe to use that length
 * and consider it as a valid header, otherwise, it is invalid. Why not
 * search for a 3rd? because now we're counting that as a malicious intent,
 * headers should normally have only ONE content length anyway.
 */
bool Request::isComplete() const {
	size_t header_end = raw_.find(HEADER_TERMINATOR);
	if (header_end == std::string::npos)
		return false;

	size_t cl1_pos =
		caseInsensitiveFind(raw_, 0, header_end, "\r\ncontent-length:");

	if (cl1_pos == std::string::npos)
		return true; // no body expected

	size_t endline1 = raw_.find("\r\n", cl1_pos);
	size_t cl1_value;
	if (!extractContentLength(raw_, cl1_pos, endline1, cl1_value))
		return false;

	size_t cl2_pos =
		caseInsensitiveFind(raw_, cl1_pos, header_end, "\r\ncontent-length:");

	if (cl2_pos != std::string::npos) {
		size_t endline2 = raw_.find("\r\n", cl2_pos);
		size_t cl2_value;
		if (!extractContentLength(raw_, cl2_pos, endline2, cl2_value)) {
			return false;
		}
		if (cl1_value != cl2_value) {
			return false;
		}
	}

	size_t body_start = header_end + HEADER_TERMINATOR.size();
	return raw_.size() - body_start >= cl1_value;
}
