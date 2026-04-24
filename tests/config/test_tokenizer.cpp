/**
 * @brief Unit tests for ConfigTokenizer using Google Test Framework.
 *
 * Test structure:
 * TEST(GroupName, TestName)
 *  - GroupName => logical test group unit name, usually name of the tested
 * class, 1 name = 1 test suite
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
 * To build: cd tests && make
 * To run all: make test
 * To run one: make test_tokenizer (runs bin/test_tokenizer)
 */

#include <gtest/gtest.h>
#include <sys/stat.h>

#include <cstdio>

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
    EXPECT_THROW(ConfigTokenizer("../../conf"), std::runtime_error);
}

TEST(ConfigTokenizer_CheckPathExists, NoThrowOnRightPath) {
    EXPECT_NO_THROW(ConfigTokenizer("../../conf/default.conf"));
}

/* tests for checkReadable()
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

class ConfigTokenizer_CheckReadable : public ::testing::Test {
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

TEST_F(ConfigTokenizer_CheckReadable, ThrowsOnNoPermissions) {
    EXPECT_THROW(ConfigTokenizer("tmp_no_perms.conf"), std::runtime_error);
}
// no need for the fixture here but I want the grouptest to have the same name
TEST_F(ConfigTokenizer_CheckReadable, NoThrowOnSaneFile) {
    EXPECT_NO_THROW(ConfigTokenizer("../../conf/default.conf"));
}

/* tests for checkExtension()
[FAIL] => multiple "."
[FAIL] => no "." in name
[FAIL] => "." is the first char (hidden file)
[FAIL] => "." is in last position, nothing after
[FAIL] => extension after dot is not "conf"
[FAIL] => CONF is uppercase (nginx rejects)
[PASS] => extension is ".conf" at the right place */

TEST(ConfigTokenizer_CheckExtensions, ThrowsOnMultipleDots) {
    EXPECT_THROW(
        ConfigTokenizer("../config/tokenizer_test_files/file.conf.conf"),
        std::runtime_error);
}

TEST(ConfigTokenizer_CheckExtensions, ThrowsOnFileHasNoDot) {
    EXPECT_THROW(
        ConfigTokenizer("../config/tokenizer_test_files/has_no_dot_conf"),
        std::runtime_error);
}

TEST(ConfigTokenizer_CheckExtensions, ThrowsOnDotFirstChar) {
    EXPECT_THROW(
        ConfigTokenizer("../config/tokenizer_test_files/.dot_first_char.conf"),
        std::runtime_error);
}

TEST(ConfigTokenizer_CheckExtensions, ThrowsOnDotLastChar) {
    EXPECT_THROW(
        ConfigTokenizer("../config/tokenizer_test_files/dot_last_char.conf."),
        std::runtime_error);
}

TEST(ConfigTokenizer_CheckExtensions, ThrowsWrongExtension) {
    EXPECT_THROW(
        ConfigTokenizer("../config/tokenizer_test_files/wrong_extension.py"),
        std::runtime_error);
}

TEST(ConfigTokenizer_CheckExtensions, ThrowsOnUppercaseExtension) {
    EXPECT_THROW(ConfigTokenizer("../config/tokenizer_test_files/file.CONF"),
                 std::runtime_error);
}

TEST(ConfigTokenizer_CheckExtensions, NoThrowOnSaneFile) {
    EXPECT_NO_THROW(ConfigTokenizer("../../conf/default.conf"));
}

/* tests for checkNotEmpty() */
// [FAIL] => file is empty (peek() returns EOF immediately)
// [PASS] => peek() returns a valid character, file has content

TEST(ConfigTokenizer_CheckNotEmpty, ThrowOnFileEmpty) {
    EXPECT_THROW(ConfigTokenizer("../config/tokenizer_test_files/empty.conf"),
                 std::runtime_error);
}

TEST(ConfigTokenizer_CheckNotEmpty, NoThrowOnSaneFile) {
    EXPECT_NO_THROW(ConfigTokenizer("../../conf/default.conf"));
}

/* tests for tokenize()
 *
 * test file: tokenizer_test_files/test_tokenizer.conf
 *   server {
 *       listen 8080;
 *   }
 *
 * expected token list (6 tokens):
 *   token[0] = "server"  line 1
 *   token[1] = "{"       line 1
 *   token[2] = "listen"  line 2
 *   token[3] = "8080"    line 2
 *   token[4] = ";"       line 2
 *   token[5] = "}"       line 3
 *
 * [PASS] => correct token count
 * [PASS] => correct token values
 * [PASS] => correct line numbers
 *
 * note: you have more tests directly in ConfigTokenizer via LOG_DEBUG()
 * that will print all your tokens/line direclty
 */

