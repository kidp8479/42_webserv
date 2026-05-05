#include "Listener.hpp"

#include <cstring>
#include "Fd.hpp"
#include <sys/socket.h>
#include <stdexcept>
#include <fcntl.h>
#include <cerrno>
#include "../logger/Logger.hpp"
#include "Client.hpp"
#include <netinet/in.h>

/**
 * we make a listen object that owns the Fd object
 * this class is now responsible for setting up the listen socket and
 * the lifetime of the fd
 */
Listener::Listener(int port, EventLoop& loop) :
	fd_(socket(AF_INET, SOCK_STREAM, 0)), loop_(loop)
{
	if (!fd_.valid()) {
		throw std::runtime_error("[listener] socket() failed");
	}
	int opt = 1;
	if (setsockopt(fd_.getFd(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		throw std::runtime_error("[listener] setsockopt() failed");
	}
	sockaddr_in addr;
	std::memset(&addr, 0, sizeof(addr));

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);

	if (bind(fd_.getFd(), (struct sockaddr*)&addr, sizeof(addr)) < 0 ) {
		std::ostringstream oss;
		oss << "[listener] bind() failed on port " << port;
		throw std::runtime_error(oss.str());
	}
	if(listen(fd_.getFd(), SOMAXCONN) < 0) {
		throw std::runtime_error("[listener] listen() failed");
	}
	setNonBlocking(fd_.getFd());
	loop_.addHandler(this, POLLIN);
}

int Listener::getFd() const {
	return fd_.getFd();
}

void Listener::handle(short revents) {
	if (revents & POLLIN) {
		acceptClients();
	}
}

void Listener::acceptClients() {
	while (true) {
		int client_fd = accept(fd_.getFd(), NULL, NULL);

		// accept failed
		if (client_fd < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				break ; // no more clients to accept
			}
			LOG_ERROR() << "[Listener] accept failed: " << strerror(errno);
			continue ; //try next iteration
		}
		//accept succeed, we set up client
		try {
			setNonBlocking(client_fd);
			Client* client = new Client(client_fd, loop_);

			loop_.addHandler(client, POLLIN);
		}
		catch (const std::exception& e) {
			close(client_fd);
			LOG_ERROR() << "Listener] client setup failed: " << e.what();
		}
	}
}

void Listener::setNonBlocking(int fd) {
	int flags = fcntl(fd, F_GETFL, 0);

	if (flags == -1) {
		std::ostringstream oss;
		oss << "[listener] fcntl(F_GETFL) failed for fd " << fd;
		throw std::runtime_error(oss.str());
	}
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
		std::ostringstream oss;
		oss << "[listener] fcntl(F_SETFL) failed for fd " << fd;
		throw std::runtime_error(oss.str());
	}
	// prevent fd leaking into child process
	if (fcntl(fd, F_SETFD, FD_CLOEXEC) == -1) {
		std::ostringstream oss;
		oss << "[listener] fcntl(F_SETFL) failed for fd " << fd;
		throw std::runtime_error(oss.str());
	}
}

bool Listener::isDone() const {
	return false;
}

const char* Listener::name() const {
	return "Listener";
}
