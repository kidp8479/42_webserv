#include <gtest/gtest.h>

#include "../../config/ConfigBuilder.hpp"
#include "../../config/ConfigTokenizer.hpp"
#include "../../config/ConfigValidator.hpp"
#include "../../logger/Logger.hpp"

// helper: tokenize a file, run build() and validate() on the result
static Config buildFromFile(const std::string& path) {
    Config config;

    ConfigTokenizer tokenizer(path);

    ConfigBuilder builder;
    config = builder.build(tokenizer.getTokenList());

    ConfigValidator validator;
    validator.validate(config);
    return config;
}

/* tests for checkPort()
[FAIL] => port is 0
[FAIL] => port is negative
[FAIL] => port is more than 65535
[FAIL] => port is not set
[PASS] => port is in range [1-65535]
*/
TEST(ConfigValidator_checkPort, ThrowsIfPortIs0) {
    EXPECT_THROW(
        buildFromFile(
            "../config/validator_test_files/server/invalid_port_0.conf"),
        std::runtime_error);
}

TEST(ConfigValidator_checkPort, ThrowsIfPortIsNegative) {
    EXPECT_THROW(
        buildFromFile(
            "../config/validator_test_files/server/invalid_port_negative.conf"),
        std::runtime_error);
}

TEST(ConfigValidator_checkPort, ThrowsIfPortIsTooBig) {
    EXPECT_THROW(
        buildFromFile(
            "../config/validator_test_files/server/invalid_port_too_big.conf"),
        std::runtime_error);
}

TEST(ConfigValidator_checkPort, ThrowsIfNotSet) {
    EXPECT_THROW(
        buildFromFile(
            "../config/validator_test_files/server/invalid_port_not_set.conf"),
        std::runtime_error);
}

TEST(ConfigValidator_checkPort, NoThrowsValidPort) {
    EXPECT_NO_THROW(
        buildFromFile("../config/validator_test_files/server/valid_port.conf"));
}

TEST(ConfigValidator_checkPort, NoThrowsValidPortLimits) {
    EXPECT_NO_THROW(buildFromFile(
        "../config/validator_test_files/server/valid_port_limits.conf"));
}

/* tests for checkHost()
[FAIL] => host is an invalid string (not localhost)
[FAIL] => IP address is not in range [0-255]
[FAIL] => IP adress contains empty members
[PASS] => host is localhost
[PASS] => host is a valid IP address = 4 members from [0-255] range
[PASS] => host is a valid IP address = 4 members from [0-255] range limits
*/
TEST(ConfigValidator_checkHost, ThrowsIfHostIsInvalidString) {
    EXPECT_THROW(
        buildFromFile(
            "../config/validator_test_files/server/invalid_host_string.conf"),
        std::runtime_error);
}

TEST(ConfigValidator_checkHost, ThrowsIfHostIPIsOutOfRange) {
    EXPECT_THROW(buildFromFile("../config/validator_test_files/server/"
                               "invalid_host_ip_out_of_range.conf"),
                 std::runtime_error);
}

TEST(ConfigValidator_checkHost, ThrowsIfHostIPHasEmptyMember) {
    EXPECT_THROW(buildFromFile("../config/validator_test_files/server/"
                               "invalid_host_ip_empty_member.conf"),
                 std::runtime_error);
}

TEST(ConfigValidator_checkHost, NoThrowsIfHostIsLocalhost) {
    EXPECT_NO_THROW(buildFromFile(
        "../config/validator_test_files/server/valid_host_localhost.conf"));
}

TEST(ConfigValidator_checkHost, NoThrowsIfHostHasValidIp) {
    EXPECT_NO_THROW(buildFromFile(
        "../config/validator_test_files/server/valid_host_ip.conf"));
}

TEST(ConfigValidator_checkHost, NoThrowsIfHostHasValidIpLimits) {
    EXPECT_NO_THROW(buildFromFile(
        "../config/validator_test_files/server/valid_host_up_limits.conf"));
}

/* tests for checkErrorCode()
[FAIL] => error code is not in range [400-599]
[PASS] => error code is in range [400-599]
*/
TEST(ConfigValidator_checkErrorCode, ThrowsIfErrorCodeInvalid) {
    EXPECT_THROW(
        buildFromFile(
            "../config/validator_test_files/server/invalid_error_code.conf"),
        std::runtime_error);
}

TEST(ConfigValidator_checkErrorCode, NoThrowsIfErroCodeValid) {
    EXPECT_NO_THROW(buildFromFile(
        "../config/validator_test_files/server/valid_error_code.conf"));
}

/* tests for checkDuplicateHostPort()
[FAIL] => two server blocks with identical host:port
[PASS] => two server blocks with same host, different ports
[PASS] => two server blocks with different hosts, same port
*/
TEST(ConfigValidator_checkDuplicateHostPort, ThrowsIfDuplicateHostPort) {
    EXPECT_THROW(buildFromFile("../config/validator_test_files/server/"
                               "invalid_duplicate_host_port.conf"),
                 std::runtime_error);
}

TEST(ConfigValidator_checkDuplicateHostPort, NoThrowsIfSameHostDifferentPort) {
    EXPECT_NO_THROW(
        buildFromFile("../config/validator_test_files/server/"
                      "valid_same_host_different_port.conf"));
}

TEST(ConfigValidator_checkDuplicateHostPort, NoThrowsIfDifferentHostSamePort) {
    EXPECT_NO_THROW(
        buildFromFile("../config/validator_test_files/server/"
                      "valid_different_host_same_port.conf"));
}

/* tests for checkDuplicatePath()
[FAIL] => two location blocks (inside the same server block) with identical path
[PASS] => two location blocks (inside the same server block) with different path
*/
TEST(ConfigValidator_checkDuplicatePath, ThrowsIfSameLocationPath) {
    EXPECT_THROW(buildFromFile("../config/validator_test_files/location/"
                               "invalid_path_duplicate.conf"),
                 std::runtime_error);
}

TEST(ConfigValidator_checkDuplicatePath, NoThrowsIfDifferentLocationPath) {
    EXPECT_NO_THROW(buildFromFile(
        "../config/validator_test_files/location/valid_path_different.conf"));
}