#include <gtest/gtest.h>
#include <string.h>

#include "../../http/Request.hpp"

TEST(RequestTest, Constructor_initialStateIsClean) {
    // Construction initializes all values as empty
    Request req;

    EXPECT_EQ(req.getMethod(), "");
    EXPECT_EQ(req.getTarget(), "");
    EXPECT_EQ(req.getProtocol(), "");
    EXPECT_EQ(req.getBody(), "");
    EXPECT_TRUE(req.getHeaders().empty());

    EXPECT_FALSE(req.isComplete());
    EXPECT_FALSE(req.isError());
    EXPECT_EQ(req.getErrorCode(), 0);
}

// Test Request copy constructor
TEST(RequestTest, CopyConstructor_copyCleanReq) {
    // Copy constructor should copy all empty values
    Request req;
    Request reqCopy(req);

    EXPECT_EQ(reqCopy.getMethod(), "");
    EXPECT_EQ(reqCopy.getTarget(), "");
    EXPECT_EQ(reqCopy.getProtocol(), "");
    EXPECT_EQ(reqCopy.getBody(), "");
    EXPECT_TRUE(reqCopy.getHeaders().empty());

    EXPECT_FALSE(reqCopy.isComplete());
    EXPECT_FALSE(reqCopy.isError());
    EXPECT_EQ(reqCopy.getErrorCode(), 0);
}

class RequestTestFixture : public ::testing::Test {
protected:
    Request req;

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

    // Transfer-Encoding body
    const char* transfer_encode_header = "Transfer-Encoding: chunked\r\n";
    const char* transfer_encode_body =
        "5\r\n"
        "Hello\r\n"
        "7\r\n"
        "1234567\r\n"
        "0\r\n"
        "\r\n";
    const char* transfer_encode_nonull =
        "A\r\n"
        "Unfinished\r\n";
    const char* transfer_encode_null =
        "0\r\n"
        "\r\n";
    const char* transfer_encode_missing =
        "4\r\n"
        "Wait\r\n"
        "A\r\n"
        "Unfinished\r";

    // Connection header
    const char* connect_keep = "Connection: keep-alive\r\n";
    const char* connect_close = "Connection: close\r\n";

    // chunk variants - HTTP/1.0
    const char* chunk1_p0 = "GET / HTTP/1.0\r\n";

    // chunk variants - target with path and query
    const char* chunk1_pq = "GET /path-name?query=words HTTP/1.1\r\n";

    // chunk variants - optional whitespace
    const char* chunk1_OWS = "   GET  /    HTTP/1.1   \r\n";
    const char* chunk2_OWS = "Host:     www.example.com      \r\n";
    const char* chunk3_OWS = "Content-Length:      5      \r\n";

    // chunk variants - header case variance
    const char* chunk2_casemix = "hOsT: www.example.com\r\n";
    const char* chunk3_casemix = "cOnTEnT-lENgTH: 5\r\n";

    // chunk variants - broken chunks
    const char* chunk1_broken1 = "GET / \r\n";
    const char* chunk1_broken2 = "GET / HTTP/1.1 garbage\r\n";
    const char* chunk1_broken3 = "GET badtarget HTTP/1.1\r\n";
    const char* chunk1_broken4 = "GET / HTTP/9.1\r\n";

    // chunk variants - chunk that contains body and start line
    const char* chunk_endstart =
        "Hello"
        "POST /target HTTP/1.0\r\n";

    // chunk variants - evil misleading headers
    const char* chunk_evil1 = "Cookie: $EvilString=Content-Length:5\r\n";

    // request with cookies
    const char* cookie_request =
        "GET / HTTP/1.1\r\n"
        "Host: www.example.com\r\n"
        "Cookie: theme=dark; login=name; test=42\r\n"
        "Not-Cookie: This, Is, Not, A, Cookie\r\n"
        "\r\n";

    void SetUp() override {
        req.clearData();
    }

    void TearDown() {
    }
};

