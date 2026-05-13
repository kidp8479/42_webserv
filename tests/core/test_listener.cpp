#include <gtest/gtest.h>
//#include "../../core/ServerResources.hpp"
#include "../../core/Listener.hpp"
#include "../../config/ServerConfig.hpp"
#include "../../core/EventLoop.hpp"
#include "../../core/ServerResources.hpp"
//#include "../../handlers/Router.hpp"
#include <sys/socket.h>
#include <fcntl.h>

ServerConfig createDummyServerConfig(int port = 8083) {
	ServerConfig sconf;
	sconf.setPort(port);

	LocationConfig loc;
	loc.setPath("/");

	sconf.addLocationBlock(loc);

	return sconf;
}

class ListenerTestFixture : public ::testing::Test {
protected:
	ServerConfig server_config;
	ServerResources resources;
	EventLoop loop;

	ListenerTestFixture() :
		server_config(createDummyServerConfig(8088)),
		resources(server_config),
		loop()
	{}
};

TEST_F(ListenerTestFixture, Constructor_CreatesValidSocket) {
	Listener* listener = new Listener(8088, loop, resources);

	EXPECT_GT(listener->getFd(), 0);
	EXPECT_STREQ(listener->name(), "Listener");
}

TEST_F(ListenerTestFixture, Constructor_SetsSocketNonBlocking) {
    Listener* listener = new Listener(8088, loop, resources);

    int flags = fcntl(listener->getFd(), F_GETFL, 0);
    EXPECT_TRUE(flags & O_NONBLOCK);
}
