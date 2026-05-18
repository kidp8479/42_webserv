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
Config createDummyConfig(int port) {
    Config cfg;
    ServerConfig sconf;

	sconf.setHost("127.0.0.1");
    sconf.setPort(port);

    LocationConfig loc;
    loc.setPath("/");
    sconf.addLocationBlock(loc);

    cfg.addServerBlock(sconf);
    return cfg;
}

// Find a free port by binding to 0, reading what the OS picked, then releasing it
int getFreePort() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
	
	assert(fd >= 0);

	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = 0;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	assert(bind(fd, (sockaddr*)&addr, sizeof(addr)) == 0);
	socklen_t len = sizeof(addr);

	getsockname(fd, (sockaddr*)&addr, &len);
	int port = ntohs(addr.sin_port);

	close(fd);  // release it — server will bind to it next
	return port;
}

class ServerTestFixture : public ::testing::Test {
protected:
	int port_os;
    Config cfg;     // shared config
    Server server;  // server instance

    ServerTestFixture() :
		port_os(getFreePort()),
		cfg(createDummyConfig(port_os)),
		server(cfg)
	{}

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
    EXPECT_TRUE(canConnect(port_os));
}

TEST_F(ServerTestFixture, Constructor_CreatesMultipleListeningSockets) {
    int port1 = getFreePort();
    int port2 = getFreePort();

    // Build a second config manually for this test
    Config cfg;
    ServerConfig s1;
    ServerConfig s2;

	s1.setHost("127.0.0.1");
    s1.setPort(port1);

	s2.setHost("127.0.0.1");
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
	int port = getFreePort();

    Config cfg;

    ServerConfig s1;
    ServerConfig s2;

	s1.setHost("127.0.0.1");
    s1.setPort(port);
	s2.setHost("127.0.0.1");
    s2.setPort(port);  // duplicate port

    LocationConfig loc;
    loc.setPath("/");

    s1.addLocationBlock(loc);
    s2.addLocationBlock(loc);

    cfg.addServerBlock(s1);
    cfg.addServerBlock(s2);

    EXPECT_THROW(Server server(cfg), std::runtime_error);
}

TEST(Server, Constructor_MinimalConfig_DoesNotCrash) {
	int port = getFreePort();
    Config cfg;

    ServerConfig s;
	s.setHost("127.0.0.1");
    s.setPort(port);

    LocationConfig loc;
    loc.setPath("/");

    s.addLocationBlock(loc);
    cfg.addServerBlock(s);

    EXPECT_NO_THROW(Server server(cfg));
}
