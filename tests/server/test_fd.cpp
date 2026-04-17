#include "../../server/Fd.hpp"
#include <gtest/gtest.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>

TEST(FdBasic, DefaultFdIsMinusOne) {
	Fd fd;

	EXPECT_FALSE(fd.valid());
	ASSERT_EQ(-1, fd.getFd());
}

TEST(FdBasic, ValidFd) {
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	ASSERT_GE(fd, 0);

	Fd wrapper(fd);
	EXPECT_TRUE(wrapper.valid());
}

TEST(FdBasic, InvalidFd) {
	Fd fd(-1);

	EXPECT_FALSE(fd.valid());
}

TEST(FdRAII, DestructorClosesFd) {
	int fd = open("/tmp/test_fd.txt", O_CREAT | O_RDONLY, 0644);
	ASSERT_NE(fd, -1);
	{
		Fd wrapper(fd);
		ASSERT_TRUE(wrapper.valid());
	}
	int result = fcntl(fd, F_GETFD);

	EXPECT_EQ(-1, result);
	EXPECT_EQ(EBADF, errno);
}

TEST(FdRAII, ReleaseTransfersOwnership)
{
	int fd = open("/tmp/test_fd.txt", O_CREAT | O_RDONLY, 0644);
	ASSERT_NE(fd, -1);

	{
		Fd wrapper(fd);
		ASSERT_TRUE(wrapper.valid());

		int released = wrapper.release();
		EXPECT_EQ(fd, released);
	}
	int result = fcntl(fd, F_GETFD);

	EXPECT_NE(-1, result);
}

TEST(FdRAII, ReleaseSetsInternalFdToMinusOne)
{
	int fd = open("/tmp/test_fd.txt", O_CREAT | O_RDONLY, 0644);
	ASSERT_NE(fd, -1);

	Fd wrapper(fd);
	ASSERT_TRUE(wrapper.valid());

	wrapper.release();

	EXPECT_EQ(wrapper.getFd(), -1);
}

/** uncomment to test
TEST(FdCopy, CopyShouldNotCompile) {
	Fd fd1 = open("/tmp/test_fd.txt", O_CREAT | O_RDONLY, 0644);
	ASSERT_NE(-1, fd1.getFd());

	Fd fd2(fd1);
}
*/

/** uncomment to test
TEST(FdCopy, AssignmentShouldNotCompile) {
	Fd fd1 = open("/tmp/test_fd.txt", O_CREAT | O_RDONLY, 0644);
	ASSERT_NE(-1, fd1.getFd());

	Fd fd2 = fd1;

}
*/
