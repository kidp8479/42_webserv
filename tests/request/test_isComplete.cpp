#include <gtest/gtest.h>
#include "../../http/Request.hpp"
#include <string.h>
/**
 * Why I didn't use a test fixture here: the strings tested are all
 * different, and its easer just to see the actual string within
 * the scope of each test. I did not test the helper functions bc
 * they are static to the .cpp file and are indirectly tested through
 * the isComplete() function itself.
 */
TEST(RequestTest,
		EvilStringReturnsTrueParserNeedsToTakeCareofSemanticValidationSorryCharlieBrown) {
	Request req;

	const char* evilString =
		"GET / HTTP/1.1\r\n"
		"Cookie Cookie: $EvilString=\"Content-Length: 5\"\r\n"
		"\r\n";
	
	req.append(evilString, strlen(evilString));

	EXPECT_TRUE(req.isComplete());
}

TEST(RequestTest, EvilContentLengthInCookieIgnoredReturnsTrue) {
    Request req;
    const char* raw = "GET / HTTP/1.1\r\n"
        "Cookie: $Evil=\"Content-Length: 999\"\r\n"
        "\r\n";
    // total request is 57 bytes, no real Content-Length header present
    // isComplete() should return true after \r\n\r\n with no body expected
    size_t expected_len = strlen(raw);
    req.append(raw, expected_len);
	
    EXPECT_TRUE(req.isComplete());
    // if isComplete() had incorrectly picked up Content-Length: 999
    // it would return false here waiting for 999 more bytes
	
	// prove it didnt wait for 999 more bytes
	req.append("A", 1); // add one more
	EXPECT_TRUE(req.isComplete()); // it's still complete
}

TEST(RequestTest, IsComplete_ContentLengthWithExactBodyReturnsTrue) {
	const char* raw = "POST / HTTP/1.1\r\nContent-Length: 5\r\n\r\nhello";

	Request req;
	req.append(raw, strlen(raw));

	EXPECT_TRUE(req.isComplete());
}

TEST(RequestTest, IsComplete_NoContentLengthReturnsTrue) {
	const char* raw = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n";

	Request req;
	req.append(raw, strlen(raw));

	EXPECT_TRUE(req.isComplete());
}

TEST(RequestTest, IsComplete_ContentLengthBodyNotYetCompleteReturnsFalse) {
	const char* raw = "POST / HTTP/1.1\r\nContent-Length: 5\r\n\r\nhel";

	Request req;
	req.append(raw, strlen(raw));

	EXPECT_FALSE(req.isComplete());
}

TEST(RequestTest, IsComplete_ContentLenghtIsZeroReturnsTrue) {
	const char* raw = "POST / HTTP/1.1\r\nContent-Length: 0\r\n\r\n";

	Request req;
	req.append(raw, strlen(raw));

	EXPECT_TRUE(req.isComplete());
}

TEST(RequestTest, IsComplete_TwoIdenticalContentLengthsReturnsTrue) {
	const char* raw =
		"POST / HTTP/1.1\r\nContent-Length: 5\r\nContent-Length: 5\r\n\r\nhello";

	Request req;
	req.append(raw, strlen(raw));

	EXPECT_TRUE(req.isComplete());
}

TEST(RequestTest, IsComplete_TwoDifferentContentLengthsReturnsFalse) {
	const char* raw =
		"POST / HTTP/1.1\r\nContent-Length: 5\r\nContent-Length: 9\r\n\r\nhello";

	Request req;
	req.append(raw, strlen(raw));

	EXPECT_FALSE(req.isComplete());
}

TEST(RequestTest, IsComplete_ContentLenghtUppercaseReturnsTrue) {
	const char* raw = "POST / HTTP/1.1\r\nCONTENT-LENGTH: 5\r\n\r\nhello";

	Request req;
	req.append(raw, strlen(raw));

	EXPECT_TRUE(req.isComplete());
}

TEST(RequestTest, IsComplete_ContentLenghtLowercaseReturnsTrue) {
	const char* raw = "POST / HTTP/1.1\r\ncontent-length: 5\r\n\r\nhello";

	Request req;
	req.append(raw, strlen(raw));

	EXPECT_TRUE(req.isComplete());
}

TEST(RequestTest, IsComplete_ContentLenghtMixedcaseReturnsTrue) {
	const char* raw = "POST / HTTP/1.1\r\nConTeNt-LEngTH: 5\r\n\r\nhello";

	Request req;
	req.append(raw, strlen(raw));

	EXPECT_TRUE(req.isComplete());
}

TEST(RequestTest, IsComplete_EmptyStringReturnsFalse) {
	const char* raw = "";

	Request req;
	req.append(raw, strlen(raw));

	EXPECT_FALSE(req.isComplete());
}

TEST(RequestTest, IsComplete_PartialRequestReturnsFalse) {
	const char* raw = "GET / HTTP/1.1";

	Request req;
	req.append(raw, strlen(raw));

	EXPECT_FALSE(req.isComplete());
}

TEST(RequestTest, IsComplete_HeadersButNoTerminatorReturnsFalse) {
	const char* raw = "GET / HTTP/1.1\r\nHost: localhost";

	Request req;
	req.append(raw, strlen(raw));

	EXPECT_FALSE(req.isComplete());
}

TEST(RequestTest, IsComplete_BodyLargerThanContentLengthReturnsTrue) {
	const char* raw = "POST / HTTP/1.1\r\nContent-Length: 3\r\n\r\nhello";

	Request req;
	req.append(raw, strlen(raw));

	EXPECT_TRUE(req.isComplete());
}
