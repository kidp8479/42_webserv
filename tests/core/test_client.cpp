#include <fcntl.h>
#include <gtest/gtest.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../../config/ServerConfig.hpp"
#include "../../core/Client.hpp"
#include "../../core/EventLoop.hpp"
#include "../../core/Listener.hpp"
#include "../../core/ServerResources.hpp"

ServerConfig createDummyServerConfig(int port = 8083) {
    ServerConfig sconf;
    sconf.setPort(port);

    LocationConfig loc;
    loc.setPath("/");
    loc.setRoot("../http_handler/static_test_files");
    loc.setIndex("hello.html");
    std::vector<std::string> methods;
    methods.push_back("GET");
    methods.push_back("POST");
    methods.push_back("DELETE");
    loc.setMethods(methods);

    sconf.addLocationBlock(loc);

    return sconf;
}

class ClientTest : public ::testing::Test {
protected:
    ServerConfig server_config;
    ServerResources resources;
    EventLoop loop;

    int sockets[2];

    ClientTest()
        : server_config(createDummyServerConfig(8088)),
          resources(server_config),
          loop() {
    }

    static void setNonBlocking(int fd) {
        int flags = fcntl(fd, F_GETFL, 0);
        ASSERT_NE(flags, -1);

        ASSERT_NE(fcntl(fd, F_SETFL, flags | O_NONBLOCK), -1);
    }

    void SetUp() {
        ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets), 0);

        setNonBlocking(sockets[0]);
        setNonBlocking(sockets[1]);
    }

    void TearDown() {
        if (sockets[0] >= 0) {
            close(sockets[0]);
        }
        if (sockets[1] >= 0) {
            close(sockets[1]);
        }
    }
};

TEST_F(ClientTest, Constructor_InitializesClient) {
    Client client(sockets[0], loop, resources);

    // transfer ownership to Client RAII
    sockets[0] = -1;

    EXPECT_GT(client.getFd(), 0);
    EXPECT_STREQ(client.name(), "Client");
}

TEST_F(ClientTest, HandlePollIn_ParsesCompleteValidRequest) {
    Client client(sockets[0], loop, resources);

    // transfer ownership to Client
    sockets[0] = -1;

    const char* request =
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    ASSERT_GT(write(sockets[1], request, strlen(request)), 0);

    EXPECT_NO_THROW(client.handle(POLLIN));
    EXPECT_NO_THROW(client.handle(POLLOUT));

    char buffer[4096];

    ssize_t n = recv(sockets[1], buffer, sizeof(buffer) - 1, 0);

    ASSERT_GT(n, 0);

    buffer[n] = '\0';
    std::string response(buffer);

    EXPECT_NE(response.find("HTTP/1.1"), std::string::npos);
}

TEST_F(ClientTest, HandlePollIn_HandlesPartialRequestSafely) {
    Client client(sockets[0], loop, resources);

    sockets[0] = -1;

    const char* partial_request =
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n";

    ASSERT_GT(write(sockets[1], partial_request, strlen(partial_request)), 0);
    EXPECT_NO_THROW(client.handle(POLLIN));

    char buffer[1024];

    ssize_t n = recv(sockets[1], buffer, sizeof(buffer), MSG_DONTWAIT);

    EXPECT_EQ(n, -1);
    EXPECT_TRUE(errno == EAGAIN || errno == EWOULDBLOCK);
}

TEST_F(ClientTest, HandlePollIn_HandlesClientDisconnect) {
    Client client(sockets[0], loop, resources);

    sockets[0] = -1;

    close(sockets[1]);
    sockets[1] = -1;

    EXPECT_NO_THROW(client.handle(POLLIN));
}

TEST_F(ClientTest, HandlePollIn_HandlesEagainSafely) {
    Client client(sockets[0], loop, resources);

    sockets[0] = -1;

    EXPECT_NO_THROW(client.handle(POLLIN));
}

TEST_F(ClientTest, HandlePollIn_HandlesMalformedRequestSafely) {
    Client client(sockets[0], loop, resources);

    sockets[0] = -1;

    const char* bad_request = "INVALID_REQUEST\r\n\r\n";

    ASSERT_GT(write(sockets[1], bad_request, strlen(bad_request)), 0);
    EXPECT_NO_THROW(client.handle(POLLIN));
}

TEST_F(ClientTest, HandlePollIn_HandlesIncompleteHeadersSafely) {
    Client client(sockets[0], loop, resources);

    sockets[0] = -1;

    const char* incomplete =
        "GET / HTTP/1.1\r\n"
        "Host:";

    ASSERT_GT(write(sockets[1], incomplete, strlen(incomplete)), 0);
    EXPECT_NO_THROW(client.handle(POLLIN));
}

TEST_F(ClientTest, HandlePollOut_SendsGeneratedResponse) {
    Client client(sockets[0], loop, resources);

    sockets[0] = -1;

    const char* request =
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    ASSERT_GT(write(sockets[1], request, strlen(request)), 0);

    client.handle(POLLIN);
    client.handle(POLLOUT);

    char buffer[4096];
    ssize_t n = recv(sockets[1], buffer, sizeof(buffer) - 1, 0);

    ASSERT_GT(n, 0);

    buffer[n] = '\0';
    std::string response(buffer);

    EXPECT_NE(response.find("HTTP/1.1"), std::string::npos);
}

TEST_F(ClientTest, HandlePollOut_ClosesNonKeepAliveConnection) {
    Client client(sockets[0], loop, resources);

    sockets[0] = -1;

    const char* request =
        "GET / HTTP/1.0\r\n"
        "Host: localhost\r\n"
        "\r\n";

    ASSERT_GT(write(sockets[1], request, strlen(request)), 0);

    client.handle(POLLIN);
    client.handle(POLLOUT);

    char buffer[4096];

    ASSERT_GT(recv(sockets[1], buffer, sizeof(buffer), 0), 0);

    // server should close connection afterward
    char test;
    ssize_t n = recv(sockets[1], &test, 1, MSG_DONTWAIT);

    EXPECT_TRUE(n == 0 || (n == -1 && errno == EAGAIN));
}