/********************************* Parsing **********************************/

TEST_F(RequestTestFixture, Parse_StartLine) {
    // Appending just the start line extracts method, target, and protocol
    req.append(chunk1, strlen(chunk1));
    EXPECT_EQ(req.getMethod(), "GET");
    EXPECT_EQ(req.getTarget(), "/");
    EXPECT_EQ(req.getProtocol(), "HTTP/1.1");
}

TEST_F(RequestTestFixture, Parse_Headers) {
    // Appending start line and headers extracts the headers
    req.append(chunk1, strlen(chunk1));
    req.append(chunk2, strlen(chunk2));
    req.append(chunk3, strlen(chunk3));
    EXPECT_EQ(req.getHeaderValue("host"), "www.example.com");
    EXPECT_EQ(req.getHeaderValue("content-length"), "5");
}

TEST_F(RequestTestFixture, Parse_FullRequest) {
    // Parsing a valid message extracts matching data
    req.append(full_request, strlen(full_request));
    EXPECT_EQ(req.getMethod(), "GET");
    EXPECT_EQ(req.getTarget(), "/");
    EXPECT_EQ(req.getProtocol(), "HTTP/1.1");
    EXPECT_EQ(req.getBody(), "Hello");
    EXPECT_EQ(req.getHeaderValue("content-length"), "5");
}

TEST_F(RequestTestFixture, Parse_AllChunks) {
    // Parsing a valid message delivered in chunks extracts matching data
    req.append(chunk1, strlen(chunk1));
    req.append(chunk2, strlen(chunk2));
    req.append(chunk3, strlen(chunk3));
    req.append(chunk4, strlen(chunk4));
    req.append(chunk5, strlen(chunk5));
    EXPECT_EQ(req.getMethod(), "GET");
    EXPECT_EQ(req.getTarget(), "/");
    EXPECT_EQ(req.getProtocol(), "HTTP/1.1");
    EXPECT_EQ(req.getBody(), "Hello");
    EXPECT_EQ(req.getHeaderValue("host"), "www.example.com");
    EXPECT_EQ(req.getHeaderValue("content-length"), "5");
}

TEST_F(RequestTestFixture, Parse_TransferEncodedBody) {
    // Parsing a valid message using the chunked transfer-encoding
    // method for the body extracts matching data
    req.append(chunk1, strlen(chunk1));
    req.append(chunk2, strlen(chunk2));
    req.append(transfer_encode_header, strlen(transfer_encode_header));
    req.append(chunk4, strlen(chunk4));
    req.append(transfer_encode_body, strlen(transfer_encode_body));
    EXPECT_EQ(req.getMethod(), "GET");
    EXPECT_EQ(req.getTarget(), "/");
    EXPECT_EQ(req.getProtocol(), "HTTP/1.1");
    EXPECT_EQ(req.getBody(), "Hello1234567");
    EXPECT_EQ(req.getHeaderValue("host"), "www.example.com");
    EXPECT_EQ(req.getHeaderValue("transfer-encoding"), "chunked");
}

TEST_F(RequestTestFixture, Parse_TransferEncodedBodyOverride) {
    // Transfer-Encoding overrides Content-Length
    req.append(chunk1, strlen(chunk1));
    req.append(chunk2, strlen(chunk2));
    req.append(chunk3, strlen(chunk3));
    req.append(transfer_encode_header, strlen(transfer_encode_header));
    req.append(chunk4, strlen(chunk4));
    req.append(transfer_encode_body, strlen(transfer_encode_body));
    EXPECT_EQ(req.getMethod(), "GET");
    EXPECT_EQ(req.getTarget(), "/");
    EXPECT_EQ(req.getProtocol(), "HTTP/1.1");
    EXPECT_EQ(req.getBody(), "Hello1234567");
    EXPECT_EQ(req.getHeaderValue("host"), "www.example.com");
    EXPECT_EQ(req.getHeaderValue("transfer-encoding"), "chunked");
    EXPECT_EQ(req.getHeaderValue("content-length"), "5");
}

