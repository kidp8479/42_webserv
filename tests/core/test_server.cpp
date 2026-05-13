#include <gtest/gtest.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>

#include "../../config/Config.hpp"
#include "../../config/LocationConfig.hpp"
#include "../../config/ServerConfig.hpp"
#include "../../core/Server.hpp"

// Minimal dummy config for testing
Config createDummyConfig(int port = 8083) {
    Config cfg;
    ServerConfig sconf;
    sconf.setPort(port);

    LocationConfig loc;
    loc.setPath("/");
    sconf.addLocationBlock(loc);

    cfg.addServerBlock(sconf);
    return cfg;
}

class ServerTestFixture : public ::testing::Test {
protected:
    Config cfg;     // shared config
    Server server;  // server instance

    ServerTestFixture() : cfg(createDummyConfig()), server(cfg) {
    }

    // Wrappers
    int createSocket() {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        EXPECT_NE(fd, -1);
        return fd;
    }

    sockaddr_in makeAddress(int port) {
        sockaddr_in addr;

        std::memset(&addr, 0, sizeof(addr));

        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

        return addr;
    }

    bool canConnect(int port) {
        int fd = createSocket();
        sockaddr_in addr = makeAddress(port);

        int ret = connect(fd, (sockaddr*)&addr, sizeof(addr));
        close(fd);
        return ret == 0;
    }
};

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------

// Test construction does not throw
TEST_F(ServerTestFixture, Constructor_CreatesListeningSocket) {
    EXPECT_TRUE(canConnect(8083));
}

TEST_F(ServerTestFixture, Constructor_CreatesMultipleListeningSockets) {
    const int port1 = 8085;
    const int port2 = 8086;

    // Build a second config manually for this test
    Config cfg;
    ServerConfig s1;
    ServerConfig s2;

    s1.setPort(port1);
    s2.setPort(port2);

    LocationConfig loc;
    loc.setPath("/");

    s1.addLocationBlock(loc);
    s2.addLocationBlock(loc);

    cfg.addServerBlock(s1);
    cfg.addServerBlock(s2);

    Server testServer(cfg);

    // Verify both ports are actually listening
    EXPECT_TRUE(canConnect(port1));
    EXPECT_TRUE(canConnect(port2));
}

TEST(Server, Constructor_InvalidConfig_Throws) {
    Config cfg;

    ServerConfig s1;
    ServerConfig s2;

    s1.setPort(8085);
    s2.setPort(8085);  // duplicate port

    LocationConfig loc;
    loc.setPath("/");

    s1.addLocationBlock(loc);
    s2.addLocationBlock(loc);

    cfg.addServerBlock(s1);
    cfg.addServerBlock(s2);

    EXPECT_THROW(Server server(cfg), std::runtime_error);
}

TEST(Server, Constructor_MinimalConfig_DoesNotCrash) {
    Config cfg;

    ServerConfig s;
    s.setPort(9090);

    LocationConfig loc;
    loc.setPath("/");

    s.addLocationBlock(loc);
    cfg.addServerBlock(s);

    EXPECT_NO_THROW(Server server(cfg));
}
