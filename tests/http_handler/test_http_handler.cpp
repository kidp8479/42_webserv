#include <gtest/gtest.h>

#include "../../config/LocationConfig.hpp"
#include "../../config/ServerConfig.hpp"
#include "../../handlers/Handler.hpp"
#include "../../http/Request.hpp"
#include "../../http/Response.hpp"

// Build a Request exactly like Client does: feed raw bytes and let it parse
static Request makeRequest(const std::string& raw) {
    Request req;
    req.append(raw.c_str(), raw.size());
    return req;
}

// Base fixture: location "/" allowing GET, POST, DELETE
// root points to static test files (path relative to tests/bin/ where tests
// run)
class TestHandler : public ::testing::Test {
protected:
    void SetUp() {
        std::vector<std::string> methods;
        methods.push_back("GET");
        methods.push_back("POST");
        methods.push_back("DELETE");
        loc_.setPath("/");
        loc_.setRoot("../http_handler/static_test_files");
        loc_.setMethods(methods);
        server_.addLocationBlock(loc_);
    }

    LocationConfig loc_;
    ServerConfig server_;
    Response response_;
};

/* tests for run() - isError() branch
   [FAIL] => request parsing failed (Charlie set isError), sendError() is called
*/
TEST_F(TestHandler, MalformedRequestTriggersError) {
    Request req = makeRequest("GARBAGE THIS IS NOT HTTP\r\n\r\n");

    ASSERT_TRUE(req.isError());

    Handler::run(req, loc_, server_, response_);

    EXPECT_FALSE(response_.getRaw().empty());
    EXPECT_EQ(response_.getRaw().find("200"), std::string::npos);
}

/* tests for run() - 501 Not Implemented check
   [FAIL] => method is not GET, POST, or DELETE (PATCH, PUT, etc.)
   [PASS] => method is GET, POST, or DELETE
*/
TEST_F(TestHandler, UnknownMethodIs501) {
    Request req = makeRequest(
        "PATCH / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n");

    ASSERT_FALSE(req.isError());
    ASSERT_EQ(req.getMethod(), "PATCH");

    Handler::run(req, loc_, server_, response_);

    EXPECT_NE(response_.getRaw().find("501"), std::string::npos);
}

TEST_F(TestHandler, PutMethodIs501) {
    Request req = makeRequest(
        "PUT / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n");

    ASSERT_FALSE(req.isError());

    Handler::run(req, loc_, server_, response_);

    EXPECT_NE(response_.getRaw().find("501"), std::string::npos);
}

/* tests for run() - 405 Method Not Allowed check
   [FAIL] => method is valid (GET/POST/DELETE) but not listed in location's
   allowed methods
   [PASS] => method is listed in location's allowed methods (single or multiple)
*/
TEST_F(TestHandler, MethodNotInLocationIs405) {
    Request req = makeRequest(
        "DELETE / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n");

    ASSERT_FALSE(req.isError());
    ASSERT_EQ(req.getMethod(), "DELETE");

    // location that only allows GET
    LocationConfig loc;
    loc.setPath("/");
    std::vector<std::string> methods;
    methods.push_back("GET");
    loc.setMethods(methods);

    Handler::run(req, loc, server_, response_);

    EXPECT_NE(response_.getRaw().find("405"), std::string::npos);
}

TEST_F(TestHandler, MultipleMethodAuthorizedInvalid405) {
    Request req = makeRequest(
        "DELETE / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n");

    ASSERT_FALSE(req.isError());
    ASSERT_EQ(req.getMethod(), "DELETE");

    LocationConfig loc;
    loc.setPath("/");
    std::vector<std::string> methods;
    methods.push_back("GET");
    methods.push_back("POST");
    loc.setMethods(methods);

    Handler::run(req, loc, server_, response_);

    EXPECT_FALSE(response_.getRaw().empty());
    EXPECT_NE(response_.getRaw().find("405"), std::string::npos);
}