class ConfigTokenizer_Tokenize : public ::testing::Test {
protected:
    void SetUp() {
        tokenizer_ = nullptr;
        tokenizer_ = new ConfigTokenizer(
            "../config/tokenizer_test_files/test_tokenizer.conf");
        tokens_ = tokenizer_->getTokenList();
    }
    void TearDown() {
        delete tokenizer_;
    }
    ConfigTokenizer* tokenizer_;
    std::vector<Token> tokens_;
};

TEST_F(ConfigTokenizer_Tokenize, CorrectTokenCount) {
    EXPECT_EQ(tokens_.size(), 6u);
}

TEST_F(ConfigTokenizer_Tokenize, FirstTokenIsServer) {
    EXPECT_EQ(tokens_[0].value, "server");
}

TEST_F(ConfigTokenizer_Tokenize, FirstTokenIsOnLine1) {
    EXPECT_EQ(tokens_[0].line, 1u);
}

/* tests for tokenize() edge cases
 *
 * test files needed in ../config/tokenizer_test_files/:
 *   comment_only.conf       => "# this is a comment"
 *   inline_comment.conf     => "server {\n    listen 8080; # comment\n}"
 *   multi_spaces.conf       => "server  {" (multiple spaces between tokens)
 *   semicolon_attached.conf => "server {\n    listen 8080;\n}"
 *   empty_lines.conf        => "\n\nserver {\n\n    listen 8080;\n\n}"
 *
 * [PASS] => comment line produces no tokens
 * [PASS] => inline comment is ignored, tokens before # are kept
 * [PASS] => multiple spaces between tokens don't produce empty tokens
 * [PASS] => semicolon attached to word produces same tokens as separated
 * [PASS] => empty lines are silently skipped
 */

TEST(ConfigTokenizer_Tokenize_EdgeCases, CommentLineProducesNoTokens) {
    ConfigTokenizer tokenizer(
        "../config/tokenizer_test_files/comment_only.conf");
    EXPECT_EQ(tokenizer.getTokenList().size(), 0u);
}

TEST(ConfigTokenizer_Tokenize_EdgeCases, InlineCommentIgnored) {
    ConfigTokenizer tokenizer(
        "../config/tokenizer_test_files/inline_comment.conf");
    const std::vector<Token>& tokens = tokenizer.getTokenList();
    // expected: "server" "{" "listen" "8080" ";" "}"
    // anything after # on the listen line should be dropped
    ASSERT_EQ(tokens.size(), 6u);
    EXPECT_EQ(tokens[0].value, "server");
    EXPECT_EQ(tokens[1].value, "{");
    EXPECT_EQ(tokens[2].value, "listen");
    EXPECT_EQ(tokens[3].value, "8080");
    EXPECT_EQ(tokens[4].value, ";");
    EXPECT_EQ(tokens[5].value, "}");
}

TEST(ConfigTokenizer_Tokenize_EdgeCases, MultipleSpacesBetweenTokens) {
    ConfigTokenizer tokenizer(
        "../config/tokenizer_test_files/multi_spaces.conf");
    const std::vector<Token>& tokens = tokenizer.getTokenList();
    // extra spaces should not produce empty tokens
    for (size_t i = 0; i < tokens.size(); i++) {
        EXPECT_FALSE(tokens[i].value.empty());
    }
}

TEST(ConfigTokenizer_Tokenize_EdgeCases, SemicolonAttachedToWord) {
    // "listen 8080;" and "listen 8080 ;" should produce identical token lists
    ConfigTokenizer attached(
        "../config/tokenizer_test_files/semicolon_attached.conf");
    ConfigTokenizer separated(
        "../config/tokenizer_test_files/test_tokenizer.conf");

    const std::vector<Token>& t1 = attached.getTokenList();
    const std::vector<Token>& t2 = separated.getTokenList();

    ASSERT_EQ(t1.size(), t2.size());
    for (size_t i = 0; i < t1.size(); i++) {
        EXPECT_EQ(t1[i].value, t2[i].value);
    }
}

TEST(ConfigTokenizer_Tokenize_EdgeCases, EmptyLinesIgnored) {
    ConfigTokenizer tokenizer(
        "../config/tokenizer_test_files/empty_lines.conf");
    const std::vector<Token>& tokens = tokenizer.getTokenList();
    // same content as test_tokenizer.conf, just with empty lines added
    // token count and values should be identical
    ASSERT_EQ(tokens.size(), 6u);
    EXPECT_EQ(tokens[0].value, "server");
    EXPECT_EQ(tokens[1].value, "{");
    EXPECT_EQ(tokens[2].value, "listen");
    EXPECT_EQ(tokens[3].value, "8080");
    EXPECT_EQ(tokens[4].value, ";");
    EXPECT_EQ(tokens[5].value, "}");
}