TEST_F(RequestTestFixture, Parse_GetPathQuery) {
    req.append(chunk1_pq, strlen(chunk1_pq));
    EXPECT_EQ(req.getMethod(), "GET");
    EXPECT_EQ(req.getTarget(), "/path-name?query=words");
    EXPECT_EQ(req.getProtocol(), "HTTP/1.1");
    EXPECT_EQ(req.getPath(), "/path-name");
    EXPECT_EQ(req.getQuery(), "query=words");
}

TEST_F(RequestTestFixture, Parse_GetPathNoQuery) {
    req.append(chunk1, strlen(chunk1));
    EXPECT_EQ(req.getMethod(), "GET");
    EXPECT_EQ(req.getTarget(), "/");
    EXPECT_EQ(req.getProtocol(), "HTTP/1.1");
    EXPECT_EQ(req.getPath(), "/");
    EXPECT_EQ(req.getQuery(), "");
}

TEST_F(RequestTestFixture, Parse_FullRequestClear) {
    // Extracting data then clearing the data should leave all values empty
    req.append(full_request, strlen(full_request));
    req.clearData();
    EXPECT_EQ(req.getMethod(), "");
    EXPECT_EQ(req.getTarget(), "");
    EXPECT_EQ(req.getProtocol(), "");
    EXPECT_EQ(req.getBody(), "");
    EXPECT_TRUE(req.getHeaders().empty());
}

TEST_F(RequestTestFixture, Parse_FullRequestCopy) {
    // After extracting data, Request copies contain the same data
    Request reqCopy;

    req.append(full_request, strlen(full_request));
    reqCopy = req;
    EXPECT_EQ(reqCopy.getMethod(), "GET");
    EXPECT_EQ(reqCopy.getTarget(), "/");
    EXPECT_EQ(reqCopy.getProtocol(), "HTTP/1.1");
    EXPECT_EQ(reqCopy.getBody(), "Hello");
    EXPECT_EQ(reqCopy.getHeaders().at("host"), "www.example.com");
    EXPECT_EQ(reqCopy.getHeaders().at("content-length"), "5");
}

TEST_F(RequestTestFixture, Parse_FullRequestCopyClear) {
    // Clearing data from request does not clear it from copies
    Request reqCopy;

    req.append(full_request, strlen(full_request));
    reqCopy = req;
    req.clearData();
    EXPECT_EQ(reqCopy.getMethod(), "GET");
    EXPECT_EQ(reqCopy.getTarget(), "/");
    EXPECT_EQ(reqCopy.getProtocol(), "HTTP/1.1");
    EXPECT_EQ(reqCopy.getBody(), "Hello");
    EXPECT_EQ(reqCopy.getHeaders().at("host"), "www.example.com");
    EXPECT_EQ(reqCopy.getHeaders().at("content-length"), "5");
}

TEST_F(RequestTestFixture, Parse_OptionalWhitespace) {
    // Parsing messages with valid optional whitespace extracts matching data
    req.append(chunk1_OWS, strlen(chunk1_OWS));
    req.append(chunk2_OWS, strlen(chunk2_OWS));
    req.append(chunk3_OWS, strlen(chunk3_OWS));
    req.append(chunk4, strlen(chunk4));
    req.append(chunk5, strlen(chunk5));
    EXPECT_EQ(req.getMethod(), "GET");
    EXPECT_EQ(req.getTarget(), "/");
    EXPECT_EQ(req.getProtocol(), "HTTP/1.1");
    EXPECT_EQ(req.getBody(), "Hello");
    EXPECT_EQ(req.getHeaderValue("host"), "www.example.com");
    EXPECT_EQ(req.getHeaderValue("content-length"), "5");
}

