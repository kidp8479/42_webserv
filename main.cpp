#include <cstdlib>

#include "config/ConfigTokenizer.hpp"
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

    // LOG_DEBUG() << "test log debug";
    // LOG_INFO() << "test log info";
    // LOG_WARNING() << "test log warning";
    // LOG_ERROR() << "test log error";

    std::string path = (argc > 1) ? argv[1] : "conf/default.conf";

    try {
        ConfigTokenizer tokenizer(path);
    } catch (const std::exception& e) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
