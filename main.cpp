#include <csignal>
#include <cstdlib>

#include "config/Config.hpp"
#include "config/ConfigParser.hpp"
#include "logger/Logger.hpp"
#include "server/Server.hpp"

/* add this when polling is implemented
volatile sig_atomic_t g_running = 1;

void handleSigInt(int) {
    g_running = 0;
}
*/
int main(int argc, char** argv) {
    //	uncomment when polling is implemented
    //	signal(SIGINT, handleSigInt);
    //	signal(SIGPIPE, SIG_IGN);

    if (argc > 3) {
        std::cerr << "usage: ./webserv [config file] [log level] (default "
                     "config: conf/default.conf)\n";
        return EXIT_FAILURE;
    }

    std::string level = (argc == 3) ? argv[2] : "INFO";

    if (!initLogger(level))
        return EXIT_FAILURE;
    Logger::get().setLogFile("webserv.log");

    std::string file_path = (argc > 1) ? argv[1] : "conf/default.conf";

    try {
        ConfigParser parser;
        Config config = parser.parse(file_path);
    } catch (const std::exception& e) {
        return EXIT_FAILURE;
    }

    Config config;

    // create one server block
    ServerConfig server_conf;
    server_conf.setPort(8080);

    // add to config
    config.addServerBlock(server_conf);

    Server server(config);
    if (!server.start())
        return (EXIT_FAILURE);

    return EXIT_SUCCESS;
}
