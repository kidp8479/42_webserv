/**
 * @brief Unit tests for ConfigTokenizer using Google Test.
 *
 * Test structure:
 *   TEST(GroupName, TestName)
 *  - GroupName is the class under test,
 *  - TestName describes the specific case.
 *
 * Key macros:
 *   EXPECT_THROW(expr, ExceptionType) passes if expr throws ExceptionType
 *   EXPECT_NO_THROW(expr)             passes if expr throws nothing
 *   EXPECT_EQ(a, b)                   passes if a == b
 *   EXPECT_TRUE(expr)                 passes if expr is true
 *
 * To build and run:
 *   cd tests && make && ./run_tests
 *
 */

#include <gtest/gtest.h>

#include "../config/ConfigTokenizer.hpp"
#include "../logger/Logger.hpp"

// tests for checkPathExists()
// [FAIL] => path doesn't exist (stat returns -1)
// [FAIL] => path exists but is a directory (S_ISDIR is true)
// [PASS] => path exists and is a file

TEST(ConfigTokenizer_CheckPathExists, ThrowsOnNonExistentPath) {
    EXPECT_THROW(ConfigTokenizer("nonexistent.conf"), std::runtime_error);
}

TEST(ConfigTokenizer_CheckPathExists, ThrowsOnPathIsADirectory) {
    EXPECT_THROW(ConfigTokenizer("../conf"), std::runtime_error);
}

TEST(ConfigTokenizer_CheckPathExists, NoThrowsOnRightPath) {
    EXPECT_NO_THROW(ConfigTokenizer("../conf/default.conf"));
}