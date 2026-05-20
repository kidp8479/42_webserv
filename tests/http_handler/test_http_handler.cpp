#include <gtest/gtest.h>
#include <sys/stat.h>

#include <fstream>

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
class TestHttpHandler : public ::testing::Test {
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

    void TearDown() {
        std::remove(
            "../http_handler/static_test_files/uploads_test/test_file.txt");
    }

    LocationConfig loc_;
    ServerConfig server_;
    Response response_;
};

/* tests for run() - isError() branch
   [FAIL] => request parsing failed (Charlie set isError), sendError() is called
*/
TEST_F(TestHttpHandler, MalformedRequestTriggersError) {
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
TEST_F(TestHttpHandler, UnknownMethodIs501) {
    Request req = makeRequest(
        "PATCH / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n");

    ASSERT_FALSE(req.isError());
    ASSERT_EQ(req.getMethod(), "PATCH");

    Handler::run(req, loc_, server_, response_);

    EXPECT_NE(response_.getRaw().find("501"), std::string::npos);
}

TEST_F(TestHttpHandler, PutIsNotImplemented) {
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
TEST_F(TestHttpHandler, MethodNotInLocationIs405) {
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

TEST_F(TestHttpHandler, MethodNotInMultipleAllowedMethodsIs405) {
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
TEST_F(TestHttpHandler, AmbiguousLocationBlockIs500) {
    Request req = makeRequest(
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n");

    ASSERT_FALSE(req.isError());
    ASSERT_EQ(req.getMethod(), "GET");

    LocationConfig loc;
    std::vector<std::string> methods;
    methods.push_back("GET");

    // this location block is a return/upload/cgi bloc all at the same time
    // I can't guess what the .conf file writter had in mind so : error 500
    loc.setMethods(methods);
    loc.setReturnCode(301);
    loc.setUploadPath("/upload");
    loc.addCgiInterpreter(".php", "whatever/this/is/a/test");

    Handler::run(req, loc, server_, response_);

    EXPECT_FALSE(response_.getRaw().empty());
    EXPECT_NE(response_.getRaw().find("500"), std::string::npos);
}

/* tests for handleReturn - redirect location block
   [PASS] => 301 redirect: status line contains 301, Location header set
   [PASS] => 302 redirect: status line contains 302, Location header set
   [PASS] => 307 redirect: status line contains 307, Location header set
   [PASS] => 308 redirect: status line contains 308, Location header set
   [PASS] => unknown code in range [300-399]: fallback reason, Location header
   set
*/
TEST_F(TestHttpHandler, Redirect301HasLocationHeader) {
    Request req = makeRequest(
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n");

    LocationConfig loc;
    std::vector<std::string> methods;
    methods.push_back("GET");
    loc.setMethods(methods);
    loc.setReturnCode(301);
    loc.setReturnUrl("https://example.com/new");

    Handler::run(req, loc, server_, response_);

    EXPECT_NE(response_.getRaw().find("301"), std::string::npos);
    EXPECT_NE(response_.getRaw().find("Location:"), std::string::npos);
    EXPECT_NE(response_.getRaw().find("https://example.com/new"),
              std::string::npos);
}

TEST_F(TestHttpHandler, Redirect302HasLocationHeader) {
    Request req = makeRequest(
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n");

    LocationConfig loc;
    std::vector<std::string> methods;
    methods.push_back("GET");
    loc.setMethods(methods);
    loc.setReturnCode(302);
    loc.setReturnUrl("https://example.com/moved");

    Handler::run(req, loc, server_, response_);

    EXPECT_NE(response_.getRaw().find("302"), std::string::npos);
    EXPECT_NE(response_.getRaw().find("Location:"), std::string::npos);
    EXPECT_NE(response_.getRaw().find("https://example.com/moved"),
              std::string::npos);
}

TEST_F(TestHttpHandler, Redirect307HasLocationHeader) {
    Request req = makeRequest(
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n");

    LocationConfig loc;
    std::vector<std::string> methods;
    methods.push_back("GET");
    loc.setMethods(methods);
    loc.setReturnCode(307);
    loc.setReturnUrl("https://example.com/temp");

    Handler::run(req, loc, server_, response_);

    EXPECT_NE(response_.getRaw().find("307"), std::string::npos);
    EXPECT_NE(response_.getRaw().find("Location:"), std::string::npos);
    EXPECT_NE(response_.getRaw().find("https://example.com/temp"),
              std::string::npos);
}

TEST_F(TestHttpHandler, Redirect308HasLocationHeader) {
    Request req = makeRequest(
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n");

    LocationConfig loc;
    std::vector<std::string> methods;
    methods.push_back("GET");
    loc.setMethods(methods);
    loc.setReturnCode(308);
    loc.setReturnUrl("https://example.com/permanent");

    Handler::run(req, loc, server_, response_);

    EXPECT_NE(response_.getRaw().find("308"), std::string::npos);
    EXPECT_NE(response_.getRaw().find("Location:"), std::string::npos);
    EXPECT_NE(response_.getRaw().find("https://example.com/permanent"),
              std::string::npos);
}

TEST_F(TestHttpHandler, UnknownRedirectCodeHasFallbackReason) {
    Request req = makeRequest(
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n");

    LocationConfig loc;
    std::vector<std::string> methods;
    methods.push_back("GET");
    loc.setMethods(methods);
    loc.setReturnCode(303);
    loc.setReturnUrl("https://example.com/other");

    Handler::run(req, loc, server_, response_);

    EXPECT_NE(response_.getRaw().find("303"), std::string::npos);
    EXPECT_NE(response_.getRaw().find("Location:"), std::string::npos);
    EXPECT_NE(response_.getRaw().find("Redirect"), std::string::npos);
}

/* tests for handleStatic - serve regular file
   [PASS] => file exists on disk, 200 + correct Content-Type
   [FAIL] => file not found, 404
   [FAIL] => path contains "..", directory traversal attempt => 403
*/
TEST_F(TestHttpHandler, DirectoryTraversalIs403) {
    Request req = makeRequest(
        "GET /../../etc/passwd HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n");

    ASSERT_FALSE(req.isError());

    Handler::run(req, loc_, server_, response_);

    EXPECT_NE(response_.getRaw().find("403"), std::string::npos);
}

TEST_F(TestHttpHandler, StaticFileServed200) {
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

TEST_F(TestHttpHandler, StaticFileNotFoundIs404) {
    Request req = makeRequest(
        "GET /nonexistent.html HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n");

    ASSERT_FALSE(req.isError());

    Handler::run(req, loc_, server_, response_);

    EXPECT_NE(response_.getRaw().find("404"), std::string::npos);
}

/* tests for handleStatic - directory handling
   [PASS] => directory with index file configured and present, serves index
   [FAIL] => directory, no index, listing off => 403
   [PASS] => directory, no index, listing on => 200 + HTML listing
   [PASS] => directory with index.html, serves the index
   [FAIL] => directory without index, listing off => 403
   [PASS] => directory without index, listing on => 200 + HTML list
*/
TEST_F(TestHttpHandler, DirectoryWithIndexServed200) {
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

TEST_F(TestHttpHandler, DirectoryNoIndexListingOffIs403) {
    Request req = makeRequest(
        "GET /emptydir HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n");

    ASSERT_FALSE(req.isError());

    // directory listing off by default, no index set
    Handler::run(req, loc_, server_, response_);

    EXPECT_NE(response_.getRaw().find("403"), std::string::npos);
}

TEST_F(TestHttpHandler, DirectoryNoIndexListingOnServes200) {
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
TEST_F(TestHttpHandler, CustomErrorPageServedWhenConfigured) {
    Request req = makeRequest(
        "GET /nonexistent.html HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n");

    ASSERT_FALSE(req.isError());

    server_.addErrorPage(404,
                         "../http_handler/static_test_files/custom_404.html");

    Handler::run(req, loc_, server_, response_);

    EXPECT_NE(response_.getRaw().find("404"), std::string::npos);
    EXPECT_NE(response_.getRaw().find("Custom 404"), std::string::npos);
}

TEST_F(TestHttpHandler, DeleteFileReturns204AndRemovesFile) {
    std::string path = "../http_handler/static_test_files/delete_me.txt";
    std::ofstream f(path.c_str());
    f << "delete me";
    f.close();

    std::vector<std::string> methods;
    methods.push_back("GET");
    methods.push_back("DELETE");
    loc_.setMethods(methods);

    Request req = makeRequest(
        "DELETE /delete_me.txt HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n");

    ASSERT_FALSE(req.isError());
    Handler::run(req, loc_, server_, response_);

    EXPECT_NE(response_.getRaw().find("204"), std::string::npos);

    struct stat info;
    EXPECT_NE(stat(path.c_str(), &info), 0);
}

TEST_F(TestHttpHandler, MissingCustomErrorPageFallsBackToHardcoded) {
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

/* tests for handleUpload
   [PASS] => upload success
   [FAIL] => upload failed
*/

TEST_F(TestHttpHandler, HandleUploadSuccessIs201) {
    Request req = makeRequest(
        "POST /upload_test/test_file.txt HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 13\r\n"
        "\r\n"
        "hello upload!");

    ASSERT_FALSE(req.isError());

    loc_.setUploadPath("../http_handler/static_test_files/uploads_test");

    Handler::run(req, loc_, server_, response_);

    EXPECT_NE(response_.getRaw().find("201"), std::string::npos);

    struct stat info;
    EXPECT_EQ(
        stat("../http_handler/static_test_files/uploads_test/test_file.txt",
             &info),
        0);
}

TEST_F(TestHttpHandler, HandleUploadInvalidPathIs500) {
    Request req = makeRequest(
        "POST /upload_test/test_file.txt HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 13\r\n"
        "\r\n"
        "hello upload!");

    ASSERT_FALSE(req.isError());

    loc_.setUploadPath("../http_handler/static_test_files/does_not_exist");

    Handler::run(req, loc_, server_, response_);

    EXPECT_NE(response_.getRaw().find("500"), std::string::npos);
}