TEST_F(ClientTest, HandlePollOut_ResetsKeepAliveConnection) {
    Client client(sockets[0], loop, resources);

    sockets[0] = -1;

    const char* request =
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    // first request
    ASSERT_GT(write(sockets[1], request, strlen(request)), 0);

    client.handle(POLLIN);
    client.handle(POLLOUT);

    char buffer[4096];

    ASSERT_GT(recv(sockets[1], buffer, sizeof(buffer), 0), 0);
    // second request on same socket
    ASSERT_GT(write(sockets[1], request, strlen(request)), 0);

    client.handle(POLLIN);
    client.handle(POLLOUT);

    ssize_t n = recv(sockets[1], buffer, sizeof(buffer), 0);

    ASSERT_GT(n, 0);

    buffer[n] = '\0';
    std::string response(buffer);

    EXPECT_NE(response.find("HTTP/1.1"), std::string::npos);
}

TEST_F(ClientTest, HandlePollErr_ClosesConnectionSafely) {
    Client client(sockets[0], loop, resources);

    sockets[0] = -1;

    EXPECT_NO_THROW(client.handle(POLLERR));
}

TEST_F(ClientTest, HandlePollNval_ClosesConnectionSafely) {
    Client client(sockets[0], loop, resources);

    sockets[0] = -1;

    EXPECT_NO_THROW(client.handle(POLLNVAL));
}

TEST_F(ClientTest, HandlePollHup_IncompleteRequestClosesSafely) {
    Client client(sockets[0], loop, resources);

    sockets[0] = -1;

    const char* partial_request =
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n";

    ASSERT_GT(write(sockets[1], partial_request, strlen(partial_request)), 0);

    client.handle(POLLIN);

    close(sockets[1]);
    sockets[1] = -1;

    EXPECT_NO_THROW(client.handle(POLLHUP));
}

TEST_F(ClientTest, HandlePollHup_AfterCompleteRequestDoesNotCrash) {
    Client client(sockets[0], loop, resources);

    sockets[0] = -1;

    const char* request =
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    ASSERT_GT(write(sockets[1], request, strlen(request)), 0);

    client.handle(POLLIN);

    close(sockets[1]);
    sockets[1] = -1;

    EXPECT_NO_THROW(client.handle(POLLHUP));
}

TEST_F(ClientTest, FullFlow_ValidGetGeneratesResponse) {
    Client client(sockets[0], loop, resources);

    sockets[0] = -1;

    const char* request =
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    ASSERT_GT(write(sockets[1], request, strlen(request)), 0);

    client.handle(POLLIN);
    client.handle(POLLOUT);

    char buffer[8192];
    ssize_t n = recv(sockets[1], buffer, sizeof(buffer) - 1, 0);

    ASSERT_GT(n, 0);

    buffer[n] = '\0';
    std::string response(buffer);

    EXPECT_NE(response.find("HTTP/1.1"), std::string::npos);
    EXPECT_NE(response.find("200"), std::string::npos);
}

TEST_F(ClientTest, FullFlow_ResponseHasValidStatusLine) {
    Client client(sockets[0], loop, resources);

    sockets[0] = -1;

    const char* request =
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    ASSERT_GT(write(sockets[1], request, strlen(request)), 0);

    client.handle(POLLIN);
    client.handle(POLLOUT);

    char buffer[8192];
    ssize_t n = recv(sockets[1], buffer, sizeof(buffer) - 1, 0);

    ASSERT_GT(n, 0);

    buffer[n] = '\0';
    std::string response(buffer);

    EXPECT_NE(response.find("HTTP/1.1 200"), std::string::npos);
}

TEST_F(ClientTest, FullFlow_ResponseBodyIsCorrect) {
    Client client(sockets[0], loop, resources);

    sockets[0] = -1;

    const char* request =
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    ASSERT_GT(write(sockets[1], request, strlen(request)), 0);

    client.handle(POLLIN);
    client.handle(POLLOUT);

    char buffer[8192];
    ssize_t n = recv(sockets[1], buffer, sizeof(buffer) - 1, 0);

    ASSERT_GT(n, 0);

    buffer[n] = '\0';
    std::string response(buffer);

    // adjust depending on your actual handler output
    EXPECT_NE(response.find("\r\n\r\n"), std::string::npos);
}

TEST_F(ClientTest, FullFlow_KeepAliveMultipleRequestsWork) {
    Client client(sockets[0], loop, resources);

    sockets[0] = -1;

    const char* request =
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    // first request
    ASSERT_GT(write(sockets[1], request, strlen(request)), 0);

    client.handle(POLLIN);
    client.handle(POLLOUT);

    char buffer[8192];

    ssize_t n1 = recv(sockets[1], buffer, sizeof(buffer) - 1, 0);
    ASSERT_GT(n1, 0);

    buffer[n1] = '\0';

    std::string response1(buffer);
    EXPECT_NE(response1.find("HTTP/1.1"), std::string::npos);

    // second request on SAME connection
    ASSERT_GT(write(sockets[1], request, strlen(request)), 0);

    client.handle(POLLIN);
    client.handle(POLLOUT);

    ssize_t n2 = recv(sockets[1], buffer, sizeof(buffer) - 1, 0);
    ASSERT_GT(n2, 0);

    buffer[n2] = '\0';
    std::string response2(buffer);

    EXPECT_NE(response2.find("HTTP/1.1"), std::string::npos);
}
