#include <gtest/gtest.h>
#include <string.h>

#include "../../http/Request.hpp"

TEST(RequestTest, Constructor_initialStateIsClean) {
	Request	req;

	EXPECT_EQ(req.getMethod(), kNone);
	EXPECT_EQ(req.getTarget(), "");
	EXPECT_EQ(req.getProtocol(), "");
	EXPECT_EQ(req.getBody(), "");
	EXPECT_TRUE(req.getHeaders().empty());
}

TEST(RequestTest, CopyConstructor_copyCleanReq) {
	Request	req;
	Request reqCopy(req);

	EXPECT_EQ(reqCopy.getMethod(), kNone);
	EXPECT_EQ(reqCopy.getTarget(), "");
	EXPECT_EQ(reqCopy.getProtocol(), "");
	EXPECT_EQ(reqCopy.getBody(), "");
	EXPECT_TRUE(reqCopy.getHeaders().empty());
}

class RequestTestFixture : public ::testing::Test {
protected:
	Request	req;

	// Full request
	const char* full_request =
	"GET / HTTP/1.1\r\n"
	"Host: www.example.com\r\n"
	"Content-Length: 5\r\n"
	"\r\n"
	"Hello";

	// chunks
	const char* chunk1 = "GET / HTTP/1.1\r\n";
	const char* chunk2 = "Host: www.example.com\r\n";
	const char* chunk3 = "Content-Length: 5\r\n";
	const char* chunk4 = "\r\n";
	const char* chunk5 = "Hello";

	// chunk variants - optional whitesapce
	const char* chunk1_OWS = "   GET  /    HTTP/1.1   \r\n";
	const char* chunk2_OWS = "Host:     www.example.com      \r\n";
	const char* chunk3_OWS = "Content-Length:      5      \r\n";

	void SetUp() override {
		req.clearData();
	}

	void TearDown() {

	}
};

TEST_F(RequestTestFixture, isComplete_FullRequest) {
	req.append(full_request, strlen(full_request));
	EXPECT_TRUE(req.isComplete());
}

TEST_F(RequestTestFixture, isComplete_AllChunks) {
	req.append(chunk1, strlen(chunk1));
	req.append(chunk2, strlen(chunk2));
	req.append(chunk3, strlen(chunk3));
	req.append(chunk4, strlen(chunk4));
	req.append(chunk5, strlen(chunk5));
	EXPECT_TRUE(req.isComplete());
}

TEST_F(RequestTestFixture, isComplete_NoEmptyLine) {
	req.append(chunk1, strlen(chunk1));
	req.append(chunk2, strlen(chunk2));
	req.append(chunk3, strlen(chunk3));
	req.append(chunk5, strlen(chunk5));
	EXPECT_FALSE(req.isComplete());
}

TEST_F(RequestTestFixture, isComplete_AddEmptyLine) {
	req.append(chunk1, strlen(chunk1));
	req.append(chunk2, strlen(chunk2));
	EXPECT_FALSE(req.isComplete());
	req.append(chunk4, strlen(chunk4));
	EXPECT_TRUE(req.isComplete());
}

TEST_F(RequestTestFixture, isComplete_NoHeadersNoBody) {
	req.append(chunk1, strlen(chunk1));
	req.append(chunk4, strlen(chunk4));
	EXPECT_TRUE(req.isComplete());
}

TEST_F(RequestTestFixture, isComplete_NoBody) {
	req.append(chunk1, strlen(chunk1));
	req.append(chunk2, strlen(chunk2));
	req.append(chunk4, strlen(chunk4));
	EXPECT_TRUE(req.isComplete());
}

TEST_F(RequestTestFixture, isComplete_AddBody) {
	req.append(chunk1, strlen(chunk1));
	req.append(chunk2, strlen(chunk2));
	req.append(chunk3, strlen(chunk3));
	req.append(chunk4, strlen(chunk4));
	EXPECT_FALSE(req.isComplete());
	req.append(chunk5, strlen(chunk5));
	EXPECT_TRUE(req.isComplete());
}

