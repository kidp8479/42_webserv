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

/* tests for build() dispatcher
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

/* tests for build() multiple server blocks
[PASS] => two server blocks correctly parsed */

TEST(ConfigBuilder_Build, CorrectlyParsesMultipleServerBlocks) {
    Config config = buildFromFile(
        "../config/builder_test_files/valid_multiple_server_blocks.conf");
    ASSERT_EQ(config.getServerBlock().size(), 2u);
    EXPECT_EQ(config.getServerBlock()[0].getPort(), 8080);
    EXPECT_EQ(config.getServerBlock()[1].getPort(), 8081);
}

/* tests for parseServerBlock()
[FAIL] => "{" missing after "server"
[FAIL] => block is never closed with "}"
[FAIL] => unknown directive inside server block */

TEST(ConfigBuilder_ParseServerBlock, ThrowsOnMissingOpenBrace) {
    EXPECT_THROW(
        buildFromFile(
            "../config/builder_test_files/server_missing_open_brace.conf"),
        std::runtime_error);
}

TEST(ConfigBuilder_ParseServerBlock, ThrowsOnUnclosedBlock) {
    EXPECT_THROW(buildFromFile(
                     "../config/builder_test_files/server_unclosed_block.conf"),
                 std::runtime_error);
}

TEST(ConfigBuilder_ParseServerBlock, ThrowsOnUnknownDirective) {
    EXPECT_THROW(
        buildFromFile("../config/builder_test_files/unknown_directive.conf"),
        std::runtime_error);
}

/* tests for parseListen()
[FAIL] => no value after "listen" (end of file)
[FAIL] => listen value has no ":" separator
[FAIL] => ";" missing after listen value
[PASS] => host and port are correctly parsed */

TEST(ConfigBuilder_ParseListen, ThrowsOnMissingValue) {
    EXPECT_THROW(
        buildFromFile("../config/builder_test_files/listen_missing_value.conf"),
        std::runtime_error);
}

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
[FAIL] => no value after "client_max_body_size" (end of file)
[FAIL] => ";" missing after value
[PASS] => kilobyte unit correctly converted
[PASS] => megabyte unit correctly converted
[PASS] => gigabyte unit correctly converted
[PASS] => no unit treated as bytes */

TEST(ConfigBuilder_ParseClientBodySize, ThrowsOnMissingValue) {
    EXPECT_THROW(buildFromFile("../config/builder_test_files/"
                               "client_max_body_size_missing_value.conf"),
                 std::runtime_error);
}

TEST(ConfigBuilder_ParseClientBodySize, ThrowsOnMissingSemicolon) {
    EXPECT_THROW(buildFromFile("../config/builder_test_files/"
                               "client_max_body_size_missing_semicolon.conf"),
                 std::runtime_error);
}

TEST(ConfigBuilder_ParseClientBodySize, CorrectlyParsesKilobytes) {
    Config config = buildFromFile(
        "../config/builder_test_files/client_max_body_size_kilobytes.conf");
    ASSERT_EQ(config.getServerBlock().size(), 1u);
    EXPECT_EQ(config.getServerBlock()[0].getMaxBodySize(), 4u * 1024u);
}

TEST(ConfigBuilder_ParseClientBodySize, CorrectlyParsesMegabytes) {
    Config config = buildFromFile(
        "../config/builder_test_files/client_max_body_size_megabytes.conf");
    ASSERT_EQ(config.getServerBlock().size(), 1u);
    EXPECT_EQ(config.getServerBlock()[0].getMaxBodySize(), 10u * 1048576u);
}

TEST(ConfigBuilder_ParseClientBodySize, CorrectlyParsesGigabytes) {
    Config config = buildFromFile(
        "../config/builder_test_files/client_max_body_size_gigabytes.conf");
    ASSERT_EQ(config.getServerBlock().size(), 1u);
    EXPECT_EQ(config.getServerBlock()[0].getMaxBodySize(), 2u * 1073741824u);
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

/* tests for parseLocationBlock()
[FAIL] => "{" missing after location path
[FAIL] => block is never closed with "}"
[FAIL] => unknown directive inside location block
[PASS] => path correctly stored */

TEST(ConfigBuilder_ParseLocationBlock, ThrowsOnMissingOpenBrace) {
    EXPECT_THROW(
        buildFromFile(
            "../config/builder_test_files/location_missing_open_brace.conf"),
        std::runtime_error);
}

TEST(ConfigBuilder_ParseLocationBlock, ThrowsOnUnclosedBlock) {
    EXPECT_THROW(
        buildFromFile(
            "../config/builder_test_files/location_unclosed_block.conf"),
        std::runtime_error);
}

TEST(ConfigBuilder_ParseLocationBlock, ThrowsOnUnknownDirective) {
    EXPECT_THROW(
        buildFromFile(
            "../config/builder_test_files/location_unknown_directive.conf"),
        std::runtime_error);
}

/* tests for parseMethods()
[FAIL] => invalid method name (not GET/POST/DELETE)
[FAIL] => ";" missing after methods
[PASS] => single method correctly stored
[PASS] => multiple methods correctly stored */

/* tests for parseRoot()
[FAIL] => no value after "root" (end of file)
[FAIL] => ";" missing after value
[PASS] => root path correctly stored */

/* tests for parseIndex()
[FAIL] => no value after "index" (end of file)
[FAIL] => ";" missing after value
[PASS] => index file correctly stored */

/* tests for parseAutoIndex()
[FAIL] => value is not "on" or "off"
[FAIL] => ";" missing after value
[PASS] => "on" sets directory listing to true
[PASS] => "off" sets directory listing to false */

/* tests for parseUploadPath()
[FAIL] => no value after "upload_path" (end of file)
[FAIL] => ";" missing after value
[PASS] => upload path correctly stored */

/* tests for parseCGI()
[FAIL] => no extension after "cgi" (end of file)
[FAIL] => no binary after extension (end of file)
[FAIL] => ";" missing after binary
[PASS] => extension and binary correctly stored
[PASS] => multiple cgi entries correctly stored */

/* tests for parseReturn()
[FAIL] => no code after "return" (end of file)
[FAIL] => no url after code (end of file)
[FAIL] => ";" missing after url
[PASS] => code and url correctly stored */
