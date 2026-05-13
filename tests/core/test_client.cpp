#include <gtest/gtest.h>
#include "../../config/ServerConfig.hpp"
#include "../../core/Listener.hpp"
#include "../../core/Client.hpp"
#include "../../core/EventLoop.hpp"
#include "../../core/ServerResources.hpp"

#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>

ServerConfig createDummyServerConfig(int port = 8083) {
	ServerConfig sconf;
	sconf.setPort(port);

	LocationConfig loc;
	loc.setPath("/");

	sconf.addLocationBlock(loc);

	return sconf;
}

class ClientTest : public ::testing::Test {
protected:
	ServerConfig server_config;
	ServerResources resources;
	EventLoop loop;

	int sockets[2];

	ClientTest() :
		server_config(createDummyServerConfig(8088)),
		resources(server_config),
		loop()
	{}

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
	
	sockets[0] = -1;
	
	EXPECT_GT(client.getFd(), 0);
	EXPECT_STREQ(client.name(), "Client");
}
