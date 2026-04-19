#include "../../server/Client.hpp"
#include "../../server/Fd.hpp"
#include <gtest/gtest.h>
#include <fcntl.h>
#include <unistd.h>

/*****************************************************************************/
/*                      Basic constructor / destructor                       */
/*****************************************************************************/

TEST(ClientBasicTest, CreateFromValidFd) {
	int fd = open("/tmp/test_fd.txt", O_CREAT | O_RDONLY, 0644);
	ASSERT_TRUE(fd >= 0);
	
	Client client(fd);
	EXPECT_EQ(fd, client.getFd());
	EXPECT_EQ(Client::kReading, client.getState());
	EXPECT_FALSE(client.getRequest().isComplete());
	EXPECT_EQ("", client.getResponse().getRaw());
}

TEST(ClientBasicTest, FdIsClosedOnDestruction) {
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

/*****************************************************************************/
/*                          fixture: ClientTestBase                          */
/*****************************************************************************/

class ClientTestBase : public ::testing::Test {
protected:
	int fds[2];
	std::unique_ptr<Client> client;
	int peer;
	//side note:  my mental model for this set up:
	//think of client as what we're testing (us the person) on the phone
	//and peer is the friend on the other end of the phone

	void SetUp() override {
		// socketpair simulates already accepted connection:
		// socket -> bind -> listen -> accept
		// so we can directly test handleRead and handleWrite behavior
		ASSERT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, fds));
		client = std::make_unique<Client>(fds[0]);
		peer = fds[1];
	}

	void TearDown() override {
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

	void prepareCompleteRequest() {
		sendToClient("Get / HTTP/1.1\r\n\r\n");
		Client::ReadResult result = client->read();
		ASSERT_EQ(Client::kReadComplete, result);

		client->getResponse().buildFrom(client->getRequest());
	}
};

/*****************************************************************************/
/*                             Client State Tests                            */
/*****************************************************************************/
class ClientStateTest : public ClientTestBase {};

TEST_F(ClientStateTest, InitialStateIsReading) {
	EXPECT_EQ(Client::kReading, client->getState());
}

TEST_F(ClientStateTest, ReadCompleteTransitionsToWriting) {
	sendToClient("GET / HTTP/1.1\r\nHost: test\r\n\r\n");

	Client::ReadResult result = client->read();

	EXPECT_EQ(Client::kReadComplete, result);
	EXPECT_EQ(Client::kWriting, client->getState());
}

TEST_F(ClientStateTest, WriteDoneTransitionsToDone) {
	prepareCompleteRequest();
	Client::WriteResult result = client->write();

	EXPECT_EQ(Client::kWriteDone, result);
	EXPECT_EQ(Client::kDone, client->getState());
}

TEST_F(ClientStateTest, ReadClosedSetsDone) {
	//simulate client disconnect
	close(peer);

	Client::ReadResult result = client->read();

	EXPECT_EQ(Client::kReadClosed, result);
	EXPECT_EQ(Client::kDone, client->getState());
}

/*****************************************************************************/
/*                             Client Bytes Tests                            */
/*****************************************************************************/
class ClientBytes : public ClientTestBase {};

TEST_F(ClientBytesTest, FullWriteCompletesInOneGo) {
	prepareCompleteRequest();
	Client::WriteResult result = client->write();

	EXPECT_EQ(Client::kWriteDone, result);
	EXPECT_EQ(Client::kDone, client->getState());
}

TEST_F(ClientBytesTest, WriteEventuallyCompletes) {
	prepareCompleteRequest();
	while (client->getState() != Client::kDone) {
		Client::WriteResult result = client->write();
		EXPECT_NE(Client::kWriteError, result);
	}
	EXPECT_EQ(Client::kDone, client->getState());
}

TEST_F(ClientBytesTest, WriteMayRequireMultipleCalls)
{
	prepareCompleteRequest();
	while (client->getState() != Client::kDone)
	{
		Client::WriteResult result = client->write();
		EXPECT_NE(Client::kWriteError, result);
	}
	EXPECT_EQ(Client::kDone, client->getState());
}

/*****************************************************************************/
/*                             Client Read Tests                             */
/*****************************************************************************/
class ClientReadTest : public ClientTestBase {};

TEST_F(ClientReadTest, ReadCompleteTransitionsToWriting) {
	sendToClient("Get / HTTP/1.1\r\nHost: test\r\n\r\n");
	
	Client::ReadResult result = client->read();

	EXPECT_EQ(Client::kReadComplete, result);
	EXPECT_EQ(Client::kWriting, client->getState());
	
}
/*
TEST_F(ClientReadTest, DataIsStoredInRequestBuffer) {
	sendToClient("Hello");

	client->read();

}

TEST_F(ClientReadTest, PartialRequestReturnsPending) {
	sendToClient("GET / HTT");

	EXPECT_EQ(Client::kReadPending, client->read());
}
*/

/*****************************************************************************/
/*                             Client Write Tests                            */
/*****************************************************************************/
