#include "../../server/Server.hpp"
#include "../../config/Config.hpp"
#include "../../config/ServerConfig.hpp"
#include "../../config/LocationConfig.hpp"
#include "../../logger/Logger.hpp"
#include <gtest/gtest.h>
#include <thread>
#include <chrono>

// Minimal dummy config for testing
Config createDummyConfig(int port = 8083) {
    Config cfg;
    ServerConfig sconf;
    sconf.setPort(port);

    LocationConfig loc;
    sconf.addLocationBlock(loc);

    cfg.addServerBlock(sconf);
    return cfg;
}

// ---------------------------------------------------------------------------
// Test fixture: allows access to private Server members
// ---------------------------------------------------------------------------
class ServerTestFixture : public ::testing::Test {
protected:
    Config cfg = createDummyConfig();   // shared config
    Server server{cfg};                  // server instance
	
	// Wrapper so TEST_F subclasses can reach the private method
    void setupSocket(int port) {
        server.setupSocket(port);  // fixture is a friend, so this compiles
    }
};

// ---------------------------------------------------------------------------
// Tests using the fixture
// ---------------------------------------------------------------------------

// Test construction does not throw
TEST_F(ServerTestFixture, ConstructDoesNotThrow) {
    EXPECT_NO_THROW({
        Server s(cfg);
    });
}

// Test start() adds a socket (runs in separate thread because it blocks)
TEST_F(ServerTestFixture, StartAddsSocket) {
    std::thread t([this]() {
        server.start();
    });

    // Give start() some time to create the socket
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    EXPECT_EQ(server.getSockets().size(), 1);

    server.stop();
    t.detach(); // temporary, replace with proper shutdown if implemented
}

// Test stop() clears sockets
TEST_F(ServerTestFixture, StopClearsSockets) {
    std::thread t([this]() { server.start(); });
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    EXPECT_GT(server.getSockets().size(), 0);

    server.stop();
    EXPECT_EQ(server.getSockets().size(), 0);

    t.detach();
}

// Test private setupSocket() via friend access
TEST_F(ServerTestFixture, SetupSocketAddsSocket) {
    setupSocket(8084);

    EXPECT_EQ(server.getSockets().size(), 1);

    server.stop();
}