/* tests for run() - 500 Internal Server Error check
   [FAIL] => multiple discriminants set in the location block can't resolve
   properly, ambiguous block, internal server error
*/
TEST_F(TestHandler, AmbiguousLocationBlockIs500) {
    Request req = makeRequest(
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n");

    ASSERT_FALSE(req.isError());
    ASSERT_EQ(req.getMethod(), "GET");

    LocationConfig loc;
    std::vector<std::string> methods;
    methods.push_back("GET");
    loc.setMethods(methods);
    loc.setReturnCode(301);
    loc.setUploadPath("/upload");
    loc.addCgiInterpreter(".php", "whatever/this/is/a/test");

    Handler::run(req, loc, server_, response_);

    EXPECT_FALSE(response_.getRaw().empty());
    EXPECT_NE(response_.getRaw().find("500"), std::string::npos);
}

/* tests for handleStatic - serve regular file
   [PASS] => file exists on disk, 200 + correct Content-Type
   [FAIL] => file not found, 404
*/
TEST_F(TestHandler, StaticFileServed200) {
    Request req = makeRequest(
        "GET /hello.html HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n");

    ASSERT_FALSE(req.isError());

    Handler::run(req, loc_, server_, response_);

    EXPECT_NE(response_.getRaw().find("200"), std::string::npos);
    EXPECT_NE(response_.getRaw().find("text/html"), std::string::npos);
    EXPECT_NE(response_.getRaw().find("Hello from webserv"), std::string::npos);
}

TEST_F(TestHandler, StaticFileNotFoundIs404) {
    Request req = makeRequest(
        "GET /nonexistent.html HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n");

    ASSERT_FALSE(req.isError());

    Handler::run(req, loc_, server_, response_);

    EXPECT_NE(response_.getRaw().find("404"), std::string::npos);
}

/* tests for handleStatic - directory handling
   [PASS] => directory with index.html, serves the index
   [FAIL] => directory without index, listing off => 403
   [PASS] => directory without index, listing on => 200 + HTML list
*/
TEST_F(TestHandler, DirectoryWithIndexServed200) {
    Request req = makeRequest(
        "GET /subdir HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n");

    ASSERT_FALSE(req.isError());

    loc_.setIndex("index.html");

    Handler::run(req, loc_, server_, response_);

    EXPECT_NE(response_.getRaw().find("200"), std::string::npos);
    EXPECT_NE(response_.getRaw().find("Index page"), std::string::npos);
}

TEST_F(TestHandler, DirectoryNoIndexListingOffIs403) {
    Request req = makeRequest(
        "GET /emptydir HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n");

    ASSERT_FALSE(req.isError());

    // directory listing off by default, no index set
    Handler::run(req, loc_, server_, response_);

    EXPECT_NE(response_.getRaw().find("403"), std::string::npos);
}

TEST_F(TestHandler, DirectoryNoIndexListingOnServes200) {
    Request req = makeRequest(
        "GET /emptydir HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n");

    ASSERT_FALSE(req.isError());

    loc_.setDirectoryListing(true);

    Handler::run(req, loc_, server_, response_);

    EXPECT_NE(response_.getRaw().find("200"), std::string::npos);
    EXPECT_NE(response_.getRaw().find("text/html"), std::string::npos);
}

/* tests for sendError - custom error page
   [PASS] => error code has a configured page on disk, serves that file
   [FAIL] => configured page not on disk, falls back to hardcoded HTML
*/
TEST_F(TestHandler, CustomErrorPageServedWhenConfigured) {
    Request req = makeRequest(
        "GET /nonexistent.html HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n");

    ASSERT_FALSE(req.isError());

    server_.addErrorPage(404,
                         "../http_handler/static_test_files/custom_404.html");

    Handler::run(req, loc_, server_, response_);

    EXPECT_NE(response_.getRaw().find("200"), std::string::npos);
    EXPECT_NE(response_.getRaw().find("Custom 404"), std::string::npos);
}

TEST_F(TestHandler, MissingCustomErrorPageFallsBackToHardcoded) {
    Request req = makeRequest(
        "GET /nonexistent.html HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n");

    ASSERT_FALSE(req.isError());

    server_.addErrorPage(
        404, "../http_handler/static_test_files/does_not_exist.html");

    Handler::run(req, loc_, server_, response_);

    EXPECT_NE(response_.getRaw().find("404"), std::string::npos);
    EXPECT_NE(response_.getRaw().find("Not Found"), std::string::npos);
}
