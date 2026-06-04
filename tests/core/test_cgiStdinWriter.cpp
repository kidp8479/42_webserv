#include <fcntl.h>
#include <gtest/gtest.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../../core/CgiStdinWriter.hpp"
#include "../../core/EventLoop.hpp"

class CgiStdinWriterTest : public ::testing::Test {
protected:
    int fds[2];
    EventLoop loop;

    static void setNonBlocking(int fd) {
        int flags = fcntl(fd, F_GETFL, 0);
        ASSERT_NE(flags, -1);
        ASSERT_NE(fcntl(fd, F_SETFL, flags | O_NONBLOCK), -1);
    }

    void SetUp() override {
        ASSERT_EQ(pipe(fds), 0);
        signal(SIGPIPE, SIG_IGN);

        setNonBlocking(fds[0]);
        setNonBlocking(fds[1]);
    }

    void TearDown() override {
        close(fds[0]);
        close(fds[1]);
    }
};

TEST_F(CgiStdinWriterTest, ConstructorInitialState) {
    CgiStdinWriter writer(fds[1], "hello", loop);

    EXPECT_FALSE(writer.isDone());
    EXPECT_STREQ(writer.name(), "CgiStdinWriter");
}

TEST_F(CgiStdinWriterTest, PartialWriteDoesNotFinishImmediately) {
    std::string body(100000, 'A');

    CgiStdinWriter writer(fds[1], body, loop);

    writer.handle(POLLOUT);

    // should have written something
    char buf[4096];
    ssize_t n = read(fds[0], buf, sizeof(buf));

    EXPECT_GT(n, 0);

    // IMPORTANT: not guaranteed done after one write
    EXPECT_FALSE(writer.isDone());
}

TEST_F(CgiStdinWriterTest, CompleteWriteFinishesAndClosesFd) {
    std::string body = "POST_DATA";
    CgiStdinWriter writer(fds[1], body, loop);
    writer.handle(POLLOUT);

    EXPECT_TRUE(writer.isDone());
    // drain the data first
    char buf[1024];
    ssize_t n = read(fds[0], buf, sizeof(buf));

    EXPECT_EQ(n, (ssize_t)body.size());                    // all data written
    EXPECT_EQ(0, memcmp(buf, body.c_str(), body.size()));  // correct data

    // now check EOF — fd should be closed, read returns 0
    n = read(fds[0], buf, sizeof(buf));
    EXPECT_EQ(n, 0);
}

TEST_F(CgiStdinWriterTest, WriteFailureMarksDone) {
    std::string body = "data";

    CgiStdinWriter writer(fds[1], body, loop);

    close(fds[0]);  // break pipe

    writer.handle(POLLOUT);

    EXPECT_TRUE(writer.isDone());
}
