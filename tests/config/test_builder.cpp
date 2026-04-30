#include <gtest/gtest.h>

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
[FAIL] => config file contains only comments (no server block)
[PASS] => minimal valid server block */

TEST(ConfigBuilder_Build, ThrowsIfFirstTokenIsNotServer) {
    EXPECT_THROW(
        buildFromFile(
            "../config/builder_test_files/server/not_first_token.conf"),
        std::runtime_error);
}

TEST(ConfigBuilder_Build, ThrowsOnNoServerBlock) {
    EXPECT_THROW(
        buildFromFile("../config/builder_test_files/server/comments_only.conf"),
        std::runtime_error);
}

TEST(ConfigBuilder_Build, NoThrowOnMinimalValidBlock) {
    EXPECT_NO_THROW(buildFromFile(
        "../config/builder_test_files/server/valid_minimal.conf"));
}

/* tests for build() multiple server blocks
[PASS] => two server blocks correctly parsed */

TEST(ConfigBuilder_Build, CorrectlyParsesMultipleServerBlocks) {
    Config config = buildFromFile(
        "../config/builder_test_files/server/"
        "valid_multiple_server_blocks.conf");
    ASSERT_EQ(config.getServerBlock().size(),
              2u);  // check if we have at least 2 server blocks or next lines
                    // would crash
    EXPECT_EQ(config.getServerBlock()[0].getPort(), 8080);  // real check
    EXPECT_EQ(config.getServerBlock()[1].getPort(), 8081);  // real check
}

/* tests for build() complete server block
[PASS] => all server-level directives correctly parsed */

TEST(ConfigBuilder_Build, CorrectlyParsesCompleteServerBlock) {
    Config config = buildFromFile(
        "../config/builder_test_files/server/valid_complete.conf");
    ASSERT_EQ(
        config.getServerBlock().size(),
        1u);  // check if we have at least a server or next lines would crash
    EXPECT_EQ(config.getServerBlock()[0].getHost(), "127.0.0.1");  // real check
    EXPECT_EQ(config.getServerBlock()[0].getPort(), 8080);         // real check
    EXPECT_EQ(config.getServerBlock()[0].getMaxBodySize(),
              10u * 1048576u);  // real check
    const std::map<int, std::string>& pages =
        config.getServerBlock()[0].getErrorPages();
    ASSERT_EQ(pages.size(), 1u);  // check size before accessing
    EXPECT_EQ(pages.at(404), "/errors/404.html");  // real check
}

/* tests for parseServerBlock()
[FAIL] => "{" missing after "server"
[FAIL] => block is never closed with "}"
[FAIL] => unknown directive inside server block */

TEST(ConfigBuilder_ParseServerBlock, ThrowsOnMissingOpenBrace) {
    EXPECT_THROW(
        buildFromFile(
            "../config/builder_test_files/server/missing_open_brace.conf"),
        std::runtime_error);
}

TEST(ConfigBuilder_ParseServerBlock, ThrowsOnUnclosedBlock) {
    EXPECT_THROW(buildFromFile(
                     "../config/builder_test_files/server/unclosed_block.conf"),
                 std::runtime_error);
}

TEST(ConfigBuilder_ParseServerBlock, ThrowsOnUnknownDirective) {
    EXPECT_THROW(
        buildFromFile(
            "../config/builder_test_files/server/unknown_directive.conf"),
        std::runtime_error);
}

/* tests for parseListen()
[FAIL] => no value after "listen" (end of file)
[FAIL] => listen value has no ":" separator
[FAIL] => ";" missing after listen value
[PASS] => host and port are correctly parsed */

TEST(ConfigBuilder_ParseListen, ThrowsOnMissingValue) {
    EXPECT_THROW(
        buildFromFile(
            "../config/builder_test_files/server/listen_missing_value.conf"),
        std::runtime_error);
}

TEST(ConfigBuilder_ParseListen, ThrowsOnMissingColon) {
    EXPECT_THROW(
        buildFromFile(
            "../config/builder_test_files/server/listen_missing_colon.conf"),
        std::runtime_error);
}

