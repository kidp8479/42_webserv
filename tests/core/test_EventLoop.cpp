#include <gtest/gtest.h>
#include <sys/socket.h>

#include "../../core/EventLoop.hpp"
#include "../../core/IEventHandler.hpp"

class FakeHandler : public IEventHandler {
public:
    explicit FakeHandler(int fd)
        : handled(false), last_events(0), fd_(fd), done_(false) {
    }

    int getFd() const {
        return fd_;
    }
    void handle(short revents) {
        handled = true;
        // revents propagated correctly
        last_events = revents;
    }
    bool isDone() const {
        return done_;
    }
    const char* name() const {
        return "FakeHandler";
    }
    void markDone() {
        done_ = true;
    }
    bool handled;
    short last_events;

private:
    int fd_;
    bool done_;
};

class EventLoopTest : public ::testing::Test {
protected:
    EventLoop loop;

    int sockets[2];

    void SetUp() {
        // use socketpair for testing, simulating a connection with read and
        // write ends
        ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets), 0);
    }

    void TearDown() {
        close(sockets[0]);
        close(sockets[1]);
    }
};

TEST_F(EventLoopTest, addHandler_FullReactorLoopWorksAsIntended) {
    FakeHandler* handler = new FakeHandler(sockets[0]);
    loop.addHandler(handler, POLLIN);

    const char byte = 'A';
    // write to socket[0] so socket[1] becomes readable and poll can detect
    // POLLIN simulates incoming network data
    ASSERT_EQ(write(sockets[1], &byte, 1), 1);

    int ready = loop.wait(1000);
    // poll dected 1 fd was ready
    ASSERT_EQ(ready, 1);

    loop.dispatch();

    EXPECT_TRUE(handler->handled);
    EXPECT_TRUE(handler->last_events & POLLIN);
}

TEST_F(EventLoopTest, modifyHandler_UpdatesPollEventsCorrectly) {
    FakeHandler* handler = new FakeHandler(sockets[0]);
    loop.addHandler(handler, POLLIN);

    // change event to POLLOUT
    loop.modifyHandler(handler, POLLOUT);

    int ready = loop.wait(1000);
    ASSERT_EQ(ready, 1);

    loop.dispatch();

    EXPECT_TRUE(handler->handled);
    EXPECT_TRUE(handler->last_events & POLLOUT);
}

TEST_F(EventLoopTest, removeHandler_PreventsFutureDispatch) {
    FakeHandler* handler = new FakeHandler(sockets[0]);

    loop.addHandler(handler, POLLIN);

    // remove BEFORE any event happens
    loop.removeHandler(handler);

    const char byte = 'A';
    ASSERT_EQ(write(sockets[1], &byte, 1), 1);

    int ready = loop.wait(1000);

    // dispatch does nothing for removed handler
    loop.dispatch();

    EXPECT_FALSE(handler->handled);
    EXPECT_EQ(ready, 0);
}

TEST_F(EventLoopTest, dispatch_SupportsMultipleHandlers) {
    int sockets2[2];

    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets2), 0);

    FakeHandler* h1 = new FakeHandler(sockets[0]);
    FakeHandler* h2 = new FakeHandler(sockets2[0]);

    loop.addHandler(h1, POLLIN);
    loop.addHandler(h2, POLLIN);

    char a = 'A';
    char b = 'B';

    ASSERT_EQ(write(sockets[1], &a, 1), 1);
    ASSERT_EQ(write(sockets2[1], &b, 1), 1);

    int ready = loop.wait(1000);

    EXPECT_EQ(ready, 2);

    loop.dispatch();

    EXPECT_TRUE(h1->handled);
    EXPECT_TRUE(h2->handled);

    EXPECT_TRUE(h1->last_events & POLLIN);
    EXPECT_TRUE(h2->last_events & POLLIN);

    close(sockets2[0]);
    close(sockets2[1]);
}

TEST_F(EventLoopTest, cleanup_RemovesDoneHandlers) {
    FakeHandler* h1 = new FakeHandler(sockets[0]);
    FakeHandler* h2 = new FakeHandler(sockets[1]);

    loop.addHandler(h1, POLLIN);
    loop.addHandler(h2, POLLIN);

    h1->markDone();

    loop.cleanup();

    // only h2 remains
    EXPECT_EQ(h2->getFd(), sockets[1]);
}

TEST_F(EventLoopTest, cleanup_PreservesActiveHandlers) {
    FakeHandler* h1 = new FakeHandler(sockets[0]);
    FakeHandler* h2 = new FakeHandler(sockets[1]);

    loop.addHandler(h1, POLLIN);
    loop.addHandler(h2, POLLIN);

    h1->markDone();

    loop.cleanup();

    char a = 'A';
    ASSERT_EQ(write(sockets[0], &a, 1), 1);

    int ready = loop.wait(1000);
    loop.dispatch();

    EXPECT_EQ(ready, 1);
    EXPECT_TRUE(h2->handled);
}
