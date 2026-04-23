#include <arpa/inet.h>  // inet_addr()
#include <fcntl.h>
#include <gtest/gtest.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <cstring>
#include <thread>

#include "../../config/Config.hpp"
#include "../../config/LocationConfig.hpp"
#include "../../config/ServerConfig.hpp"
#include "../../logger/Logger.hpp"
#include "../../server/Server.hpp"

// Minimal dummy config for testing
Config createDummyConfig(int port = 8083) {
    Config cfg;
    ServerConfig sconf;
    sconf.setPort(port);

    // LocationConfig loc;
    // sconf.addLocationBlock(loc);

    cfg.addServerBlock(sconf);
    return cfg;
}

class ServerTestFixture : public ::testing::Test {
protected:
    Config cfg;     // shared config
    Server server;  // server instance

    ServerTestFixture() : cfg(createDummyConfig()), server(cfg) {
    }

    // Wrappers so TEST_F subclasses can reach the private method
    void setupSocket(int port) {
        server.setupSocket(port);
    }

    int acceptClient() {
        return server.acceptClient();
    }

    void handleRead(Client& client) {
        server.handleRead(client);
    }

    void handleWrite(Client& client) {
        server.handleWrite(client);
    }

    std::map<int, Client*>& clients() {
        return server.clients_;
    }

    const std::vector<int>& sockets() const {
        return server.getSockets();
    }

    void setNonBlocking(int fd) {
        server.setNonBlocking(fd);
    }
};

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------

// Test construction does not throw
TEST_F(ServerTestFixture, Constructor_InitialStateIsClean) {
    EXPECT_TRUE(sockets().empty());
    EXPECT_TRUE(clients().empty());
}

TEST_F(ServerTestFixture, Destructor_ClosesSockets) {
    ASSERT_NO_THROW(setupSocket(0));
    ASSERT_FALSE(sockets().empty());

    int fd = sockets()[0];
    server.stop();

    ASSERT_EQ(fcntl(fd, F_GETFD), -1);
    ASSERT_EQ(errno, EBADF);
}

// ---------------------------------------------------------------------------
// stop()
// ---------------------------------------------------------------------------

TEST_F(ServerTestFixture, Stop_ClearsAllSockets) {
    setupSocket(0);
    setupSocket(0);
    ASSERT_EQ(sockets().size(), 2u);
    server.stop();
    ASSERT_TRUE(sockets().empty());
    ASSERT_TRUE(clients().empty());
}

TEST_F(ServerTestFixture, Stop_ClearsClientsAndSockets) {
    setupSocket(19999);

    // manually inject a client
    int sv[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
    clients()[sv[0]] = new Client(sv[0]);

    server.stop();

    ASSERT_TRUE(sockets().empty());
    ASSERT_TRUE(clients().empty());

    close(sv[1]);  // sv[0] closed by stop()
}

// ---------------------------------------------------------------------------
// setupSocket()
// ---------------------------------------------------------------------------

TEST_F(ServerTestFixture, SetupSocket_DuplicatePortFails) {
    setupSocket(19999);
    ASSERT_THROW(setupSocket(19999), std::runtime_error);
}

TEST_F(ServerTestFixture, SetupSocket_MultiplePortsSucceeds) {
    setupSocket(19998);
    setupSocket(19999);
    ASSERT_EQ(sockets().size(), 2u);
}

// invalid port 0 — OS assigns random port, should succeed
TEST_F(ServerTestFixture, SetupSocket_PortZeroSucceeds) {
    ASSERT_NO_THROW(setupSocket(0));
    ASSERT_FALSE(sockets().empty());
}

TEST_F(ServerTestFixture, SetupSocket_SocketIsNonBlocking) {
    setupSocket(19999);
    int flags = fcntl(sockets()[0], F_GETFL, 0);
    ASSERT_TRUE(flags & O_NONBLOCK);
}

TEST_F(ServerTestFixture, SetupSocket_HasReuseAddr) {
    setupSocket(19999);
    int opt;
    socklen_t optlen = sizeof(opt);
    getsockopt(sockets()[0], SOL_SOCKET, SO_REUSEADDR, &opt, &optlen);
    ASSERT_TRUE(opt);
}

// ---------------------------------------------------------------------------
// setNonBlocking()
// ---------------------------------------------------------------------------

TEST_F(ServerTestFixture, SetNonBlocking_FdIsNonBlocking) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    setNonBlocking(fd);
    int flags = fcntl(fd, F_GETFL, 0);
    ASSERT_TRUE(flags & O_NONBLOCK);
    close(fd);
}

// ---------------------------------------------------------------------------
// acceptClient()
// ---------------------------------------------------------------------------

TEST_F(ServerTestFixture, AcceptClient_ReturnsValidFd) {
    setupSocket(19999);

    // connect a client in background
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(19999);
    connect(client_fd, (struct sockaddr*)&addr, sizeof(addr));

    int accepted = acceptClient();
    ASSERT_GE(accepted, 0);

    close(client_fd);
    close(accepted);
}

TEST_F(ServerTestFixture, AcceptClient_NoPendingConnectionReturnsNegative) {
    setupSocket(19999);
    int result = acceptClient();
    ASSERT_EQ(result, -1);
}

TEST_F(ServerTestFixture, AcceptClient_AcceptedFdIsNonBlocking) {
    setupSocket(19999);

    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(19999);
    connect(client_fd, (struct sockaddr*)&addr, sizeof(addr));

    int accepted = acceptClient();
    int flags = fcntl(accepted, F_GETFL, 0);
    ASSERT_TRUE(flags & O_NONBLOCK);

    close(client_fd);
    close(accepted);
}

// ---------------------------------------------------------------------------
// handleRead()
// ---------------------------------------------------------------------------

TEST_F(ServerTestFixture, HandleRead_SetsClientDoneOnClose) {
    int sv[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv);

    Client client(sv[0]);
    close(sv[1]);  // close client side — triggers bytes == 0 in recv()

    handleRead(client);
    ASSERT_EQ(client.getState(), Client::kDone);
}

TEST_F(ServerTestFixture, HandleRead_PartialDataStaysReading) {
    int sv[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv);

    Client client(sv[0]);
    write(sv[1], "GET / HTTP/1.1\r\n", 16);  // no \r\n\r\n yet

    handleRead(client);
    ASSERT_EQ(client.getState(), Client::kReading);

    close(sv[1]);
}

TEST_F(ServerTestFixture, HandleRead_CompleteRequestSwitchesToWriting) {
    int sv[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv);

    Client client(sv[0]);
    write(sv[1], "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n", 35);

    handleRead(client);
    ASSERT_EQ(client.getState(), Client::kWriting);

    close(sv[1]);
}

TEST_F(ServerTestFixture, HandleRead_PeerCloseSetsClientDone) {
    int sv[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv);

    Client client(sv[0]);
    close(sv[1]);  // simulate peer disconnect

    handleRead(client);
    ASSERT_EQ(client.getState(), Client::kDone);
}

// ---------------------------------------------------------------------------
// handleWrite()
// ---------------------------------------------------------------------------

TEST_F(ServerTestFixture, HandleWrite_EmptyResponseSetsDone) {
    int sv[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv);

    Client client(sv[0]);

    // get client into kWriting naturally
    write(sv[1], "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n", 35);
    handleRead(client);
    ASSERT_EQ(client.getState(), Client::kWriting);  // sanity check

    // now test write
    handleWrite(client);
    ASSERT_EQ(client.getState(), Client::kDone);

    close(sv[1]);
}
