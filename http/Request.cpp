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

/**
 * @brief Performs a case-insensitive substring search in a bounded range.
 *
 * Searches for `needle` inside `data[start:end]` without allocating
 * or copying a temporary normalized buffer.
 *
 * @note This avoids copying/normalizing the full buffer for performance
 * reasons and operates directly on the raw request data.
 *
 * @warning This is a low-level utility and does NOT validate HTTP structure.
 *
 * @param data   Raw input buffer
 * @param start  Start index (inclusive)
 * @param end    End boundary (exclusive)
 * @param needle Target substring (assumed lowercase for comparison)
 *
 * @return Index immediately after the matched needle,
 *         or std::string::npos if not found
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

/**
 * @brief Extracts a numeric Content-Length value from a header substring.
 *
 * Parses a numeric string located in the range [start:end) and converts it
 * into a size_t value using a stringstream.
 *
 * @par parsing_rules
 * - Leading whitespace is ignored
 * - Trailing whitespace AFTER the numeric value is accepted
 * - Any non-whitespace trailing characters are rejected
 *
 * @param data        Raw request buffer
 * @param start       Start index of numeric value
 * @param end         End index of header line
 * @param out_length  Output parameter for parsed value
 *
 * @return true if parsing succeeded, false otherwise
 */
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
	if (iss.fail())
		return false;

	//skip trailing spaces
	char c;
	while (iss.get(c)) {
		if (!std::isspace(c))
			return false;
	}
	return true;
}

/**
 * @brief Determines whether the HTTP request is fully received.
 *
 * This function operates on the raw request buffer and checks:
 * 1. Presence of end-of-headers marker (\r\n\r\n)
 * 2. Optional Content-Length header(s)
 * 3. Whether the body size matches the expected length
 *
 * @design_choice
 * - Operates directly on the raw buffer without copying or normalization
 *   for performance reasons.
 * - Uses case-insensitive matching for HTTP header keys.
 *
 * @important
 * This function does NOT perform HTTP parsing or validation:
 * - parse headers into structured key/value pairs
 * - validate HTTP syntax
 * - enforce protocol correctness
 *
 * Its sole responsibility is to determine stream completeness at the I/O level.
 *
 * @content_length_handling
 * There may be multiple Content-Length headers in the raw request.
 * If multiple values exist, they must be identical to be considered valid
 * for computing body size.
 *
 * Only the first valid Content-Length value is used for completeness checking
 * in this layer. Additional occurrences are ignored here by design.
 *
 * Detection of malformed requests (e.g. multiple differing Content-Length
 * headers) is deferred to the HTTP parsing/validation layer.
 *
 * @security_note
 * Only valid header-line matches are considered (not substrings inside values),
 * reducing risk of header injection tricks.
 *
 * @layering_note
 * This separation ensures a clean architecture:
 * - This function handles transport-level completeness (stream logic)
 * - The parser handles semantic correctness and malicious input detection
 *
 * @return true if the request buffer contains a complete HTTP message
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
