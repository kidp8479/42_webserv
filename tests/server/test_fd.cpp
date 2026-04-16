#include "../../server/Fd.hpp"
#include <gtest/gtest.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>

TEST(FdBasic, ValidFd) {
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	ASSERT_GE(fd, 0);

	Fd wrapper(fd);
	EXPECT_TRUE(wrapper.valid());
}

TEST(FdBasic, InvalidFd) {
	Fd wrapper(-1);

	EXPECT_FALSE(wrapper.valid());
}