TEST(ConfigBuilder_ParseListen, ThrowsOnMissingSemicolon) {
    EXPECT_THROW(buildFromFile("../config/builder_test_files/server/"
                               "listen_missing_semicolon.conf"),
                 std::runtime_error);
}

TEST(ConfigBuilder_ParseListen, CorrectlyParsesHostAndPort) {
    Config config =
        buildFromFile("../config/builder_test_files/server/valid_minimal.conf");
    ASSERT_EQ(
        config.getServerBlock().size(),
        1u);  // check if we have at least a server or next lines would crash
    EXPECT_EQ(config.getServerBlock()[0].getHost(), "127.0.0.1");  // real check
    EXPECT_EQ(config.getServerBlock()[0].getPort(), 8080);         // real check
}

/* tests for parseClientBodySize()
[FAIL] => no value after "client_max_body_size" (end of file)
[FAIL] => ";" missing after value
[FAIL] => invalid unit suffix (not K/M/G)
[FAIL] => value overflows size_t after unit multiplication
[PASS] => kilobyte unit correctly converted
[PASS] => megabyte unit correctly converted
[PASS] => gigabyte unit correctly converted
[PASS] => no unit treated as bytes */

TEST(ConfigBuilder_ParseClientBodySize, ThrowsOnMissingValue) {
    EXPECT_THROW(buildFromFile("../config/builder_test_files/server/"
                               "client_max_body_size_missing_value.conf"),
                 std::runtime_error);
}

TEST(ConfigBuilder_ParseClientBodySize, ThrowsOnMissingSemicolon) {
    EXPECT_THROW(buildFromFile("../config/builder_test_files/server/"
                               "client_max_body_size_missing_semicolon.conf"),
                 std::runtime_error);
}

TEST(ConfigBuilder_ParseClientBodySize, ThrowsOnInvalidUnit) {
    EXPECT_THROW(buildFromFile("../config/builder_test_files/server/"
                               "client_max_body_size_invalid_unit.conf"),
                 std::runtime_error);
}

TEST(ConfigBuilder_ParseClientBodySize, ThrowsOnOverflow) {
    EXPECT_THROW(buildFromFile("../config/builder_test_files/server/"
                               "client_max_body_size_overflow.conf"),
                 std::runtime_error);
}

TEST(ConfigBuilder_ParseClientBodySize, CorrectlyParsesKilobytes) {
    Config config = buildFromFile(
        "../config/builder_test_files/server/"
        "client_max_body_size_kilobytes.conf");
    ASSERT_EQ(
        config.getServerBlock().size(),
        1u);  // check if we have at least a server or next line would crash
    EXPECT_EQ(config.getServerBlock()[0].getMaxBodySize(),
              4u * 1024u);  // real check
}

TEST(ConfigBuilder_ParseClientBodySize, CorrectlyParsesMegabytes) {
    Config config = buildFromFile(
        "../config/builder_test_files/server/"
        "client_max_body_size_megabytes.conf");
    ASSERT_EQ(
        config.getServerBlock().size(),
        1u);  // check if we have at least a server or next line would crash
    EXPECT_EQ(config.getServerBlock()[0].getMaxBodySize(),
              10u * 1048576u);  // real check
}

TEST(ConfigBuilder_ParseClientBodySize, CorrectlyParsesGigabytes) {
    Config config = buildFromFile(
        "../config/builder_test_files/server/"
        "client_max_body_size_gigabytes.conf");
    ASSERT_EQ(
        config.getServerBlock().size(),
        1u);  // check if we have at least a server or next line would crash
    EXPECT_EQ(config.getServerBlock()[0].getMaxBodySize(),
              2u * 1073741824u);  // real check
}

TEST(ConfigBuilder_ParseClientBodySize, CorrectlyParsesRawBytes) {
    Config config = buildFromFile(
        "../config/builder_test_files/server/client_max_body_size_bytes.conf");
    ASSERT_EQ(
        config.getServerBlock().size(),
        1u);  // check if we have at least a server or next line would crash
    EXPECT_EQ(config.getServerBlock()[0].getMaxBodySize(), 512u);  // real check
}