TEST_F(RequestTestFixture, Parse_CaseInsensitive) {
    // Parsing a valid message delivered in chunks extracts matching data
    // regardless of case used in header names
    req.append(chunk1, strlen(chunk1));
    req.append(chunk2_casemix, strlen(chunk2_casemix));
    req.append(chunk3_casemix, strlen(chunk3_casemix));
    req.append(chunk4, strlen(chunk4));
    req.append(chunk5, strlen(chunk5));
    EXPECT_EQ(req.getMethod(), "GET");
    EXPECT_EQ(req.getTarget(), "/");
    EXPECT_EQ(req.getProtocol(), "HTTP/1.1");
    EXPECT_EQ(req.getBody(), "Hello");
    EXPECT_EQ(req.getHeaderValue("host"), "www.example.com");
    EXPECT_EQ(req.getHeaderValue("content-length"), "5");
}

TEST_F(RequestTestFixture, Parse_CutBody) {
    // Using a "Content-Length" with a size smaller than the body in the
    // message will cut the extracted body string to match sizes
    req.append(chunk1, strlen(chunk1));
    req.append(chunk2, strlen(chunk2));
    req.append("Content-Length: 4\r\n", 19);
    req.append(chunk4, strlen(chunk4));
    req.append(chunk5, strlen(chunk5));
    EXPECT_EQ(req.getMethod(), "GET");
    EXPECT_EQ(req.getTarget(), "/");
    EXPECT_EQ(req.getProtocol(), "HTTP/1.1");
    EXPECT_EQ(req.getBody(), "Hell");
    EXPECT_EQ(req.getHeaderValue("content-length"), "4");
}

TEST_F(RequestTestFixture, Parse_DeceptiveContentLen) {
    // Despite a misleading instance of "Content-Length", the correct size
    // will be used for the body
    req.append(chunk1, strlen(chunk1));
    req.append(chunk2, strlen(chunk2));
    req.append(chunk_evil1, strlen(chunk_evil1));
    req.append("Content-Length: 4\r\n", 19);
    req.append(chunk4, strlen(chunk4));
    req.append(chunk5, strlen(chunk5));
    EXPECT_EQ(req.getMethod(), "GET");
    EXPECT_EQ(req.getTarget(), "/");
    EXPECT_EQ(req.getProtocol(), "HTTP/1.1");
    EXPECT_EQ(req.getBody(), "Hell");
    EXPECT_EQ(req.getHeaderValue("content-length"), "4");
}

TEST_F(RequestTestFixture, Parse_EmptyLineStart) {
    // A single empty line at the beginning is accepted
    req.append(chunk4, strlen(chunk4));
    req.append(chunk1, strlen(chunk1));
    req.append(chunk2, strlen(chunk2));
    req.append(chunk3, strlen(chunk3));
    req.append(chunk4, strlen(chunk4));
    req.append(chunk5, strlen(chunk5));
    EXPECT_EQ(req.getMethod(), "GET");
    EXPECT_EQ(req.getTarget(), "/");
    EXPECT_EQ(req.getProtocol(), "HTTP/1.1");
    EXPECT_EQ(req.getBody(), "Hello");
    EXPECT_EQ(req.getHeaderValue("host"), "www.example.com");
    EXPECT_EQ(req.getHeaderValue("content-length"), "5");
}

TEST_F(RequestTestFixture, Parse_BodyEndsInNewline) {
    // A body ending in a newline should be parsed with it
    req.append(chunk1, strlen(chunk1));
    req.append(chunk2, strlen(chunk2));
    req.append("Content-Length: 7\r\n", 19);
    req.append(chunk4, strlen(chunk4));
    req.append(chunk5, strlen(chunk5));
    req.append(chunk4, strlen(chunk4));
    EXPECT_EQ(req.getBody(), "Hello\r\n");
}

TEST_F(RequestTestFixture, Parse_StartLineMissingToken) {
    // Start lines with missing tokens should return an error
    req.append(chunk1_broken1, strlen(chunk1_broken1));
    EXPECT_TRUE(req.isError());
}

