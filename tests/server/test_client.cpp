#include "../../server/Client.hpp"
#include "../../server/Fd.hpp"
#include <gtest/gtest.h>
#include <fcntl.h>
#include <unistd.h>

TEST(ClientBasic, CreateFromValidFd) {
	int fd = open("/tmp/test_fd.txt", O_CREAT | O_RDONLY, 0644);
	ASSERT_TRUE(fd >= 0);
	
	Client client(fd);
	EXPECT_EQ(fd, client.getFd());
	EXPECT_EQ(Client::kReading, client.getState());
	//temporary test for now
	EXPECT_TRUE(client.getRequest().isComplete());
	EXPECT_EQ("", client.getResponse().getRaw());
}

TEST(ClientBasic, FdIsClosedOnDestruction) {
	int fd = open("/tmp/test_fd.txt", O_CREAT | O_RDONLY, 0644);
	ASSERT_TRUE(fd >= 0);
	{	
		Client client(fd);
	}
	char buffer[10];
	ssize_t result = read(fd, &buffer, 1);
	EXPECT_EQ(-1, result);
	EXPECT_EQ(EBADF, errno);
}

class ClientState : public ::testing::Test {
protected:
	int fds[2];
	Client* client;
	int peer;
	//side note:  my mental model for this set up:
	//think of client as what we're testing (us the person) on the phone
	//and peer is the friend on the other end of the phone

	void SetUp() {
		// socketpair simulates already accepted connection:
		// socket -> bind -> listen -> accept
		// so we can directly test handleRead and handleWrite behavior
		ASSERT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, fds));
		client = new Client(fds[0]);
		peer = fds[1];
	}

	void TearDown() {
		delete client;
		close(peer);
	}

	// helper: send data to client
	void sendToClient(const std::string& data) {
		//when we do write, ex :write(peer, "hello it's me", 13)
		// in case you didnt catch that - Adele song reference xD
		// pipeline:
			// test code (peer side) -> write() -> socketpair connection ->
			// Client fd -> Client::read()
		ASSERT_GT(write(peer, data.c_str(), data.size()), 0);
	}
};

TEST_F(ClientState, InitialStateIsReading) {
	EXPECT_EQ(Client::kReading, client->getState());
}

TEST_F(ClientState, ReadCompleteTransitionsToWriting) {
	sendToClient("GET / HTTP/1.1\r\nHost: test\r\n\r\n");

	Client::ReadResult result = client->read();

	EXPECT_EQ(Client::kReadComplete, result);
	EXPECT_EQ(Client::kWriting, client->getState());
}

TEST_F(ClientState, WriteDoneTransitionsToDone) {
	sendToClient("GET / HTTP/1.1\r\nHost: test\r\n\r\n");

	// read request
	ASSERT_EQ(Client::kReadComplete, client->read());

	// build response
	client->getResponse().buildFrom(client->getRequest());

	//  write response
	Client::WriteResult result = client->write();

	EXPECT_EQ(Client::kWriteDone, result);
	EXPECT_EQ(Client::kDone, client->getState());
}

TEST_F(ClientState, ReadClosedSetsDone) {
	//simulate client disconnect
	close(peer);

	Client::ReadResult result = client->read();

	EXPECT_EQ(Client::kReadClosed, result);
	EXPECT_EQ(Client::kDone, client->getState());
}
