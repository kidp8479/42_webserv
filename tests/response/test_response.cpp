#include <gtest/gtest.h>
#include <string.h>

#include "../../http/Response.hpp"

TEST(ResponseTest, Constructor_initialRespIsClean) {
    // Construction initializes all values as empty
    Response resp;
    EXPECT_TRUE(resp.getRaw().empty());
}

// Test Request copy constructor
TEST(ResponseTest, CopyConstructor_copyCleanResp) {
    // Copy constructor should copy all empty values
    Response resp;
    Response respCopy(resp);

    EXPECT_TRUE(respCopy.getRaw().empty());
}

class ResponseTestFixture : public ::testing::Test {
protected:
    Response resp;

    // Full response example
    const char* full_response =
        "HTTP/1.1 200 OK\r\n"
        "Server: Webserv\r\n"
        "Content-Length: 5\r\n"
        "\r\n"
        "Hello";

    // Standard error example
    const char* error_response =
        "HTTP/1.1 404 Not Found\r\n"
        "Content-Length: 48\r\n"
        "\r\n"
        "<html><body><h1>404 Not Found</h1></body></html>";
};

/********************************* Parsing **********************************/

TEST_F(ResponseTestFixture, Set_Status) {
    resp.setStatus(200, "OK");
    EXPECT_EQ(resp.getRaw(), "HTTP/1.1 200 OK\r\n\r\n");
}

TEST_F(ResponseTestFixture, Set_StatusNoReason) {
    resp.setStatus(200, "");
    EXPECT_EQ(resp.getRaw(), "HTTP/1.1 200\r\n\r\n");
}

TEST_F(ResponseTestFixture, Set_StatusError) {
    resp.setStatus(HttpConstants::kNotFound);
    EXPECT_EQ(resp.getRaw(), "HTTP/1.1 404 Not Found\r\n\r\n");
}

TEST_F(ResponseTestFixture, Set_StatusOverride) {
    resp.setStatus(200, "OK");
    EXPECT_EQ(resp.getRaw(), "HTTP/1.1 200 OK\r\n\r\n");
    resp.setStatus(201, "Created");
    EXPECT_EQ(resp.getRaw(), "HTTP/1.1 201 Created\r\n\r\n");
}

TEST_F(ResponseTestFixture, Set_Header) {
    resp.setHeader("Server", "Webserv");
    EXPECT_EQ(resp.getRaw(), "Server: Webserv\r\n\r\n");
}

TEST_F(ResponseTestFixture, Set_HeadersPlural) {
    resp.setHeader("Server", "Webserv");
    EXPECT_EQ(resp.getRaw(), "Server: Webserv\r\n\r\n");
    resp.setHeader("Content-Type", "text");
    resp.setHeader("Content-Length", "5");
    EXPECT_EQ(resp.getRaw(),
              "Server: Webserv\r\nContent-Type: text\r\n"
              "Content-Length: 5\r\n\r\n");
}

TEST_F(ResponseTestFixture, Set_Body) {
    resp.setBody("Hello");
    EXPECT_EQ(resp.getRaw(), "\r\nHello");
}

TEST_F(ResponseTestFixture, Set_BodyOverride) {
    resp.setBody("Hello");
    EXPECT_EQ(resp.getRaw(), "\r\nHello");
    resp.setBody("What?");
    EXPECT_EQ(resp.getRaw(), "\r\nWhat?");
}

TEST_F(ResponseTestFixture, Set_FullResponse) {
    resp.setStatus(200, "OK");
    resp.setHeader("Server", "Webserv");
    resp.setHeader("Content-Length", "5");
    resp.setBody("Hello");
    EXPECT_EQ(resp.getRaw(), full_response);
}

TEST_F(ResponseTestFixture, Build_Error) {
    resp.buildError(404, "Not Found");
    EXPECT_EQ(resp.getRaw(), error_response);
}

TEST_F(ResponseTestFixture, Build_ErrorFromError) {
    resp.buildError(HttpConstants::kNotFound);
    EXPECT_EQ(resp.getRaw(), error_response);
}

TEST_F(ResponseTestFixture, Set_Reset) {
    resp.setRaw(full_response);
    EXPECT_EQ(resp.getRaw(), full_response);
    resp.reset();
    EXPECT_TRUE(resp.getRaw().empty());
}

TEST_F(ResponseTestFixture, Copy_FullResponse) {
    resp.setRaw(full_response);
    Response respCopy = resp;
    resp.reset();
    EXPECT_TRUE(resp.getRaw().empty());
    EXPECT_EQ(respCopy.getRaw(), full_response);
}