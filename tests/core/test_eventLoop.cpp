#include <gtest/gtest.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../../core/EventLoop.hpp"
#include "../../core/IEventHandler.hpp"

class FakeHandler : public IEventHandler {
public:
    explicit FakeHandler(int fd, bool* timeout_flag = NULL)
        : handled(false),
          last_events(0),
          fd_(fd),
          done_(false),
          timed_out_(false),
          timeout_flag_(timeout_flag) {
    }

    int getFd() const {
        return fd_;
    }

    void handle(short revents) {
        handled = true;
        last_events = revents;
    }

    bool isDone() const {
        return done_;
    }

    bool isTimedOut() const {
        return timed_out_;
    }

    const char* name() const {
        return "FakeHandler";
    }

    void onTimeout() {
        if (timeout_flag_) {
            *timeout_flag_ = true;
        }
    }

    void markDone() {
        done_ = true;
    }

    void markTimedOut() {
        timed_out_ = true;
    }

    bool handled;
    short last_events;

private:
    int fd_;
    bool done_;
    bool timed_out_;
    bool* timeout_flag_;
};

class EventLoopTest : public ::testing::Test {
protected:
    EventLoop loop;
    int sockets[2];

    void SetUp() {
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
    ASSERT_EQ(write(sockets[1], &byte, 1), 1);

    int ready = loop.wait(1000);

    ASSERT_EQ(ready, 1);

    loop.dispatch();

    EXPECT_TRUE(handler->handled);
    EXPECT_TRUE(handler->last_events & POLLIN);
}

TEST_F(EventLoopTest, modifyHandler_UpdatesPollEventsCorrectly) {
    FakeHandler* handler = new FakeHandler(sockets[0]);

    loop.addHandler(handler, POLLIN);

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

    loop.removeHandler(handler);

    const char byte = 'A';
    ASSERT_EQ(write(sockets[1], &byte, 1), 1);

    int ready = loop.wait(100);

    EXPECT_EQ(ready, 0);

    loop.dispatch();
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

    EXPECT_NO_THROW(loop.cleanup());
}

TEST_F(EventLoopTest, cleanup_CallsOnTimeoutForTimedOutHandlers) {
    bool timeout_called = false;

    FakeHandler* handler =
        new FakeHandler(sockets[0], &timeout_called);

    loop.addHandler(handler, POLLIN);

    handler->markTimedOut();

    loop.cleanup();

    EXPECT_TRUE(timeout_called);
}
