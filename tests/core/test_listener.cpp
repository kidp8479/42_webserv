#include <assert.h>
#include <fcntl.h>
#include <gtest/gtest.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../../config/LocationConfig.hpp"
#include "../../config/ServerConfig.hpp"
#include "../../core/EventLoop.hpp"
#include "../../core/Listener.hpp"
#include "../../core/ServerResources.hpp"

ServerConfig createDummyServerConfig(int port) {
    ServerConfig sconf;
    sconf.setHost("127.0.0.1");
    sconf.setPort(port);

    LocationConfig loc;
    loc.setPath("/");

    sconf.addLocationBlock(loc);

    return sconf;
}

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

    close(fd);
    return port;
}

class ListenerTestFixture : public ::testing::Test {
protected:
    int port_os;
    ServerConfig server_config;
    ServerResources resources;
    EventLoop loop;

    ListenerTestFixture()
        : port_os(getFreePort()),
          server_config(createDummyServerConfig(port_os)),
          resources(server_config),
          loop() {
    }
};

TEST_F(ListenerTestFixture, constructor_CreatesValidSocket) {
    Listener* listener = new Listener(loop, resources);

    EXPECT_GE(listener->getFd(), 0);
}

TEST_F(ListenerTestFixture, SocketIsNonBlocking) {
    Listener* listener = new Listener(loop, resources);

    int flags = fcntl(listener->getFd(), F_GETFL);

    ASSERT_NE(flags, -1);
    EXPECT_TRUE(flags & O_NONBLOCK);
}

TEST_F(ListenerTestFixture, BindsConfiguredPort) {
    Listener* listener = new Listener(loop, resources);

    sockaddr_in addr{};
    socklen_t len = sizeof(addr);

    ASSERT_EQ(
        getsockname(listener->getFd(),
                    reinterpret_cast<sockaddr*>(&addr),
                    &len),
        0);

    EXPECT_EQ(ntohs(addr.sin_port), port_os);
}

TEST_F(ListenerTestFixture, ReturnsCorrectName) {
    Listener* listener = new Listener(loop, resources);

    EXPECT_STREQ("Listener", listener->name());
}
