#include <fcntl.h>
#include <gtest/gtest.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>

#include "../../server/Fd.hpp"

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

TEST(FdRAII, ReleaseTransfersOwnership) {
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

TEST(FdReset, ResetFromValidClosesOldFd) {
    int fd1 = open("/tmp/test_fd1.txt", O_CREAT | O_RDWR, 0644);
    int fd2 = open("/tmp/test_fd2.txt", O_CREAT | O_RDWR, 0644);

    ASSERT_NE(fd1, -1);
    ASSERT_NE(fd2, -1);

    {
        Fd wrapper(fd1);
        ASSERT_TRUE(wrapper.valid());

        wrapper.reset(fd2);

        EXPECT_EQ(wrapper.getFd(), fd2);

        // old fd must be closed
        EXPECT_EQ(fcntl(fd1, F_GETFD), -1);
    }

    // wrapper should still manage fd2 and close it now
    EXPECT_EQ(fcntl(fd2, F_GETFD), -1);
}

TEST(FdReset, ResetFromInvalidSetsNewFd) {
    int fd = open("/tmp/test_fd.txt", O_CREAT | O_RDWR, 0644);
    ASSERT_NE(fd, -1);

    Fd wrapper;  // invalid fd (-1)
    ASSERT_FALSE(wrapper.valid());

    wrapper.reset(fd);

    EXPECT_TRUE(wrapper.valid());
    EXPECT_EQ(wrapper.getFd(), fd);

    // cleanup check
    EXPECT_NE(fcntl(fd, F_GETFD), -1);
}

TEST(FdReset, ResetMultipleTimesNoLeaks) {
    int fd1 = open("/tmp/test_fd1.txt", O_CREAT | O_RDWR, 0644);
    int fd2 = open("/tmp/test_fd2.txt", O_CREAT | O_RDWR, 0644);
    int fd3 = open("/tmp/test_fd3.txt", O_CREAT | O_RDWR, 0644);

    ASSERT_NE(fd1, -1);
    ASSERT_NE(fd2, -1);
    ASSERT_NE(fd3, -1);

    {
        Fd wrapper(fd1);

        wrapper.reset(fd2);
        EXPECT_EQ(wrapper.getFd(), fd2);

        wrapper.reset(fd3);
        EXPECT_EQ(wrapper.getFd(), fd3);

        // intermediate fds should be closed
        EXPECT_EQ(fcntl(fd1, F_GETFD), -1);
        EXPECT_EQ(fcntl(fd2, F_GETFD), -1);
    }

    // final fd should also be closed by destructor
    EXPECT_EQ(fcntl(fd3, F_GETFD), -1);
}

TEST(FdRAII, ReleaseSetsInternalFdToMinusOne) {
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
