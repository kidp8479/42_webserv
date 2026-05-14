#include <gtest/gtest.h>
#include "../../core/EventLoop.hpp"
#include "../../core/IEventHandler.hpp"
#include <sys/socket.h>

class FakeHandler : public IEventHandler {
public: 
	explicit FakeHandler(int fd) :
		handled(false),
		last_events(0),
		fd_(fd),
		done_(false)
	{}

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
		ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets), 0);
	}

	void TearDown() {
		close(sockets[0]);
		close(sockets[1]);
	}
};

TEST_F(EventLoopTest, addHandler_AddsFdToPollList) {
	FakeHandler* handler = new FakeHandler(sockets[0]);;
	loop.addHandler(handler, POLLIN);

	const char byte = 'A';

	ASSERT_EQ(write(sockets[1], &byte, 1), 1);

	int ready = loop.wait(1000);

	ASSERT_EQ(ready, 1);

	loop.dispatch();

	EXPECT_TRUE(handler->handled);
	EXPECT_TRUE(handler->last_events & POLLIN);
}
