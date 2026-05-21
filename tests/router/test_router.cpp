#include <gtest/gtest.h>

#include "../../config/LocationConfig.hpp"
#include "../../config/ServerConfig.hpp"
#include "../../handlers/Router.hpp"
#include "../../logger/Logger.hpp"

// fixture with root "/" - "/" matches every URI as shortest prefix
class TestRouter : public ::testing::Test {
protected:
    void SetUp() {
        LocationConfig location_block1;
        LocationConfig location_block2;
        LocationConfig location_block3;

        location_block1.setPath("/");
        location_block2.setPath("/api");
        location_block3.setPath("/api/users");

        server_config_.addLocationBlock(location_block1);
        server_config_.addLocationBlock(location_block2);
        server_config_.addLocationBlock(location_block3);
    }

    ServerConfig server_config_;
};

// fixture without root "/" - unmatched URIs throw
class TestRouterNoRoot : public ::testing::Test {
protected:
    void SetUp() {
        LocationConfig location_block1;
        LocationConfig location_block2;

        location_block1.setPath("/api");
        location_block2.setPath("/api/users");

        server_config_.addLocationBlock(location_block1);
        server_config_.addLocationBlock(location_block2);
    }

    ServerConfig server_config_;
};

/* TestRouter */

TEST_F(TestRouter, LongestPrefixMatch) {
    Router router(server_config_);
    const LocationConfig& loc = router.resolve("/api/users/123");
    EXPECT_EQ(loc.getPath(), "/api/users");
}

TEST_F(TestRouter, ExactMatch) {
    Router router(server_config_);
    const LocationConfig& loc = router.resolve("/api");
    EXPECT_EQ(loc.getPath(), "/api");
}

TEST_F(TestRouter, RootExactMatch) {
    Router router(server_config_);
    const LocationConfig& loc = router.resolve("/");
    EXPECT_EQ(loc.getPath(), "/");
}

TEST_F(TestRouter, RootMatchesWhenNothingElseDoes) {
    Router router(server_config_);
    const LocationConfig& loc = router.resolve("/something/else");
    EXPECT_EQ(loc.getPath(), "/");
}

TEST_F(TestRouter, IntermediatePrefix) {
    Router router(server_config_);
    const LocationConfig& loc = router.resolve("/api/other");
    EXPECT_EQ(loc.getPath(), "/api");
}

TEST_F(TestRouter, SimilarPrefixDoesNotMatch) {
    Router router(server_config_);
    // /apiary shares prefix /api but is not a sub-path of /api
    const LocationConfig& loc = router.resolve("/apiary");
    EXPECT_EQ(loc.getPath(), "/");
}

/* TestRouterTrailingSlash - location paths with trailing slash */

class TestRouterTrailingSlash : public ::testing::Test {
protected:
    void SetUp() {
        LocationConfig location_block1;
        LocationConfig location_block2;
        LocationConfig location_block3;

        location_block1.setPath("/");
        location_block2.setPath("/api/");
        location_block3.setPath("/api/users/");

        server_config_.addLocationBlock(location_block1);
        server_config_.addLocationBlock(location_block2);
        server_config_.addLocationBlock(location_block3);
    }

    ServerConfig server_config_;
};

TEST_F(TestRouterTrailingSlash, URIWithoutSlashMatchesLocationWithSlash) {
    Router router(server_config_);
    const LocationConfig& loc = router.resolve("/api/users");
    EXPECT_EQ(loc.getPath(), "/api/users/");
}

TEST_F(TestRouterTrailingSlash, URIWithSlashMatchesLocationWithSlash) {
    Router router(server_config_);
    const LocationConfig& loc = router.resolve("/api/users/");
    EXPECT_EQ(loc.getPath(), "/api/users/");
}

TEST_F(TestRouterTrailingSlash, LongestPrefixStillWorks) {
    Router router(server_config_);
    const LocationConfig& loc = router.resolve("/api/users/123");
    EXPECT_EQ(loc.getPath(), "/api/users/");
}

TEST_F(TestRouterTrailingSlash, IntermediatePrefixWithSlash) {
    Router router(server_config_);
    const LocationConfig& loc = router.resolve("/api/other");
    EXPECT_EQ(loc.getPath(), "/api/");
}

/* TestRouterNoRoot - no root, throw on no match */

TEST_F(TestRouterNoRoot, NoMatchThrows) {
    Router router(server_config_);
    EXPECT_THROW(router.resolve("/unknown/path"), std::runtime_error);
}

TEST_F(TestRouterNoRoot, ExactMatchStillWorks) {
    Router router(server_config_);
    const LocationConfig& loc = router.resolve("/api/users");
    EXPECT_EQ(loc.getPath(), "/api/users");
}

TEST_F(TestRouterNoRoot, LongestPrefixStillWorks) {
    Router router(server_config_);
    const LocationConfig& loc = router.resolve("/api/users/456");
    EXPECT_EQ(loc.getPath(), "/api/users");
}
