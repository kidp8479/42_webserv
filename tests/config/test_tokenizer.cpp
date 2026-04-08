/**
 * @brief Unit tests for ConfigTokenizer using Google Test Framework.
 *
 * Test structure:
 * TEST(GroupName, TestName)
 *  - GroupName => logical test group unit name - usually name of the tested
 * class - 1 name = 1 test suite
 *  - TestName => describes the specific case.
 *
 * Key macros:
 * EXPECT_THROW(expr, ExceptionType) passes if expr throws ExceptionType
 * EXPECT_NO_THROW(expr) passes if expr throws nothing
 * EXPECT_EQ(a, b) passes if a == b
 * EXPECT_TRUE(expr) passes if expr is true
 * EXPECT_FALSE(expr) passes if expr is false
 * EXPECT_NE(a, b) passes if a != b
 * EXPECT_GT(a, b) passes if a > b
 * EXPECT_GE(a, b) passes if a >= b
 * EXPECT_LT(a, b) passes if a < b
 * EXPECT_LE(a, b) passes if a <= b
 * EXPECT_STREQ(a, b) passes if C strings are equal (strcmp == 0)
 * EXPECT_STRNE(a, b) passes if C strings are not equal
 *
 * use ASSERT type macro when you want to stop the test suite on any failure
 * ASSERT_THROW(expr, type) same as EXPECT_THROW but STOPS the test on failure
 * ASSERT_NO_THROW(expr) same as EXPECT_NO_THROW but STOPS the test on failure
 * ASSERT_EQ(a, b) same as EXPECT_EQ but STOPS the test on failure
 *
 * note: there are plenty of other macros
 *
 * To build and run from the test\ Makefile: cd tests && make && ./run_tests
 */

#include <gtest/gtest.h>

// !! include what you need for your class to run normally !!
#include "../../config/ConfigTokenizer.hpp"
#include "../../logger/Logger.hpp"

/* tests for checkPathExists()
[FAIL] => path doesn't exist (stat returns -1)
[FAIL] => path exists but is a directory (S_ISDIR is true)
[PASS] => path exists and is a file */

TEST(ConfigTokenizer_CheckPathExists, ThrowsOnNonExistentPath) {
    EXPECT_THROW(ConfigTokenizer("nonexistent.conf"), std::runtime_error);
}

TEST(ConfigTokenizer_CheckPathExists, ThrowsOnPathIsADirectory) {
    EXPECT_THROW(ConfigTokenizer("../conf"), std::runtime_error);
}

TEST(ConfigTokenizer_CheckPathExists, NoThrowOnRightPath) {
    EXPECT_NO_THROW(ConfigTokenizer("../conf/default.conf"));
}

/* tests for checkPathExists()
[FAIL] => is_open() returns false (for either wrong perms or error on open)
[PASS] => file opened */

/* when various tests need the same env (create file, init object..), we use a
 * fixture :
 *
 * class MyFixture : public ::testing::Test {
 * protected:
 *   void SetUp() => runs BEFORE each TEST_F, prepares the environment
 *   void TearDown() => runs AFTER each TEST_F, cleans up
 * };
 *
 * TEST_F(MyFixture, TestName) - uses the fixture instead of TEST()
 * lifecycle: SetUp -> TEST_F -> TearDown (repeated for each test)
 */

class ConfigTokenizerCreateFileNoPerms : public ::testing::Test {
protected:
    void SetUp() {
        std::ofstream f("tmp_no_perms.conf");
        f << "content";
        f.close();
        chmod("tmp_no_perms.conf", 0000);
    }
    void TearDown() {
        chmod("tmp_no_perms.conf", 0644);
        remove("tmp_no_perms.conf");
    }
};

TEST_F(ConfigTokenizerCreateFileNoPerms, ThrowsOnNoPermissions) {
    EXPECT_THROW(ConfigTokenizer("tmp_no_perms.conf"), std::runtime_error);
}

TEST(ConfigTokenizer_CheckReadable, NoThrowOnSaneFile) {
    EXPECT_NO_THROW(ConfigTokenizer("../conf/default.conf"));
}

// TODO

/* tests for checkExtension()
[FAIL] => no "." in name
[FAIL] => "." is the first char (hidden file)
[FAIL] => "." is in last position, nothing after
[FAIL] => extension after dot is not "conf"
[PASS] => extension is ".conf" at the right place */

/* tests for checkNotEmpty() */
// [FAIL] => file is empty (peek() returns EOF immediately)
// [PASS] => peek() returns a valid character, file has content