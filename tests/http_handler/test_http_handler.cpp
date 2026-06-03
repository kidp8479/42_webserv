#include <gtest/gtest.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fstream>

#include "../../config/LocationConfig.hpp"
#include "../../config/ServerConfig.hpp"
#include "../../core/Client.hpp"
#include "../../core/EventLoop.hpp"
#include "../../core/ServerResources.hpp"
#include "../../handlers/Handler.hpp"
#include "../../http/Request.hpp"
#include "../../http/Response.hpp"

static Request makeRequest(const std::string& raw) {
    Request req;
    req.append(raw.c_str(), raw.size());
    return req;
}

static ServerConfig makeDummyServerConfig() {
    ServerConfig sc;
    sc.setHost("127.0.0.1");
    sc.setPort(8080);
    return sc;
}

class TestHttpHandler : public ::testing::Test {
protected:
    int fds[2];
    ServerConfig server_config;
    ServerResources resources;
    EventLoop loop;
    Client* client;
    LocationConfig loc_;
    ServerConfig server_;

    TestHttpHandler()
        : server_config(makeDummyServerConfig()),
          resources(server_config),
          loop() {
    }

    void SetUp() {
        ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);
        client = new Client(fds[0], loop, resources, "127.0.0.1");

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
        delete client;
        close(fds[1]);
        std::remove(
            "../http_handler/static_test_files/uploads_test/test_file.txt");
    }

    // helper: run handler and get response raw string
    std::string run(const Request& req, const LocationConfig& loc) {
        Handler::run(req, loc, *client);
        return client->getResponse().getRaw();
    }
};

// --- error / method checks ---

TEST_F(TestHttpHandler, MalformedRequestTriggersError) {
    Request req = makeRequest("GARBAGE THIS IS NOT HTTP\r\n\r\n");
    ASSERT_TRUE(req.isError());
    std::string raw = run(req, loc_);
    EXPECT_FALSE(raw.empty());
    EXPECT_EQ(raw.find("200"), std::string::npos);
}

TEST_F(TestHttpHandler, UnknownMethodIs501) {
    Request req = makeRequest(
        "PATCH / HTTP/1.1\r\nHost: localhost\r\n\r\n");
    ASSERT_FALSE(req.isError());
    EXPECT_NE(run(req, loc_).find("501"), std::string::npos);
}

TEST_F(TestHttpHandler, PutIsNotImplemented) {
    Request req = makeRequest(
        "PUT / HTTP/1.1\r\nHost: localhost\r\n\r\n");
    ASSERT_FALSE(req.isError());
    EXPECT_NE(run(req, loc_).find("501"), std::string::npos);
}

TEST_F(TestHttpHandler, MethodNotInLocationIs405) {
    Request req = makeRequest(
        "DELETE / HTTP/1.1\r\nHost: localhost\r\n\r\n");
    ASSERT_FALSE(req.isError());
    LocationConfig loc;
    loc.setPath("/");
    std::vector<std::string> methods;
    methods.push_back("GET");
    loc.setMethods(methods);
    EXPECT_NE(run(req, loc).find("405"), std::string::npos);
}

TEST_F(TestHttpHandler, MethodNotInMultipleAllowedMethodsIs405) {
    Request req = makeRequest(
        "DELETE / HTTP/1.1\r\nHost: localhost\r\n\r\n");
    ASSERT_FALSE(req.isError());
    LocationConfig loc;
    loc.setPath("/");
    std::vector<std::string> methods;
    methods.push_back("GET");
    methods.push_back("POST");
    loc.setMethods(methods);
    EXPECT_NE(run(req, loc).find("405"), std::string::npos);
}

TEST_F(TestHttpHandler, AmbiguousLocationBlockIs500) {
    Request req = makeRequest(
        "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n");
    ASSERT_FALSE(req.isError());
    LocationConfig loc;
    std::vector<std::string> methods;
    methods.push_back("GET");
    loc.setMethods(methods);
    loc.setReturnCode(301);
    loc.setUploadPath("/upload");
    loc.addCgiInterpreter(".php", "whatever/this/is/a/test");
    EXPECT_NE(run(req, loc).find("500"), std::string::npos);
}

TEST_F(TestHttpHandler, PostOnStaticBlockIs500) {
    Request req = makeRequest(
        "POST /hello.html HTTP/1.1\r\nHost: localhost\r\nContent-Length: 0\r\n\r\n");
    ASSERT_FALSE(req.isError());
    EXPECT_NE(run(req, loc_).find("500"), std::string::npos);
}

