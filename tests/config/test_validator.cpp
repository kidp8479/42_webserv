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