TEST_F(RequestTestFixture, Parse_StartLineGarbageToken) {
    // Start lines with more than 3 tokens should return an error
    req.append(chunk1_broken2, strlen(chunk1_broken2));
    EXPECT_TRUE(req.isError());
}

TEST_F(RequestTestFixture, Parse_StartLineBadTarget) {
    // Start lines with invalid target syntax return an error
    req.append(chunk1_broken3, strlen(chunk1_broken3));
    EXPECT_TRUE(req.isError());
}

TEST_F(RequestTestFixture, Parse_StartLineBadProtocol) {
    // Start lines with unrecognized protocols return an error
    req.append(chunk1_broken4, strlen(chunk1_broken4));
    EXPECT_TRUE(req.isError());
}

TEST_F(RequestTestFixture, Parse_HeaderEqualsMaxSize) {
    // A header that is exactly equal to the max header size should pass
    req.setMaxHeaderSize(15);
    req.append(full_request, strlen(full_request));
    EXPECT_EQ(req.getHeaderValue("host"), "www.example.com");
    EXPECT_EQ(req.getHeaderValue("content-length"), "5");
    EXPECT_FALSE(req.isError());
}

TEST_F(RequestTestFixture, Parse_HeaderInvalidSize) {
    // A header that is greater than the max header size should error
    req.setMaxHeaderSize(14);
    req.append(full_request, strlen(full_request));
    EXPECT_TRUE(req.isError());
}

TEST_F(RequestTestFixture, Parse_BodyEqualsMaxSize) {
    // A body that is exactly equal to the max body size should pass
    req.setMaxBodySize(5);
    req.append(full_request, strlen(full_request));
    EXPECT_EQ(req.getBody(), "Hello");
    EXPECT_FALSE(req.isError());
}

TEST_F(RequestTestFixture, Parse_BodyInvalidSize) {
    // A body that is greater than the max body size should error
    req.setMaxBodySize(4);
    req.append(full_request, strlen(full_request));
    EXPECT_TRUE(req.isError());
}

TEST_F(RequestTestFixture, Parse_KeepAlive) {
    // If the raw buffer contains a complete message AND the start line
    // of the next message, resetData() should begin parsing the next
    req.append(chunk1, strlen(chunk1));
    req.append(chunk2, strlen(chunk2));
    req.append(chunk3, strlen(chunk3));
    req.append(chunk4, strlen(chunk4));
    req.append(chunk_endstart, strlen(chunk_endstart));
    EXPECT_EQ(req.getBody(), "Hello");
    EXPECT_TRUE(req.isComplete());
    EXPECT_TRUE(req.shouldKeepAlive());
    req.resetData();
    EXPECT_EQ(req.getMethod(), "POST");
    EXPECT_EQ(req.getTarget(), "/target");
    EXPECT_EQ(req.getProtocol(), "HTTP/1.0");
    EXPECT_EQ(req.getBody(), "");
    EXPECT_FALSE(req.isComplete());
}

/********************************* Complete **********************************/

TEST_F(RequestTestFixture, isComplete_FullRequest) {
    // A valid message should be considered complete
    req.append(full_request, strlen(full_request));
    EXPECT_TRUE(req.isComplete());
    EXPECT_FALSE(req.isError());
}

TEST_F(RequestTestFixture, isComplete_AllChunks) {
    // A valid message delivered in chunks should be considered complete
    req.append(chunk1, strlen(chunk1));
    req.append(chunk2, strlen(chunk2));
    req.append(chunk3, strlen(chunk3));
    req.append(chunk4, strlen(chunk4));
    req.append(chunk5, strlen(chunk5));
    EXPECT_TRUE(req.isComplete());
    EXPECT_FALSE(req.isError());
}

TEST_F(RequestTestFixture, isComplete_NoEmptyLine) {
    // A message with no empty line should be considered incomplete
    req.append(chunk1, strlen(chunk1));
    req.append(chunk2, strlen(chunk2));
    req.append(chunk3, strlen(chunk3));
    req.append(chunk5, strlen(chunk5));
    EXPECT_FALSE(req.isComplete());
    EXPECT_FALSE(req.isError());
}