// --- redirects ---

TEST_F(TestHttpHandler, Redirect301HasLocationHeader) {
    Request req = makeRequest(
        "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n");
    LocationConfig loc;
    std::vector<std::string> methods;
    methods.push_back("GET");
    loc.setMethods(methods);
    loc.setReturnCode(301);
    loc.setReturnUrl("https://example.com/new");
    std::string raw = run(req, loc);
    EXPECT_NE(raw.find("301"), std::string::npos);
    EXPECT_NE(raw.find("Location:"), std::string::npos);
    EXPECT_NE(raw.find("https://example.com/new"), std::string::npos);
}

TEST_F(TestHttpHandler, Redirect302HasLocationHeader) {
    Request req = makeRequest(
        "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n");
    LocationConfig loc;
    std::vector<std::string> methods;
    methods.push_back("GET");
    loc.setMethods(methods);
    loc.setReturnCode(302);
    loc.setReturnUrl("https://example.com/moved");
    std::string raw = run(req, loc);
    EXPECT_NE(raw.find("302"), std::string::npos);
    EXPECT_NE(raw.find("Location:"), std::string::npos);
}

TEST_F(TestHttpHandler, Redirect307HasLocationHeader) {
    Request req = makeRequest(
        "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n");
    LocationConfig loc;
    std::vector<std::string> methods;
    methods.push_back("GET");
    loc.setMethods(methods);
    loc.setReturnCode(307);
    loc.setReturnUrl("https://example.com/temp");
    std::string raw = run(req, loc);
    EXPECT_NE(raw.find("307"), std::string::npos);
    EXPECT_NE(raw.find("Location:"), std::string::npos);
}

TEST_F(TestHttpHandler, Redirect308HasLocationHeader) {
    Request req = makeRequest(
        "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n");
    LocationConfig loc;
    std::vector<std::string> methods;
    methods.push_back("GET");
    loc.setMethods(methods);
    loc.setReturnCode(308);
    loc.setReturnUrl("https://example.com/permanent");
    std::string raw = run(req, loc);
    EXPECT_NE(raw.find("308"), std::string::npos);
    EXPECT_NE(raw.find("Location:"), std::string::npos);
}

TEST_F(TestHttpHandler, UnknownRedirectCodeHasFallbackReason) {
    Request req = makeRequest(
        "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n");
    LocationConfig loc;
    std::vector<std::string> methods;
    methods.push_back("GET");
    loc.setMethods(methods);
    loc.setReturnCode(303);
    loc.setReturnUrl("https://example.com/other");
    std::string raw = run(req, loc);
    EXPECT_NE(raw.find("303"), std::string::npos);
    EXPECT_NE(raw.find("Redirect"), std::string::npos);
}

// --- static files ---

TEST_F(TestHttpHandler, DirectoryTraversalIs403) {
    Request req = makeRequest(
        "GET /../../etc/passwd HTTP/1.1\r\nHost: localhost\r\n\r\n");
    ASSERT_FALSE(req.isError());
    EXPECT_NE(run(req, loc_).find("403"), std::string::npos);
}

TEST_F(TestHttpHandler, StaticFileServed200) {
    Request req = makeRequest(
        "GET /hello.html HTTP/1.1\r\nHost: localhost\r\n\r\n");
    ASSERT_FALSE(req.isError());
    std::string raw = run(req, loc_);
    EXPECT_NE(raw.find("200"), std::string::npos);
    EXPECT_NE(raw.find("text/html"), std::string::npos);
    EXPECT_NE(raw.find("Hello from webserv"), std::string::npos);
}

TEST_F(TestHttpHandler, StaticFileNotFoundIs404) {
    Request req = makeRequest(
        "GET /nonexistent.html HTTP/1.1\r\nHost: localhost\r\n\r\n");
    ASSERT_FALSE(req.isError());
    EXPECT_NE(run(req, loc_).find("404"), std::string::npos);
}

TEST_F(TestHttpHandler, DirectoryWithIndexServed200) {
    Request req = makeRequest(
        "GET /subdir/ HTTP/1.1\r\nHost: localhost\r\n\r\n");
    ASSERT_FALSE(req.isError());
    loc_.setIndex("index.html");
    std::string raw = run(req, loc_);
    EXPECT_NE(raw.find("200"), std::string::npos);
    EXPECT_NE(raw.find("Index page"), std::string::npos);
}

