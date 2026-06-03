#include <gtest/gtest.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../../handlers/CgiSpawner.hpp"
#include "../../core/EventLoop.hpp"
#include "../../core/Client.hpp"
#include "../../core/ServerResources.hpp"
#include "../../config/ServerConfig.hpp"
#include "../../config/LocationConfig.hpp"
#include "../../http/Request.hpp"

class CgiSpawnerTest : public ::testing::Test {
protected:
    int fds[2];
    EventLoop loop;

    ServerConfig createConfig() {
        ServerConfig cfg;

        LocationConfig loc;
        loc.setPath("/cgi-bin");
        loc.setRoot("./test_files");

        cfg.addLocationBlock(loc);
        return cfg;
    }

    void SetUp() override {
        ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);
    }

    void TearDown() override {
        close(fds[0]);
        close(fds[1]);
    }
};

TEST_F(CgiSpawnerTest, RejectsInvalidScriptPath) {
    ServerConfig cfg = createConfig();
    ServerResources res(cfg);

    Request req;

    Client client(fds[0], loop, res, "127.0.0.1");

    const LocationConfig& loc = cfg.getLocationBlock()[0];

    CgiSpawner spawner(loop);

    EXPECT_FALSE(spawner.spawn(req, loc, client));
}

TEST_F(CgiSpawnerTest, RejectsUnknownExtension) {
    ServerConfig cfg = createConfig();
    ServerResources res(cfg);

    Request req;

    Client client(fds[0], loop, res, "127.0.0.1");

    const LocationConfig& loc = cfg.getLocationBlock()[0];

    CgiSpawner spawner(loop);

    EXPECT_FALSE(spawner.spawn(req, loc, client));
}

TEST_F(CgiSpawnerTest, SpawnDoesNotCrash) {
    ServerConfig cfg = createConfig();
    ServerResources res(cfg);

    Request req;

    Client client(fds[0], loop, res, "127.0.0.1");

    const LocationConfig& loc = cfg.getLocationBlock()[0];

    CgiSpawner spawner(loop);

    EXPECT_NO_THROW({
        spawner.spawn(req, loc, client);
    });
}