/* tests for parseErrorPage()
[FAIL] => no code after "error_page" (end of file)
[FAIL] => no path after code (end of file)
[FAIL] => ";" missing after path
[PASS] => code and path correctly stored */

TEST(ConfigBuilder_ParseErrorPage, ThrowsOnMissingCode) {
    EXPECT_THROW(
        buildFromFile(
            "../config/builder_test_files/server/error_page_missing_code.conf"),
        std::runtime_error);
}

TEST(ConfigBuilder_ParseErrorPage, ThrowsOnMissingPath) {
    EXPECT_THROW(
        buildFromFile(
            "../config/builder_test_files/server/error_page_missing_path.conf"),
        std::runtime_error);
}

TEST(ConfigBuilder_ParseErrorPage, ThrowsOnMissingSemicolon) {
    EXPECT_THROW(buildFromFile("../config/builder_test_files/server/"
                               "error_page_missing_semicolon.conf"),
                 std::runtime_error);
}

TEST(ConfigBuilder_ParseErrorPage, CorrectlyParsesCodeAndPath) {
    Config config = buildFromFile(
        "../config/builder_test_files/server/error_page_valid.conf");
    ASSERT_EQ(
        config.getServerBlock().size(),
        1u);  // check if we have at least a server or next lines would crash
    const std::map<int, std::string>& pages =
        config.getServerBlock()[0].getErrorPages();
    ASSERT_EQ(pages.size(), 1u);  // same logic for the error pages map
    EXPECT_EQ(pages.at(404), "/errors/404.html");  // real check
}

/* tests for parseLocationBlock()
[FAIL] => "{" missing after location path
[FAIL] => block is never closed with "}"
[FAIL] => unknown directive inside location block
[PASS] => path correctly stored */

TEST(ConfigBuilder_ParseLocationBlock, ThrowsOnMissingOpenBrace) {
    EXPECT_THROW(
        buildFromFile(
            "../config/builder_test_files/location/missing_open_brace.conf"),
        std::runtime_error);
}

TEST(ConfigBuilder_ParseLocationBlock, ThrowsOnUnclosedBlock) {
    EXPECT_THROW(
        buildFromFile(
            "../config/builder_test_files/location/unclosed_block.conf"),
        std::runtime_error);
}

TEST(ConfigBuilder_ParseLocationBlock, ThrowsOnUnknownDirective) {
    EXPECT_THROW(
        buildFromFile(
            "../config/builder_test_files/location/unknown_directive.conf"),
        std::runtime_error);
}

TEST(ConfigBuilder_ParseLocationBlock, CorrectlyParsesPath) {
    Config config = buildFromFile(
        "../config/builder_test_files/location/valid_complete.conf");
    ASSERT_EQ(
        config.getServerBlock().size(),
        1u);  // check if we have at least a server or next line would crash
    ASSERT_EQ(config.getServerBlock()[0].getLocationBlock().size(),
              1u);  // same logic for location block
    EXPECT_EQ(config.getServerBlock()[0].getLocationBlock()[0].getPath(),
              "/upload");  // real check
}

/* tests for parseMethods()
[FAIL] => invalid method name (not GET/POST/DELETE)
[FAIL] => ";" missing after methods
[FAIL] => empty methods list
[FAIL] => duplicate method in list
[PASS] => multiple methods correctly stored */

TEST(ConfigBuilder_ParseMethods, ThrowsOnUnknownMethodName) {
    EXPECT_THROW(
        buildFromFile(
            "../config/builder_test_files/location/invalid_method_name.conf"),
        std::runtime_error);
}

TEST(ConfigBuilder_ParseMethods, ThrowsOnMissingSemicolon) {
    EXPECT_THROW(
        buildFromFile(
            "../config/builder_test_files/location/missing_semicolon.conf"),
        std::runtime_error);
}

TEST(ConfigBuilder_ParseMethods, ThrowsOnEmptyMethodsList) {
    EXPECT_THROW(
        buildFromFile(
            "../config/builder_test_files/location/methods_empty.conf"),
        std::runtime_error);
}

