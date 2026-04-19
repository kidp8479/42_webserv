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
	EXPECT_EQ(0, client.getState());
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
