#include "logger/Logger.hpp"

int main(int argc, char **argv) {
	if (argc < 2 || argc > 3) {
		std::cout << "usage: ./webserv [config] [log level]\n";
		return 1;
	}

	std::string level = (argc == 3) ? argv[2] : "INFO";

	if (!initLogger(level))
		return 1;
	Logger::get().setLogFile("webserv.log");

	LOG_DEBUG() << "test log debug";
	LOG_INFO() << "test log info";
	LOG_WARNING() << "test log warning";
	LOG_ERROR() << "test log warning";

    return 0;
}