TEST(ConfigBuilder_ParseMethods, ThrowsOnDuplicateMethod) {
    EXPECT_THROW(
        buildFromFile(
            "../config/builder_test_files/location/methods_duplicate.conf"),
        std::runtime_error);
}

TEST(ConfigBuilder_ParseMethods, CorrectlyStoredAllMethods) {
    Config config = buildFromFile(
        "../config/builder_test_files/location/valid_complete.conf");
    ASSERT_EQ(
        config.getServerBlock().size(),
        1u);  // check if we have at least a server or next line would crash
    ASSERT_EQ(config.getServerBlock()[0].getLocationBlock().size(),
              1u);  // same logic for location block
    const std::vector<std::string>& methods =
        config.getServerBlock()[0].getLocationBlock()[0].getMethods();
    ASSERT_EQ(methods.size(), 2u);  // check size before accessing by index
    EXPECT_EQ(methods[0], "GET");
    EXPECT_EQ(methods[1], "POST");
    EXPECT_EQ(config.getServerBlock()[0].getLocationBlock()[0].getRoot(),
              "/www/data");
    EXPECT_EQ(config.getServerBlock()[0].getLocationBlock()[0].getIndex(),
              "index.html");
    EXPECT_EQ(
        config.getServerBlock()[0].getLocationBlock()[0].getDirectoryListing(),
        true);
    EXPECT_EQ(config.getServerBlock()[0].getLocationBlock()[0].getUploadPath(),
              "/www/uploads");
    const std::map<std::string, std::string>& cgi =
        config.getServerBlock()[0].getLocationBlock()[0].getCgiInterpreters();
    ASSERT_EQ(cgi.size(), 1u);  // check size before accessing
    EXPECT_EQ(cgi.at(".php"), "/usr/bin/php-cgi");  // real check
    EXPECT_EQ(config.getServerBlock()[0].getLocationBlock()[0].getReturnCode(),
              301);
    EXPECT_EQ(config.getServerBlock()[0].getLocationBlock()[0].getReturnUrl(),
              "/new-path");
}

/* tests for parseRoot()
[FAIL] => no value after "root" (end of file)
[FAIL] => ";" missing after value
[PASS] => root path correctly stored */

TEST(ConfigBuilder_ParseRoot, ThrowsOnMissingValue) {
    EXPECT_THROW(
        buildFromFile(
            "../config/builder_test_files/location/no_value_after_root.conf"),
        std::runtime_error);
}

TEST(ConfigBuilder_ParseRoot, ThrowsOnMissingSemicolon) {
    EXPECT_THROW(buildFromFile("../config/builder_test_files/location/"
                               "root_missing_semicolon.conf"),
                 std::runtime_error);
}

TEST(ConfigBuilder_ParseRoot, CorrectlyParsesRoot) {
    Config config = buildFromFile(
        "../config/builder_test_files/location/valid_complete.conf");
    ASSERT_EQ(
        config.getServerBlock().size(),
        1u);  // check if we have at least a server or next line would crash
    ASSERT_EQ(config.getServerBlock()[0].getLocationBlock().size(),
              1u);  // same logic for location block
    EXPECT_EQ(config.getServerBlock()[0].getLocationBlock()[0].getRoot(),
              "/www/data");  // real check
}

/* tests for parseIndex()
[FAIL] => no value after "index" (end of file)
[FAIL] => ";" missing after value
[PASS] => index file correctly stored */

TEST(ConfigBuilder_ParseIndex, ThrowsOnMissingValue) {
    EXPECT_THROW(
        buildFromFile(
            "../config/builder_test_files/location/index_missing_value.conf"),
        std::runtime_error);
}

TEST(ConfigBuilder_ParseIndex, ThrowsOnMissingSemicolon) {
    EXPECT_THROW(buildFromFile("../config/builder_test_files/location/"
                               "index_missing_semicolon.conf"),
                 std::runtime_error);
}

TEST(ConfigBuilder_ParseIndex, CorrectlyParsesIndex) {
    Config config = buildFromFile(
        "../config/builder_test_files/location/valid_complete.conf");
    ASSERT_EQ(
        config.getServerBlock().size(),
        1u);  // check if we have at least a server or next line would crash
    ASSERT_EQ(config.getServerBlock()[0].getLocationBlock().size(),
              1u);  // same logic for location block
    EXPECT_EQ(config.getServerBlock()[0].getLocationBlock()[0].getIndex(),
              "index.html");  // real check
}

