#include <cstdlib>

#include "config/ConfigParser.hpp"
#include "logger/Logger.hpp"

int main(int argc, char** argv) {
    if (argc > 3) {
        std::cout << "usage: ./webserv <config file> [log level]\n";
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

    return EXIT_SUCCESS;
}
