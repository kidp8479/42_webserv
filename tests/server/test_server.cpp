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

    //LocationConfig loc;
    //sconf.addLocationBlock(loc);

    cfg.addServerBlock(sconf);
    return cfg;
}

// ---------------------------------------------------------------------------
// Test fixture: allows access to private Server members
// ---------------------------------------------------------------------------
class ServerTestFixture : public ::testing::Test {
protected:
    Config cfg;    // shared config
    Server server; // server instance
	
	ServerTestFixture() :
		cfg(createDummyConfig()), server(cfg) {}
	
	// Wrapper so TEST_F subclasses can reach the private method
    void setupSocket(int port) {
        server.setupSocket(port);  // fixture is a friend, so this compiles
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
};

// ---------------------------------------------------------------------------
// Tests using the fixture
// ---------------------------------------------------------------------------

// Test construction does not throw
TEST_F(ServerTestFixture, Construct_InitialStateIsEmpty) {
    EXPECT_TRUE(sockets().empty());
	EXPECT_TRUE(clients().empty());
}