TEST_F(RequestTestFixture, isComplete_WrongContentLen) {
	req.append(chunk1, strlen(chunk1));
	req.append(chunk2, strlen(chunk2));
	req.append("Content-Length: 6\r\n", 19);
	req.append(chunk4, strlen(chunk4));
	req.append(chunk5, strlen(chunk5));
	EXPECT_FALSE(req.isComplete());
}

TEST_F(RequestTestFixture, isComplete_NoBodyInvalidContentLen) {
	req.append(chunk1, strlen(chunk1));
	req.append(chunk2, strlen(chunk2));
	req.append("Content-Length: abc\r\n", 21);
	req.append(chunk4, strlen(chunk4));
	EXPECT_TRUE(req.isComplete());
}

TEST_F(RequestTestFixture, isComplete_Nothing) {
	EXPECT_FALSE(req.isComplete());
}

TEST_F(RequestTestFixture, Parse_FullRequest) {
	req.append(full_request, strlen(full_request));
	req.parseMessage();
	EXPECT_EQ(req.getMethod(), kGet);
	EXPECT_EQ(req.getTarget(), "/");
	EXPECT_EQ(req.getProtocol(), "HTTP/1.1");
	EXPECT_EQ(req.getBody(), "Hello");
	EXPECT_EQ(req.getHeaders().at("content-length"), "5");
}

TEST_F(RequestTestFixture, Parse_AllChunks) {
	req.append(chunk1, strlen(chunk1));
	req.append(chunk2, strlen(chunk2));
	req.append(chunk3, strlen(chunk3));
	req.append(chunk4, strlen(chunk4));
	req.append(chunk5, strlen(chunk5));
	req.parseMessage();
	EXPECT_EQ(req.getMethod(), kGet);
	EXPECT_EQ(req.getTarget(), "/");
	EXPECT_EQ(req.getProtocol(), "HTTP/1.1");
	EXPECT_EQ(req.getBody(), "Hello");
	EXPECT_EQ(req.getHeaders().at("host"), "www.example.com");
	EXPECT_EQ(req.getHeaders().at("content-length"), "5");
}

TEST_F(RequestTestFixture, Parse_FullRequestCopy) {
	Request	reqCopy;

	req.append(full_request, strlen(full_request));
	req.parseMessage();
	reqCopy = req;
	EXPECT_EQ(reqCopy.getMethod(), kGet);
	EXPECT_EQ(reqCopy.getTarget(), "/");
	EXPECT_EQ(reqCopy.getProtocol(), "HTTP/1.1");
	EXPECT_EQ(reqCopy.getBody(), "Hello");
	EXPECT_EQ(req.getHeaders().at("host"), "www.example.com");
	EXPECT_EQ(reqCopy.getHeaders().at("content-length"), "5");
}

TEST_F(RequestTestFixture, Parse_FullRequestClear) {
	req.append(full_request, strlen(full_request));
	req.parseMessage();
	req.clearData();
	EXPECT_EQ(req.getMethod(), kNone);
	EXPECT_EQ(req.getTarget(), "");
	EXPECT_EQ(req.getProtocol(), "");
	EXPECT_EQ(req.getBody(), "");
	EXPECT_TRUE(req.getHeaders().empty());
}

TEST_F(RequestTestFixture, Parse_OptionalWhitespace) {
	req.append(chunk1_OWS, strlen(chunk1_OWS));
	req.append(chunk2_OWS, strlen(chunk2_OWS));
	req.append(chunk3_OWS, strlen(chunk3_OWS));
	req.append(chunk4, strlen(chunk4));
	req.append(chunk5, strlen(chunk5));
	req.parseMessage();
	EXPECT_EQ(req.getMethod(), kGet);
	EXPECT_EQ(req.getTarget(), "/");
	EXPECT_EQ(req.getProtocol(), "HTTP/1.1");
	EXPECT_EQ(req.getBody(), "Hello");
	EXPECT_EQ(req.getHeaders().at("host"), "www.example.com");
	EXPECT_EQ(req.getHeaders().at("content-length"), "5");
}