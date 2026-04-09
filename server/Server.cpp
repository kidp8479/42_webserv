#include "Server.hpp"
#include "../logger/Logger.hpp"
#include <stdexcept>
#include <sys/socket.h> //socket
#include <cstring> // std::memset
#include <netinet/in.h> //sockaddr_in
#include <unistd.h> // close

// pulled this number from the man, in case anyone is wondering
static const int BACKLOG = 128;

Server::Server(const Config& config) : config_(config) {}

Server::~Server() {
	stop();
}

void Server::setupSocket(int port) {
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(server_fd < 0)
	{
		LOG_ERROR() << "socket() failed";
		throw std::runtime_error("socket() failed");
	}

	int opt = 1;
	// an integer flag used to configure a socket option.
	// Allow this socket to reuse an address (port), even if it was recently used.
	// without this if we run server, stop and try to restart we'll get 
	// bind() failed: address alreayd in use
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		close(server_fd);
		LOG_ERROR() << "setsockopt() failed";
		throw std::runtime_error("setsockopt() failed");
	}

	sockaddr_in addr;
	std::memset(&addr, 0, sizeof(addr));

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;  // later use host_
	addr.sin_port = htons(port);

	if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		close(server_fd);
		LOG_ERROR() << "bind() failed on port" << port;
		throw std::runtime_error("bind() failed");
	}

	if (listen(server_fd, BACKLOG) < 0){
		close(server_fd);
		LOG_ERROR() << "listen() failed";
		throw std::runtime_error("listen() failed");
	}

	sockets_.push_back(server_fd);

	LOG_INFO() << "Listening on port " << port;
}

bool Server::start() {
	try {
		const std::vector<ServerConfig>& servers =
			config_.getServerBlock();

		if (servers.empty()) {
			LOG_ERROR() << "No servers configured";
			throw std::runtime_error("No servers configured");
		}

		for (size_t i = 0; i < servers.size(); i++) {
			int port = servers[i].getPort();

			if (port == PORT_NOT_SET)
				throw std::runtime_error("Port not set");

			setupSocket(port);
		}

		//temp loop
		while (true) {
			sleep(1);
		}
	}
	catch (const std::exception& e) {
		return false;
	}
	return true;
}

void	Server::stop() {
	for (size_t i = 0; i < sockets_.size(); i++) {
		close(sockets_[i]);
	}
	sockets_.clear();
}

// getters
const std::vector<int>& Server::getSockets() const {
	return sockets_;
}

