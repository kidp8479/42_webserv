#include <gtest/gtest.h>
#include <string.h>

#include "../../http/Request.hpp"
#include "../../http/Response.hpp"

class RequestCookieTestFixture : public ::testing::Test {
protected:
    Request req;

    const char* request_start =
        "GET / HTTP/1.1\r\n"
        "Host: www.example.com\r\n";

    const char* request_end = "\r\n";

    const char* request_cookie =
        "Cookie: theme=dark; login=name; test=42\r\n";
    const char* request_cookie2 =
        "Cookie:  funds=2;      shape=circle  \r\n";
    
};

TEST_F(RequestCookieTestFixture, RequestCookie_GetCookieList) {
    req.append(request_start, strlen(request_start));
    req.append(request_cookie, strlen(request_cookie));
    req.append(request_end, strlen(request_end));
    
    std::vector<std::string> cookieList = req.getHeaderList("Cookie");
    EXPECT_EQ(cookieList.size(), (size_t) 3);
    EXPECT_EQ(cookieList[0], "theme=dark");
    EXPECT_EQ(cookieList[1], "login=name");
    EXPECT_EQ(cookieList[2], "test=42");
}

TEST_F(RequestCookieTestFixture, RequestCookie_MoreCookies) {
    req.append(request_start, strlen(request_start));
    req.append(request_cookie, strlen(request_cookie));
    req.append(request_cookie2, strlen(request_cookie2));
    req.append(request_end, strlen(request_end));
    
    std::vector<std::string> cookieList = req.getHeaderList("Cookie");
    EXPECT_EQ(cookieList.size(), (size_t) 5);
    EXPECT_EQ(cookieList[0], "theme=dark");
    EXPECT_EQ(cookieList[1], "login=name");
    EXPECT_EQ(cookieList[2], "test=42");
    EXPECT_EQ(cookieList[3], "funds=2");
    EXPECT_EQ(cookieList[4], "shape=circle");
}

TEST_F(RequestCookieTestFixture, RequestCookie_NoCookieList) {
    req.append(request_start, strlen(request_start));
    req.append(request_end, strlen(request_end));
    
    std::vector<std::string> cookieList = req.getHeaderList("Cookie");
    EXPECT_TRUE(cookieList.empty());
}

class ResponseCookieTestFixture : public ::testing::Test {
protected:
    Response resp;
};

TEST_F(ResponseCookieTestFixture, ResponseCookie_GetCookieList) {
    resp.setHeader("Set-Cookie", "name=bob");
    EXPECT_TRUE(resp.hasCookies());
    std::vector<std::string> cookieList = resp.getCookieList();
    EXPECT_EQ(cookieList.size(), (size_t) 1);
    EXPECT_EQ(cookieList[0], "name=bob");
}

TEST_F(ResponseCookieTestFixture, ResponseCookie_TrimCookie) {
    resp.setHeader("Set-Cookie", " name = bob ");
    EXPECT_TRUE(resp.hasCookies());
    std::vector<std::string> cookieList = resp.getCookieList();
    EXPECT_EQ(cookieList.size(), (size_t) 1);
    EXPECT_EQ(cookieList[0], "name=bob");
}

TEST_F(ResponseCookieTestFixture, ResponseCookie_IgnoreExtra) {
    resp.setHeader("Set-Cookie", "name=bob; Max-Age=200; HttpOnly");
    EXPECT_TRUE(resp.hasCookies());
    std::vector<std::string> cookieList = resp.getCookieList();
    EXPECT_EQ(cookieList.size(), (size_t) 1);
    EXPECT_EQ(cookieList[0], "name=bob");
}

TEST_F(ResponseCookieTestFixture, ResponseCookie_SetCookiesPlural) {
    resp.setHeader("Set-Cookie", "name=bob");
    resp.setHeader("Set-Cookie", "theme=light");
    resp.setHeader("Set-Cookie", "word=abc123");
    EXPECT_TRUE(resp.hasCookies());
    std::vector<std::string> cookieList = resp.getCookieList();
    EXPECT_EQ(cookieList.size(), (size_t) 3);
    EXPECT_EQ(cookieList[0], "name=bob");
    EXPECT_EQ(cookieList[1], "theme=light");
    EXPECT_EQ(cookieList[2], "word=abc123");
}

TEST_F(ResponseCookieTestFixture, ResponseCookie_NoCookie) {
    std::vector<std::string> cookieList = resp.getCookieList();
    EXPECT_FALSE(resp.hasCookies());
    EXPECT_TRUE(cookieList.empty());
}

TEST_F(ResponseCookieTestFixture, ResponseCookie_InvalidCookieString) {
    resp.setHeader("Set-Cookie", "");
    resp.setHeader("Set-Cookie", "namebob");
    resp.setHeader("Set-Cookie", "=bob");
    resp.setHeader("Set-Cookie", "       =bob");
    EXPECT_FALSE(resp.hasCookies());
    std::vector<std::string> cookieList = resp.getCookieList();
    EXPECT_TRUE(cookieList.empty());
}

TEST_F(ResponseCookieTestFixture, ResponseCookie_EmptyValue) {
    resp.setHeader("Set-Cookie", "name=");
    resp.setHeader("Set-Cookie", "theme=    ");
    EXPECT_TRUE(resp.hasCookies());
    std::vector<std::string> cookieList = resp.getCookieList();
    EXPECT_EQ(cookieList.size(), (size_t) 2);
    EXPECT_EQ(cookieList[0], "name=");
    EXPECT_EQ(cookieList[1], "theme=");
}

TEST_F(ResponseCookieTestFixture, ResponseCookie_ResetCookie) {
    resp.setHeader("Set-Cookie", "name=bob");
    EXPECT_TRUE(resp.hasCookies());
    resp.reset();
    EXPECT_FALSE(resp.hasCookies());
    std::vector<std::string> cookieList = resp.getCookieList();
    EXPECT_TRUE(cookieList.empty());
}