TEST_F(RequestTestFixture, isComplete_AddEmptyLine) {
    // Adding an empty line after the isComplete check should pass
    req.append(chunk1, strlen(chunk1));
    req.append(chunk2, strlen(chunk2));
    EXPECT_FALSE(req.isComplete());
    EXPECT_FALSE(req.isError());
    req.append(chunk4, strlen(chunk4));
    EXPECT_TRUE(req.isComplete());
    EXPECT_FALSE(req.isError());
}

TEST_F(RequestTestFixture, isComplete_NoHeadersNoBody) {
    // A valid message without headers or body should be considered complete
    //(HTTP/1.0 is used because a lack of Host header is otherwise an error)
    req.append(chunk1_p0, strlen(chunk1_p0));
    req.append(chunk4, strlen(chunk4));
    EXPECT_TRUE(req.isComplete());
    EXPECT_FALSE(req.isError());
}

TEST_F(RequestTestFixture, isComplete_NoBody) {
    // A valid message without a body should be considered complete
    req.append(chunk1, strlen(chunk1));
    req.append(chunk2, strlen(chunk2));
    req.append(chunk4, strlen(chunk4));
    EXPECT_TRUE(req.isComplete());
    EXPECT_FALSE(req.isError());
}

TEST_F(RequestTestFixture, isComplete_AddBody) {
    // A message with a "Content-length" header should be considered
    // incomplete until a body of matching size is added
    req.append(chunk1, strlen(chunk1));
    req.append(chunk2, strlen(chunk2));
    req.append(chunk3, strlen(chunk3));
    req.append(chunk4, strlen(chunk4));
    EXPECT_FALSE(req.isComplete());
    EXPECT_FALSE(req.isError());
    req.append(chunk5, strlen(chunk5));
    EXPECT_TRUE(req.isComplete());
    EXPECT_FALSE(req.isError());
}

TEST_F(RequestTestFixture, isComplete_AddBodyOptionalWhitespace) {
    // A message with a "Content-length" value with optional whitespaces
    // should be considered incomplete until a body of matching size is added
    req.append(chunk1, strlen(chunk1));
    req.append(chunk2, strlen(chunk2));
    req.append(chunk3_OWS, strlen(chunk3_OWS));
    req.append(chunk4, strlen(chunk4));
    EXPECT_FALSE(req.isComplete());
    EXPECT_FALSE(req.isError());
    req.append(chunk5, strlen(chunk5));
    EXPECT_TRUE(req.isComplete());
    EXPECT_FALSE(req.isError());
}

TEST_F(RequestTestFixture, isComplete_ContentLenGreaterThanBody) {
    // A message with a "Content-Length" with a value greater than the body
    // size should be considered incomplete
    req.append(chunk1, strlen(chunk1));
    req.append(chunk2, strlen(chunk2));
    req.append("Content-Length: 6\r\n", 19);
    req.append(chunk4, strlen(chunk4));
    req.append(chunk5, strlen(chunk5));
    EXPECT_FALSE(req.isComplete());
    EXPECT_FALSE(req.isError());
}

TEST_F(RequestTestFixture, isComplete_ContentLenLesserThanBody) {
    // A message with a "Content-Length" with a value lesser than the body
    // size should be considered complete
    req.append(chunk1, strlen(chunk1));
    req.append(chunk2, strlen(chunk2));
    req.append("Content-Length: 4\r\n", 19);
    req.append(chunk4, strlen(chunk4));
    req.append(chunk5, strlen(chunk5));
    EXPECT_TRUE(req.isComplete());
    EXPECT_FALSE(req.isError());
}