TEST_F(TestHttpHandler, DirectoryWithoutTrailingSlashIs301) {
    Request req = makeRequest(
        "GET /emptydir HTTP/1.1\r\nHost: localhost\r\n\r\n");
    ASSERT_FALSE(req.isError());
    std::string raw = run(req, loc_);
    EXPECT_NE(raw.find("301"), std::string::npos);
    EXPECT_NE(raw.find("Location: /emptydir/"), std::string::npos);
}

TEST_F(TestHttpHandler, DirectoryNoIndexListingOffIs403) {
    Request req = makeRequest(
        "GET /emptydir/ HTTP/1.1\r\nHost: localhost\r\n\r\n");
    ASSERT_FALSE(req.isError());
    EXPECT_NE(run(req, loc_).find("403"), std::string::npos);
}

TEST_F(TestHttpHandler, DirectoryNoIndexListingOnServes200) {
    Request req = makeRequest(
        "GET /emptydir/ HTTP/1.1\r\nHost: localhost\r\n\r\n");
    ASSERT_FALSE(req.isError());
    loc_.setDirectoryListing(true);
    std::string raw = run(req, loc_);
    EXPECT_NE(raw.find("200"), std::string::npos);
    EXPECT_NE(raw.find("text/html"), std::string::npos);
}

// --- error pages ---

TEST_F(TestHttpHandler, CustomErrorPageServedWhenConfigured) {
    Request req = makeRequest(
        "GET /nonexistent.html HTTP/1.1\r\nHost: localhost\r\n\r\n");
    ASSERT_FALSE(req.isError());
    server_.addErrorPage(404,
        "../http_handler/static_test_files/custom_404.html");
    // rebuild client with updated server config
    ServerResources res2(server_);
    delete client;
    client = new Client(fds[0], loop, res2, "127.0.0.1");
    EXPECT_NE(run(req, loc_).find("Custom 404"), std::string::npos);
}

TEST_F(TestHttpHandler, MissingCustomErrorPageFallsBackToHardcoded) {
    Request req = makeRequest(
        "GET /nonexistent.html HTTP/1.1\r\nHost: localhost\r\n\r\n");
    ASSERT_FALSE(req.isError());
    server_.addErrorPage(404,
        "../http_handler/static_test_files/does_not_exist.html");
    ServerResources res2(server_);
    delete client;
    client = new Client(fds[0], loop, res2, "127.0.0.1");
    std::string raw = run(req, loc_);
    EXPECT_NE(raw.find("404"), std::string::npos);
    EXPECT_NE(raw.find("Not Found"), std::string::npos);
}

// --- delete ---

TEST_F(TestHttpHandler, DeleteFileReturns204AndRemovesFile) {
    std::string path =
        "../http_handler/static_test_files/delete_me.txt";
    std::ofstream f(path.c_str());
    f << "delete me";
    f.close();
    std::vector<std::string> methods;
    methods.push_back("GET");
    methods.push_back("DELETE");
    loc_.setMethods(methods);
    Request req = makeRequest(
        "DELETE /delete_me.txt HTTP/1.1\r\nHost: localhost\r\n\r\n");
    ASSERT_FALSE(req.isError());
    EXPECT_NE(run(req, loc_).find("204"), std::string::npos);
    struct stat info;
    EXPECT_NE(stat(path.c_str(), &info), 0);
}

// --- upload ---

TEST_F(TestHttpHandler, HandleUploadSuccessIs201) {
    Request req = makeRequest(
        "POST /upload_test/test_file.txt HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 13\r\n"
        "\r\n"
        "hello upload!");
    ASSERT_FALSE(req.isError());
    loc_.setUploadPath("../http_handler/static_test_files/uploads_test");
    EXPECT_NE(run(req, loc_).find("201"), std::string::npos);
    struct stat info;
    EXPECT_EQ(stat(
        "../http_handler/static_test_files/uploads_test/test_file.txt",
        &info), 0);
}

TEST_F(TestHttpHandler, HandleUploadInvalidPathIs500) {
    Request req = makeRequest(
        "POST /upload_test/test_file.txt HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 13\r\n"
        "\r\n"
        "hello upload!");
    ASSERT_FALSE(req.isError());
    loc_.setUploadPath(
        "../http_handler/static_test_files/does_not_exist");
    EXPECT_NE(run(req, loc_).find("500"), std::string::npos);
}