/* tests for parseAutoIndex()
[FAIL] => value is not "on" or "off"
[FAIL] => ";" missing after value
[PASS] => "on" sets directory listing to true
[PASS] => "off" sets directory listing to false */

TEST(ConfigBuilder_ParseAutoIndex, ThrowsOnInvalidValue) {
    EXPECT_THROW(buildFromFile("../config/builder_test_files/location/"
                               "autoindex_invalid_value.conf"),
                 std::runtime_error);
}

TEST(ConfigBuilder_ParseAutoIndex, ThrowsOnMissingSemicolon) {
    EXPECT_THROW(buildFromFile("../config/builder_test_files/location/"
                               "autoindex_missing_semicolon.conf"),
                 std::runtime_error);
}

TEST(ConfigBuilder_ParseAutoIndex, CorrectlyParsesOn) {
    Config config = buildFromFile(
        "../config/builder_test_files/location/valid_complete.conf");
    ASSERT_EQ(
        config.getServerBlock().size(),
        1u);  // check if we have at least a server or next line would crash
    ASSERT_EQ(config.getServerBlock()[0].getLocationBlock().size(),
              1u);  // same logic for location block
    EXPECT_EQ(
        config.getServerBlock()[0].getLocationBlock()[0].getDirectoryListing(),
        true);  // real check
}

TEST(ConfigBuilder_ParseAutoIndex, CorrectlyParsesOff) {
    Config config = buildFromFile(
        "../config/builder_test_files/location/valid_autoindex_off.conf");
    ASSERT_EQ(
        config.getServerBlock().size(),
        1u);  // check if we have at least a server or next line would crash
    ASSERT_EQ(config.getServerBlock()[0].getLocationBlock().size(),
              1u);  // same logic for location block
    EXPECT_EQ(
        config.getServerBlock()[0].getLocationBlock()[0].getDirectoryListing(),
        false);  // real check
}

/* tests for parseUploadPath()
[FAIL] => no value after "upload_path" (end of file)
[FAIL] => ";" missing after value
[PASS] => upload path correctly stored */

TEST(ConfigBuilder_ParseUploadPath, ThrowsOnMissingValue) {
    EXPECT_THROW(buildFromFile("../config/builder_test_files/location/"
                               "upload_path_missing_value.conf"),
                 std::runtime_error);
}

TEST(ConfigBuilder_ParseUploadPath, ThrowsOnMissingSemicolon) {
    EXPECT_THROW(buildFromFile("../config/builder_test_files/location/"
                               "upload_path_missing_semicolon.conf"),
                 std::runtime_error);
}

TEST(ConfigBuilder_ParseUploadPath, CorrectlyParsesUploadPath) {
    Config config = buildFromFile(
        "../config/builder_test_files/location/valid_complete.conf");
    ASSERT_EQ(
        config.getServerBlock().size(),
        1u);  // check if we have at least a server or next line would crash
    ASSERT_EQ(config.getServerBlock()[0].getLocationBlock().size(),
              1u);  // same logic for location block
    EXPECT_EQ(config.getServerBlock()[0].getLocationBlock()[0].getUploadPath(),
              "/www/uploads");  // real check
}

/* tests for parseCGI()
[FAIL] => no extension after "cgi" (end of file)
[FAIL] => no binary after extension (end of file)
[FAIL] => ";" missing after binary
[FAIL] => extension does not start with '.'
[PASS] => extension and binary correctly stored
[PASS] => multiple cgi entries correctly stored */

TEST(ConfigBuilder_ParseCGI, ThrowsOnMissingExtension) {
    EXPECT_THROW(
        buildFromFile(
            "../config/builder_test_files/location/cgi_missing_extension.conf"),
        std::runtime_error);
}

TEST(ConfigBuilder_ParseCGI, ThrowsOnMissingBinary) {
    EXPECT_THROW(
        buildFromFile(
            "../config/builder_test_files/location/cgi_missing_binary.conf"),
        std::runtime_error);
}