TEST_F(RequestTestFixture, isComplete_NoBodyInvalidContentLen) {
    // A message with an invalid "Content-Length" value should
    // return an error
    req.append(chunk1, strlen(chunk1));
    req.append(chunk2, strlen(chunk2));
    req.append("Content-Length: abc\r\n", 21);
    req.append(chunk4, strlen(chunk4));
    EXPECT_TRUE(req.isComplete());
    EXPECT_TRUE(req.isError());
}

TEST_F(RequestTestFixture, isComplete_ContentLenCaseInsensitive) {
    // A valid message with "Content-Length" should be considered complete
    // regardless of the case used for the name
    req.append(chunk1, strlen(chunk1));
    req.append(chunk2_casemix, strlen(chunk2_casemix));
    req.append(chunk3_casemix, strlen(chunk3_casemix));
    req.append(chunk4, strlen(chunk4));
    req.append(chunk5, strlen(chunk5));
    EXPECT_TRUE(req.isComplete());
    EXPECT_FALSE(req.isError());
}

TEST_F(RequestTestFixture, isComplete_DeceptiveContentLen) {
    // A valid message without a body should be considered complete
    // even if it contains a misleading instance of "Content-Length"
    req.append(chunk1, strlen(chunk1));
    req.append(chunk2, strlen(chunk2));
    req.append(chunk_evil1, strlen(chunk_evil1));
    req.append(chunk4, strlen(chunk4));
    req.append(chunk5, strlen(chunk5));
    EXPECT_TRUE(req.isComplete());
    EXPECT_FALSE(req.isError());
}

TEST_F(RequestTestFixture, isComplete_NoNullChunked) {
    // A chunked-encoded body with no null terminator is not
    // considered complete but partially parses body
    req.append(chunk1, strlen(chunk1));
    req.append(chunk2, strlen(chunk2));
    req.append(transfer_encode_header, strlen(transfer_encode_header));
    req.append(chunk4, strlen(chunk4));
    req.append(transfer_encode_nonull, strlen(transfer_encode_nonull));
    EXPECT_EQ(req.getBody(), "Unfinished");
    EXPECT_FALSE(req.isComplete());
    EXPECT_FALSE(req.isError());

    // Appending the null chunk completes it
    req.append(transfer_encode_null, strlen(transfer_encode_null));
    EXPECT_EQ(req.getBody(), "Unfinished");
    EXPECT_TRUE(req.isComplete());
    EXPECT_FALSE(req.isError());
}

TEST_F(RequestTestFixture, isComplete_IncompleteChunked) {
    // A chunked-encoded body with an incomplete chunk doesn't complete
    // and only parses previous complete chunks
    req.append(chunk1, strlen(chunk1));
    req.append(chunk2, strlen(chunk2));
    req.append(transfer_encode_header, strlen(transfer_encode_header));
    req.append(chunk4, strlen(chunk4));
    req.append(transfer_encode_missing, strlen(transfer_encode_missing));
    EXPECT_EQ(req.getBody(), "Wait");
    EXPECT_FALSE(req.isComplete());
    EXPECT_FALSE(req.isError());

    // Malformed chunk returns an error
    req.append(transfer_encode_null, strlen(transfer_encode_null));
    EXPECT_TRUE(req.isError());
}

TEST_F(RequestTestFixture, isComplete_Nothing) {
    // An empty raw buffer should not be considered complete
    EXPECT_FALSE(req.isComplete());
    EXPECT_FALSE(req.isError());
}

TEST_F(RequestTestFixture, isComplete_SingleEmptyLine) {
    // A single empty line should not be considered complete
    req.append(chunk4, strlen(chunk4));
    EXPECT_FALSE(req.isComplete());
    EXPECT_FALSE(req.isError());
}

TEST_F(RequestTestFixture, isComplete_DoubleEmptyLine) {
    // Double empty lines with no start line shoud be icomplete and error
    req.append(chunk4, strlen(chunk4));
    req.append(chunk4, strlen(chunk4));
    EXPECT_TRUE(req.isComplete());
    EXPECT_TRUE(req.isError());
}

