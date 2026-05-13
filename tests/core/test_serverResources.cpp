#include <gtest/gtest.h>

#include "../../config/ServerConfig.hpp"
#include "../../core/ServerResources.hpp"
#include "../../handlers/Router.hpp"

ServerConfig createDummyServerConfig(int port = 8083) {
    ServerConfig sconf;
    sconf.setPort(port);

    LocationConfig loc;
    loc.setPath("/");

    sconf.addLocationBlock(loc);

    return sconf;
}

TEST(ServerResources, Constructor_StoresServerConfig) {
    ServerConfig sconf = createDummyServerConfig(8080);

    ServerResources resources(sconf);

    EXPECT_EQ(resources.serverConfig().getPort(), 8080);
}

TEST(ServerResources, Constructor_DoesNotThrow) {
    ServerConfig sconf = createDummyServerConfig();

    EXPECT_NO_THROW(ServerResources resources(sconf));
}

TEST(ServerResources, Constructor_InitializesRouter) {
    ServerConfig sconf = createDummyServerConfig();

    ServerResources resources(sconf);

    EXPECT_NO_THROW(resources.router());
}

TEST(ServerResources, CopyConstructor_RebindsRouterCorrectly) {
    ServerConfig sconf = createDummyServerConfig();

    ServerResources original(sconf);
    ServerResources copy(original);

    EXPECT_NO_THROW(copy.router());
}