TEST(ConfigBuilder_ParseCGI, ThrowsOnMissingSemicolon) {
    EXPECT_THROW(
        buildFromFile(
            "../config/builder_test_files/location/cgi_missing_semicolon.conf"),
        std::runtime_error);
}

TEST(ConfigBuilder_ParseCGI, ThrowsOnExtensionWithoutDot) {
    EXPECT_THROW(
        buildFromFile(
            "../config/builder_test_files/location/cgi_extension_no_dot.conf"),
        std::runtime_error);
}

TEST(ConfigBuilder_ParseCGI, CorrectlyParsesSingleEntry) {
    Config config = buildFromFile(
        "../config/builder_test_files/location/valid_complete.conf");
    ASSERT_EQ(
        config.getServerBlock().size(),
        1u);  // check if we have at least a server or next line would crash
    ASSERT_EQ(config.getServerBlock()[0].getLocationBlock().size(),
              1u);  // same logic for location block
    const std::map<std::string, std::string>& cgi =
        config.getServerBlock()[0].getLocationBlock()[0].getCgiInterpreters();
    ASSERT_EQ(cgi.size(), 1u);  // check size before accessing
    EXPECT_EQ(cgi.at(".php"), "/usr/bin/php-cgi");  // real check
}

TEST(ConfigBuilder_ParseCGI, CorrectlyParsesMultipleEntries) {
    Config config = buildFromFile(
        "../config/builder_test_files/location/valid_multiple_cgi.conf");
    ASSERT_EQ(
        config.getServerBlock().size(),
        1u);  // check if we have at least a server or next line would crash
    ASSERT_EQ(config.getServerBlock()[0].getLocationBlock().size(),
              1u);  // same logic for location block
    const std::map<std::string, std::string>& cgi =
        config.getServerBlock()[0].getLocationBlock()[0].getCgiInterpreters();
    ASSERT_EQ(cgi.size(), 2u);  // check size before accessing
    EXPECT_EQ(cgi.at(".php"), "/usr/bin/php-cgi");  // real check
    EXPECT_EQ(cgi.at(".py"), "/usr/bin/python3");   // real check
}

/* tests for parseReturn()
[FAIL] => no code after "return" (end of file)
[FAIL] => no url after code (end of file)
[FAIL] => ";" missing after url
[PASS] => code and url correctly stored */

TEST(ConfigBuilder_ParseReturn, ThrowsOnMissingCode) {
    EXPECT_THROW(
        buildFromFile(
            "../config/builder_test_files/location/return_missing_code.conf"),
        std::runtime_error);
}

TEST(ConfigBuilder_ParseReturn, ThrowsOnMissingUrl) {
    EXPECT_THROW(
        buildFromFile(
            "../config/builder_test_files/location/return_missing_url.conf"),
        std::runtime_error);
}

TEST(ConfigBuilder_ParseReturn, ThrowsOnSemicolonAsUrl) {
    EXPECT_THROW(buildFromFile("../config/builder_test_files/location/"
                               "return_semicolon_as_url.conf"),
                 std::runtime_error);
}

TEST(ConfigBuilder_ParseReturn, ThrowsOnMissingSemicolon) {
    EXPECT_THROW(buildFromFile("../config/builder_test_files/location/"
                               "return_missing_semicolon.conf"),
                 std::runtime_error);
}

TEST(ConfigBuilder_ParseReturn, CorrectlyParsesCodeAndUrl) {
    Config config = buildFromFile(
        "../config/builder_test_files/location/valid_complete.conf");
    ASSERT_EQ(
        config.getServerBlock().size(),
        1u);  // check if we have at least a server or next line would crash
    ASSERT_EQ(config.getServerBlock()[0].getLocationBlock().size(),
              1u);  // same logic for location block
    EXPECT_EQ(config.getServerBlock()[0].getLocationBlock()[0].getReturnCode(),
              301);  // real check
    EXPECT_EQ(config.getServerBlock()[0].getLocationBlock()[0].getReturnUrl(),
              "/new-path");  // real check
}
