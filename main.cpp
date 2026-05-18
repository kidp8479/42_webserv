#include <csignal>
#include <cstdlib>
#include <stdexcept>

#include "config/Config.hpp"
#include "config/ConfigParser.hpp"
#include "core/Server.hpp"
#include "core/Signal.hpp"
#include "logger/Logger.hpp"

// 2 catch blocks are intentional here because
// runtime errors must be logged at the source to prevent double prints
// all other std::exceptions must NOT be logged as well
// yes its not perfect, but we did not have the hindsight when we first
// decided this.  just go with it for now.
int main(int argc, char** argv) {
    signal(SIGINT, handleSigInt);
    signal(SIGPIPE, SIG_IGN);

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

        Server server(config);
        if (!server.start())
            return EXIT_FAILURE;
    } catch (const std::runtime_error& e) {
        return EXIT_FAILURE;
    } catch (const std::exception& e) {
        LOG_ERROR() << "Fatal error: " << e.what();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
