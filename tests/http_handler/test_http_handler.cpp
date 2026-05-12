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

// Base fixture: one location "/" that allows GET, POST, DELETE
class TestHandler : public ::testing::Test {
protected:
    void SetUp() {
        LocationConfig loc;
        loc.setPath("/");
        std::vector<std::string> methods;
        methods.push_back("GET");
        methods.push_back("POST");
        methods.push_back("DELETE");
        loc.setMethods(methods);
        server_.addLocationBlock(loc);
    }

    ServerConfig server_;
    Response response_;
};

/* tests for run() - isError() branch
   [FAIL] => request parsing failed (Charlie set isError), sendError() is called
*/
TEST_F(TestHandler, MalformedRequestTriggersError) {
    Request req = makeRequest("GARBAGE THIS IS NOT HTTP\r\n\r\n");

    ASSERT_TRUE(req.isError());

    LocationConfig loc;
    loc.setPath("/");
    Handler::run(req, loc, server_, response_);

    // sendError() must write something - not empty, not the 200 stub
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

    LocationConfig loc;
    loc.setPath("/");
    Handler::run(req, loc, server_, response_);

    EXPECT_NE(response_.getRaw().find("501"), std::string::npos);
}

TEST_F(TestHandler, PutMethodIs501) {
    Request req = makeRequest(
        "PUT / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n");

    ASSERT_FALSE(req.isError());

    LocationConfig loc;
    loc.setPath("/");
    Handler::run(req, loc, server_, response_);

    EXPECT_NE(response_.getRaw().find("501"), std::string::npos);
}

/* tests for run() - 405 Method Not Allowed check
   [FAIL] => method is valid (GET/POST/DELETE) but not listed in location's
   allowed methods [PASS] => method is listed in location's allowed methods
   (single or multiple)
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

/* tests for run() - 500 Internal Server Error check
   [FAIL] => multiple discriminants set in the location block can't resolve
   properly, ambiguous block, internal server error. Don't let a weird block
   pass.
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

/* tests for run() - dispatch reached
   [PASS] => all checks pass, request reaches the dispatch (Hello World stub for
   now)
*/
TEST_F(TestHandler, ValidGetReachesDispatch) {
    Request req = makeRequest(
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n");

    ASSERT_FALSE(req.isError());
    ASSERT_EQ(req.getMethod(), "GET");

    LocationConfig loc;
    loc.setPath("/");
    std::vector<std::string> methods;
    methods.push_back("GET");
    loc.setMethods(methods);

    Handler::run(req, loc, server_, response_);

    // stub still sets Hello World - test will evolve when handleStatic is done
    EXPECT_FALSE(response_.getRaw().empty());
    EXPECT_EQ(response_.getRaw().find("4"), std::string::npos);  // no 4xx
    EXPECT_EQ(response_.getRaw().find("5"), std::string::npos);  // no 5xx
}

TEST_F(TestHandler, MultipleMethodAuthorizedValidGet) {
    Request req = makeRequest(
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n");

    ASSERT_FALSE(req.isError());
    ASSERT_EQ(req.getMethod(), "GET");

    LocationConfig loc;
    loc.setPath("/");
    std::vector<std::string> methods;
    methods.push_back("GET");
    methods.push_back("POST");
    loc.setMethods(methods);

    Handler::run(req, loc, server_, response_);

    // stub still sets Hello World - test will evolve when handleStatic is done
    EXPECT_FALSE(response_.getRaw().empty());
    EXPECT_EQ(response_.getRaw().find("4"), std::string::npos);  // no 4xx
    EXPECT_EQ(response_.getRaw().find("5"), std::string::npos);  // no 5xx
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

    // stub still sets Hello World - test will evolve when handleStatic is done
    EXPECT_FALSE(response_.getRaw().empty());
    EXPECT_NE(response_.getRaw().find("405"),
              std::string::npos);  // look for 405
}