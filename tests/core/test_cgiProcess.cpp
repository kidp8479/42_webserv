#include <gtest/gtest.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../../config/ServerConfig.hpp"
#include "../../core/CgiProcess.hpp"
#include "../../core/Client.hpp"
#include "../../core/EventLoop.hpp"
#include "../../core/ServerResources.hpp"

// --------------------------------------------------
// Helper: create minimal valid config
// --------------------------------------------------

ServerConfig createDummyConfig() {
    ServerConfig cfg;
    cfg.setHost("127.0.0.1");
    cfg.setPort(8080);

    LocationConfig loc;
    loc.setPath("/");
    cfg.addLocationBlock(loc);

    return cfg;
}

// --------------------------------------------------
// Test fixture
// --------------------------------------------------
class CgiProcessTest : public ::testing::Test {
protected:
    int pipefd[2];
    EventLoop loop;
    ServerConfig config;
    ServerResources resources;

    static ServerConfig createDummyConfig() {
        ServerConfig cfg;
        cfg.setHost("127.0.0.1");
        cfg.setPort(8080);

        LocationConfig loc;
        loc.setPath("/");
        cfg.addLocationBlock(loc);

        return cfg;
    }

    CgiProcessTest() : config(createDummyConfig()), resources(config) {
    }
};

// --------------------------------------------------
// Constructor test
// --------------------------------------------------

TEST_F(CgiProcessTest, ConstructorInitializesState) {
    Client client(pipefd[0], loop, resources, "127.0.0.1");

    CgiProcess cgi(1234, pipefd[0], client, loop);

    EXPECT_FALSE(cgi.isDone());
}

// --------------------------------------------------
// Basic read behavior (POLLIN)
// --------------------------------------------------

TEST_F(CgiProcessTest, HandleReadsDataOnPOLLIN) {
    Client client(pipefd[0], loop, resources, "127.0.0.1");

    CgiProcess cgi(1234, pipefd[0], client, loop);

    const char* msg = "Hello CGI";
    write(pipefd[1], msg, strlen(msg));

    cgi.handle(POLLIN);

    EXPECT_FALSE(cgi.isDone());
}

// --------------------------------------------------
// EOF triggers finish()
// --------------------------------------------------

TEST_F(CgiProcessTest, HandleEOFTriggersFinish) {
    int fds[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);

    ServerConfig cfg;
    ServerResources resources(cfg);
    EventLoop loop;

    Client client(fds[1], loop, resources, "127.0.0.1");

    CgiProcess cgi(1234, fds[0], client, loop);

    close(fds[1]);  // IMPORTANT: triggers EOF

    cgi.handle(POLLIN);

    EXPECT_TRUE(cgi.isDone());
}

// --------------------------------------------------
// POLLERR / POLLHUP triggers finish()
// --------------------------------------------------

TEST_F(CgiProcessTest, HandlePollErrorTriggersFinish) {
    Client client(pipefd[0], loop, resources, "127.0.0.1");

    CgiProcess cgi(1234, pipefd[0], client, loop);

    cgi.handle(POLLERR);

    EXPECT_TRUE(cgi.isDone());
}

// --------------------------------------------------
// isTimedOut() sanity check (no sleep-based test)
// --------------------------------------------------

TEST_F(CgiProcessTest, IsTimedOutInitiallyFalse) {
    Client client(pipefd[0], loop, resources, "127.0.0.1");

    CgiProcess cgi(1234, pipefd[0], client, loop);

    EXPECT_FALSE(cgi.isTimedOut());
}

// --------------------------------------------------
// isDone() sanity check
// --------------------------------------------------

TEST_F(CgiProcessTest, IsDoneInitiallyFalse) {
    Client client(pipefd[0], loop, resources, "127.0.0.1");

    CgiProcess cgi(1234, pipefd[0], client, loop);

    EXPECT_FALSE(cgi.isDone());
}
