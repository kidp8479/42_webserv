#include <gtest/gtest.h>
#include <sys/stat.h>

#include <cstdio>

#include "../../config/ConfigBuilder.hpp"
#include "../../logger/Logger.hpp"

TEST(ConfigBuilder_CheckFirstToken, ThrowsOnServerNotFirstToken) {
    EXPECT_THROW(ConfigBuilder, std::runtime_error);
}