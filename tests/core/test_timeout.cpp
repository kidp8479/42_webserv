#include <gtest/gtest.h>
#include <unistd.h>

#include "../../core/Timeout.hpp"

TEST(TimeoutTest, DefaultConstructorNeverExpires) {
    Timeout timeout;

    EXPECT_FALSE(timeout.expired());
    EXPECT_EQ(timeout.limit(), -1);
}

TEST(TimeoutTest, ConstructorStoresLimit) {
    Timeout timeout(5);

    EXPECT_EQ(timeout.limit(), 5);
    EXPECT_FALSE(timeout.expired());
}

TEST(TimeoutTest, CopyConstructorCopiesState) {
    Timeout original(7);

    Timeout copy(original);

    EXPECT_EQ(copy.limit(), 7);
    EXPECT_FALSE(copy.expired());
}

TEST(TimeoutTest, AssignmentOperatorCopiesState) {
    Timeout original(9);
    Timeout copy;

    copy = original;

    EXPECT_EQ(copy.limit(), 9);
    EXPECT_FALSE(copy.expired());
}

TEST(TimeoutTest, ExpiredReturnsTrueAfterLimit) {
    Timeout timeout(1);

    sleep(2);

    EXPECT_TRUE(timeout.expired());
}

TEST(TimeoutTest, ResetRestartsTimer) {
    Timeout timeout(1);

    sleep(1);
    timeout.reset();

    EXPECT_FALSE(timeout.expired());

    sleep(2);

    EXPECT_TRUE(timeout.expired());
}

TEST(TimeoutTest, LimitReturnsConfiguredValue) {
    Timeout timeout(42);

    EXPECT_EQ(timeout.limit(), 42);
}

TEST(TimeoutTest, UnlimitedTimeoutNeverExpires) {
    Timeout timeout;

    sleep(2);

    EXPECT_FALSE(timeout.expired());
}
