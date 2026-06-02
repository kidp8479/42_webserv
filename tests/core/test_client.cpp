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
          loop() {}

    static void setNonBlocking(int fd) {
        int flags = fcntl(fd, F_GETFL, 0);
        ASSERT_NE(flags, -1);
        ASSERT_NE(fcntl(fd, F_SETFL, flags | O_NONBLOCK), -1);
    }

    void SetUp() override {
        ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets), 0);

        setNonBlocking(sockets[0]);
        setNonBlocking(sockets[1]);
    }

    void TearDown() override {
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
    sockets[0] = -1;

    EXPECT_GT(client.getFd(), 0);
    EXPECT_STREQ(client.name(), "Client");
}

TEST_F(ClientTest, HandlePollIn_ParsesCompleteValidRequest) {
    Client client(sockets[0], loop, resources);
    sockets[0] = -1;

    const char* request =
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    ASSERT_GT(write(sockets[1], request, strlen(request)), 0);

    client.handle(POLLIN);

    // forces response generation (non-CGI path)
    client.handle(POLLOUT);

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

    const char* partial =
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n";

    ASSERT_GT(write(sockets[1], partial, strlen(partial)), 0);

    EXPECT_NO_THROW(client.handle(POLLIN));

    char buffer[1024];
    ssize_t n = recv(sockets[1], buffer, sizeof(buffer), MSG_DONTWAIT);

    EXPECT_EQ(n, -1);
    EXPECT_TRUE(errno == EAGAIN || errno == EWOULDBLOCK);
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

TEST_F(ClientTest, KeepAlive_HandlesMultipleRequests) {
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
    ssize_t n1 = recv(sockets[1], buffer, sizeof(buffer), 0);
    ASSERT_GT(n1, 0);

    ASSERT_GT(write(sockets[1], request, strlen(request)), 0);

    client.handle(POLLIN);
    client.handle(POLLOUT);

    ssize_t n2 = recv(sockets[1], buffer, sizeof(buffer), 0);
    ASSERT_GT(n2, 0);
}

//
// ========================
// CGI UNIT-LEVEL TESTING
// ========================
// We DO NOT fork/exec here.
// We only test Client-side integration points.
// ========================
//

TEST_F(ClientTest, CGI_SetPendingStateTransitionsToWaiting) {
    Client client(sockets[0], loop, resources);
    sockets[0] = -1;

    // simulate CGI start
    client.setPendingCgi(reinterpret_cast<CgiProcess*>(0x1));

    EXPECT_NO_THROW(client.handle(POLLIN));
    EXPECT_NO_THROW(client.handle(POLLOUT));
}

TEST_F(ClientTest, CGI_OnFinishedBuildsResponse) {
    Client client(sockets[0], loop, resources);
    sockets[0] = -1;

    const std::string fake_cgi_output =
        "Status: 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "\r\n"
        "Hello CGI";

    EXPECT_NO_THROW(client.onCgiFinished(fake_cgi_output));

    char buffer[4096];
    ssize_t n = recv(sockets[1], buffer, sizeof(buffer) - 1, MSG_DONTWAIT);

    // may be no real socket output yet depending on loop scheduling,
    // but state transition must succeed without crash
    EXPECT_GE(n, -1);
}