/******************************** Keep Alive *********************************/

TEST_F(RequestTestFixture, KeepAlive_HTTP1) {
    // Using protocol HTTP/1.1 keeps connections alive on completion
    req.append(full_request, strlen(full_request));
    EXPECT_TRUE(req.isComplete());
    EXPECT_TRUE(req.shouldKeepAlive());
}

TEST_F(RequestTestFixture, KeepAlive_HTTP0) {
    // Using protocol HTTP/1.0 keeps closes connection on completion
    req.append(chunk1_p0, strlen(chunk1_p0));
    req.append(chunk2, strlen(chunk2));
    req.append(chunk3, strlen(chunk3));
    req.append(chunk4, strlen(chunk4));
    req.append(chunk5, strlen(chunk5));
    EXPECT_TRUE(req.isComplete());
    EXPECT_FALSE(req.shouldKeepAlive());
}

TEST_F(RequestTestFixture, KeepAlive_ConnectionClose) {
    // Using "Connection: close" closes connection despite HTTP/1.1
    req.append(chunk1, strlen(chunk1));
    req.append(connect_close, strlen(connect_close));
    req.append(chunk2, strlen(chunk2));
    req.append(chunk3, strlen(chunk3));
    req.append(chunk4, strlen(chunk4));
    req.append(chunk5, strlen(chunk5));
    EXPECT_TRUE(req.isComplete());
    EXPECT_FALSE(req.shouldKeepAlive());
}

TEST_F(RequestTestFixture, KeepAlive_ConnectionKeep) {
    // Using "Connection: keep-alive" keeps connection despite HTTP/1.0
    req.append(chunk1_p0, strlen(chunk1_p0));
    req.append(connect_keep, strlen(connect_keep));
    req.append(chunk2, strlen(chunk2));
    req.append(chunk3, strlen(chunk3));
    req.append(chunk4, strlen(chunk4));
    req.append(chunk5, strlen(chunk5));
    EXPECT_TRUE(req.isComplete());
    EXPECT_TRUE(req.shouldKeepAlive());
}

TEST_F(RequestTestFixture, KeepAlive_ConnectionCloseOn400) {
    // A 400 Bad Request error should close connection and complete message
    req.append(chunk1, strlen(chunk1));
    req.append(chunk2, strlen(chunk2));
    req.append("Content-Length: abc\r\n", 21);
    req.append(chunk4, strlen(chunk4));
    EXPECT_EQ(req.getErrorCode(), 400);
    EXPECT_TRUE(req.isComplete());
    EXPECT_FALSE(req.shouldKeepAlive());
}

TEST_F(RequestTestFixture, KeepAlive_TransferEncodeCL) {
    // Having both Transfer-Encode and Content-Length closes connections
    // despite successful parsing and HTTP/1.1
    req.append(chunk1, strlen(chunk1));
    req.append(chunk2, strlen(chunk2));
    req.append(chunk3, strlen(chunk3));
    req.append(transfer_encode_header, strlen(transfer_encode_header));
    req.append(chunk4, strlen(chunk4));
    req.append(transfer_encode_body, strlen(transfer_encode_body));
    EXPECT_TRUE(req.isComplete());
    EXPECT_FALSE(req.isError());
    EXPECT_FALSE(req.shouldKeepAlive());
}

/******************************** Get Cookies ********************************/

TEST_F(RequestTestFixture, GetCookie_GetCookieList) {
    //Using getHeaderList on a cookie header returns the values in a vector
    req.append(cookie_request, strlen(cookie_request));
    EXPECT_TRUE(req.isComplete());
    EXPECT_FALSE(req.isError());
    EXPECT_TRUE(req.shouldKeepAlive());
    
    std::vector<std::string> cookieList = req.getHeaderList("Cookie");
    EXPECT_EQ(cookieList[0], "theme=dark");
    EXPECT_EQ(cookieList[1], "login=name");
    EXPECT_EQ(cookieList[2], "test=42");
}