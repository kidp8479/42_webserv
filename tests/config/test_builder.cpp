#include <gtest/gtest.h>
#include <sys/stat.h>

#include <cstdio>

#include "../../config/ConfigBuilder.hpp"
#include "../../config/ConfigTokenizer.hpp"
#include "../../logger/Logger.hpp"

// helper: tokenize a file and run build() on the result
static Config buildFromFile(const std::string& path) {
    ConfigTokenizer tokenizer(path);
    ConfigBuilder builder;
    return builder.build(tokenizer.getTokenList());
}

/* tests for build() top-level dispatch
[FAIL] => first token is not "server"
[PASS] => minimal valid server block */

TEST(ConfigBuilder_Build, ThrowsIfFirstTokenIsNotServer) {
    EXPECT_THROW(
        buildFromFile(
            "../config/builder_test_files/server_not_first_token.conf"),
        std::runtime_error);
}

TEST(ConfigBuilder_Build, NoThrowOnMinimalValidBlock) {
    EXPECT_NO_THROW(
        buildFromFile("../config/builder_test_files/valid_minimal.conf"));
}

/* tests for parseServerBlock()
[FAIL] => "{" missing after "server"
[FAIL] => block is never closed with "}" */

TEST(ConfigBuilder_ParseServerBlock, ThrowsOnMissingOpenBrace) {
    EXPECT_THROW(
        buildFromFile("../config/builder_test_files/missing_open_brace.conf"),
        std::runtime_error);
}

TEST(ConfigBuilder_ParseServerBlock, ThrowsOnUnclosedBlock) {
    EXPECT_THROW(
        buildFromFile("../config/builder_test_files/unclosed_block.conf"),
        std::runtime_error);
}

/* tests for parseListen()
[FAIL] => listen value has no ":" separator
[FAIL] => ";" missing after listen value
[PASS] => host and port are correctly parsed */

TEST(ConfigBuilder_ParseListen, ThrowsOnMissingColon) {
    EXPECT_THROW(
        buildFromFile("../config/builder_test_files/listen_missing_colon.conf"),
        std::runtime_error);
}

TEST(ConfigBuilder_ParseListen, ThrowsOnMissingSemicolon) {
    EXPECT_THROW(
        buildFromFile(
            "../config/builder_test_files/listen_missing_semicolon.conf"),
        std::runtime_error);
}

TEST(ConfigBuilder_ParseListen, CorrectlyParsesHostAndPort) {
    Config config =
        buildFromFile("../config/builder_test_files/valid_minimal.conf");
    ASSERT_EQ(config.getServerBlock().size(), 1u);
    EXPECT_EQ(config.getServerBlock()[0].getHost(), "127.0.0.1");
    EXPECT_EQ(config.getServerBlock()[0].getPort(), 8080);
}

/* tests for parseClientBodySize()
[PASS] => megabyte unit correctly converted
[PASS] => no unit treated as bytes */

TEST(ConfigBuilder_ParseClientBodySize, CorrectlyParsesMegabytes) {
    Config config = buildFromFile(
        "../config/builder_test_files/client_max_body_size_megabytes.conf");
    ASSERT_EQ(config.getServerBlock().size(), 1u);
    EXPECT_EQ(config.getServerBlock()[0].getMaxBodySize(), 10u * 1048576u);
}

TEST(ConfigBuilder_ParseClientBodySize, CorrectlyParsesRawBytes) {
    Config config = buildFromFile(
        "../config/builder_test_files/client_max_body_size_bytes.conf");
    ASSERT_EQ(config.getServerBlock().size(), 1u);
    EXPECT_EQ(config.getServerBlock()[0].getMaxBodySize(), 512u);
}

/* tests for parseErrorPage()
[FAIL] => no code after "error_page" (end of file)
[FAIL] => no path after code (end of file)
[FAIL] => ";" missing after path
[PASS] => code and path correctly stored */

TEST(ConfigBuilder_ParseErrorPage, ThrowsOnMissingCode) {
    EXPECT_THROW(
        buildFromFile(
            "../config/builder_test_files/error_page_missing_code.conf"),
        std::runtime_error);
}

TEST(ConfigBuilder_ParseErrorPage, ThrowsOnMissingPath) {
    EXPECT_THROW(
        buildFromFile(
            "../config/builder_test_files/error_page_missing_path.conf"),
        std::runtime_error);
}

TEST(ConfigBuilder_ParseErrorPage, ThrowsOnMissingSemicolon) {
    EXPECT_THROW(
        buildFromFile(
            "../config/builder_test_files/error_page_missing_semicolon.conf"),
        std::runtime_error);
}

TEST(ConfigBuilder_ParseErrorPage, CorrectlyParsesCodeAndPath) {
    Config config =
        buildFromFile("../config/builder_test_files/error_page_valid.conf");
    ASSERT_EQ(config.getServerBlock().size(), 1u);
    const std::map<int, std::string>& pages =
        config.getServerBlock()[0].getErrorPages();
    ASSERT_EQ(pages.size(), 1u);
    EXPECT_EQ(pages.at(404), "/errors/404.html");
}
