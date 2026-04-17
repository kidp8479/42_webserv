#include "Server.hpp"
#include "Client.hpp"
#include "../logger/Logger.hpp"
#include <stdexcept>
#include <sys/socket.h> //socket
#include <cstring> // std::memset
#include <netinet/in.h> //sockaddr_in
#include <unistd.h> // close
#include <fcntl.h>
#include <cerrno>
#include <string>
#include <sstream>

Server::Server(const Config& config) : config_(config) {}

Server::~Server() {
	stop();
}

// maybe put this in a utils.hpp file?
static std::string toString(int value) {
	std::ostringstream oss;
	oss << value;
	return oss.str();
}

void	Server::setNonBlocking(int fd) {
	int flags = fcntl(fd, F_GETFL, 0);
	
	if (flags == -1) {
		serverError("fcntl(F_GETFL) failed for fd " + toString(fd));
	}
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
		serverError("fcntl(F_SETFL) failed for " + toString(fd));
	}
}

void Server::setupSocket(int port) {
	Fd server_fd(socket(AF_INET, SOCK_STREAM, 0));
	if(!server_fd.valid()){
		serverError("socket() failed");
	}


	int opt = 1;
	// an integer flag used to configure a socket option.
	// Allow this socket to reuse an address (port), even if it was recently used.
	// without this if we run server, stop and try to restart we'll get 
	// bind() failed: address alreayd in use
	if (setsockopt(server_fd.getFd(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		serverError("setsockopt() failed");
	}
	sockaddr_in addr;
	std::memset(&addr, 0, sizeof(addr));

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;  // later use host_
	addr.sin_port = htons(port);

	if (bind(server_fd.getFd(), (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		serverError("bind() failed on port " + toString(port));
	}
	if (listen(server_fd.getFd(), kBACKLOG) < 0){
		serverError("listen() failed");
	}
	setNonBlocking(server_fd.getFd());

	// transfer ownership of fd to server with release. if we dont do this
	// fd will go out of scope after this function ends and it will close the
	// fd automatically, which is what we dont want. release() saves current
	// fd in tmp and sets the current fd to -1. and the tmp fd goes into
	// sockets_ which will need to be closed manually in close()
	// this method adds a bit more complexity, but the trade-off is that
	// we dont need to keep track of closing the fd at each error path.
	sockets_.push_back(server_fd.release());
	LOG_INFO() << "Listening on port " << port;
}

int	Server::acceptClient() {
	sockaddr_in	client_addr;
	socklen_t	client_len = sizeof(client_addr);

	// Single-socket, single-threaded setup for now: we only listen on sockets_[0].
	// This will be extended later (e.g. with poll/epoll) to handle multiple
	// listening sockets and concurrent client readiness.
	Fd client_fd(accept(sockets_[0], (struct sockaddr*)&client_addr, &client_len));

	if (!client_fd.valid()) {
		// accept failed or no connection ready
		// ignore for now (poll will handle this properly later)
		return -1;
	}

	try {
		setNonBlocking(client_fd.getFd());
		LOG_DEBUG() << "Setting client " << client_fd.getFd() << " to non-blocking";
	} catch (const std::exception& e) {
		LOG_ERROR() << "Failed to set client fd " << client_fd.getFd() << " to non-blocking";
		return -1;
	}

	LOG_INFO() << "Client " << client_fd.getFd() << " connected";
	// transfer ownership of fd here, since at the end of this function
	// Fd will go out of scope and it will close the fd, an we dont want that
	// we need it for later.
	return client_fd.release();
}

void	Server::handleRead(Client& client)
{
	LOG_DEBUG() << "Reading from fd " << client.getFd();
	// we need a raw buffer here becasue recv expects raw memory
	// if we use std::string buffer:
	// it has no allocated size, writing to it is UB
	char buffer[1024];
	ssize_t bytes = recv(client.getFd(), buffer, sizeof(buffer), 0);
	LOG_DEBUG() << "Received " << bytes << " bytes from fd " << client.getFd();
	if (bytes > 0) {
		client.getRequest().append(buffer, bytes);

		//TODO: check if request is complete
		client.getResponse().buildFrom(client.getRequest());
		client.setState(Client::kWriting);
		LOG_DEBUG() << "Client " << client.getFd()
					<< " switching to WRITING state";
	}
	else if (bytes == 0) {
		client.setState(Client::kDone);
	}
	else {
		// Without poll/epoll, we cannot distinguish between "try again later"
		// and real errors, so any failure is treated as a dead connection.
		client.setState(Client::kDone);
		LOG_INFO() << "Client " << client.getFd() << " disconnected or error";
	}
}

void	Server::handleWrite(Client& client) {
	const std::string& response = client.getResponse().getRaw();

	ssize_t sent = send(client.getFd(),
			response.c_str() + client.getBytesSent(),
			response.size() - client.getBytesSent(),
			0);

	if (sent > 0) {
		client.addBytesSent(sent);
		if (client.getBytesSent() >= response.size()) {
			client.setState(Client::kDone);
		}
	} else {
		client.setState(Client::kDone);
	}
}

void	Server::serverError(const std::string& msg) {
	std::string full = "Server: " + msg;
	LOG_ERROR() << full;
	throw std::runtime_error(full);
}

bool Server::start() {
	LOG_INFO() << "Server starting...";
	try {
		const std::vector<ServerConfig>& servers =
			config_.getServerBlock();

		if (servers.empty()) {
			serverError("No servers configured");
		}

		for (size_t i = 0; i < servers.size(); i++) {
			int port = servers[i].getPort();

			if (port == PORT_NOT_SET) {
				serverError("Port not set");
			}

			setupSocket(port);
			LOG_INFO() << "Server listening setup complete";
		}

		while (true) {
			//accept new clients
			int client_fd = acceptClient();
			if (client_fd >= 0) {
				clients_.insert(std::make_pair(client_fd, new Client(client_fd)));
			}
			for(std::map<int, Client*>::iterator it = clients_.begin();
					it != clients_.end(); ) {
				Client& client = *it->second;

				if (client.getState() == Client::kReading) {
					handleRead(client);
				} else if (client.getState() == Client::kWriting) {
					handleWrite(client);
				}
				
				if (client.getState() == Client::kDone) {
					delete it->second;
					clients_.erase(it++);
				} else {
					++it;
				}
			}
		}
	}
	catch (const std::exception& e) {
		return false;
	}
	return true;
}

void	Server::stop() {
	for (std::map<int, Client*>::iterator it = clients_.begin();
			it != clients_.end(); ++it) {
		delete it->second;
	}
	clients_.clear();

	// since the transfer of the fd has been pased to sockets_ ,
	// we still need to close manually here
	for (size_t i = 0; i < sockets_.size(); i++) {
		close(sockets_[i]);
	}
	sockets_.clear();
}

// getters
const std::vector<int>& Server::getSockets() const {
	return sockets_;
}
