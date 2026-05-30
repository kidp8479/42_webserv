#include "Listener.hpp"

#include <fcntl.h>
// #include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <cerrno>
#include <cstring>
#include <stdexcept>

#include "../logger/Logger.hpp"
#include "Client.hpp"
#include "Fd.hpp"

/**
 * we make a listen object that owns the Fd object
 * this class is now responsible for setting up the listen socket and
 * the lifetime of the fd
 */
Listener::Listener(EventLoop& loop, const ServerResources& resources)
    : fd_(socket(AF_INET, SOCK_STREAM, 0)), loop_(loop), resources_(resources) {
    if (!fd_.valid()) {
        LOG_ERROR() << "[Listener] socket() failed";
        throw std::runtime_error("[listener] socket() failed");
    }
    int opt = 1;
    if (setsockopt(fd_.getFd(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) <
        0) {
        LOG_ERROR() << "[Listener] setsockopt() failed";
        throw std::runtime_error("[listener] setsockopt() failed");
    }
    setupSocket();
    setNonBlocking(fd_.getFd());
    loop_.addHandler(this, POLLIN);
}

Listener::~Listener() {
}

void Listener::setupSocket() {
    const ServerConfig& config = resources_.getServerConfig();

    struct addrinfo hints;
    struct addrinfo* res = NULL;

    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    std::ostringstream port_stream;
    port_stream << config.getPort();

    int ret = getaddrinfo(config.getHost().c_str(), port_stream.str().c_str(),
                          &hints, &res);

    if (ret != 0 || !res) {
        std::ostringstream oss;
        oss << "[Listener] getaddrinfo() failed: " << gai_strerror(ret);

        LOG_ERROR() << oss.str();
        throw std::runtime_error(oss.str());
    }

    if (bind(fd_.getFd(), res->ai_addr, res->ai_addrlen) < 0) {
        freeaddrinfo(res);

        std::ostringstream oss;
        oss << "[Listener] bind() failed on " << config.getHost() << ":"
            << config.getPort();

        LOG_ERROR() << oss.str();
        throw std::runtime_error(oss.str());
    }
    freeaddrinfo(res);

    if (listen(fd_.getFd(), SOMAXCONN) < 0) {
        LOG_ERROR() << "[Listener] listen() failed";
        throw std::runtime_error("[Listener] listen() failed");
    }
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
                break;  // no more clients to accept
            }
            if (errno == EMFILE || errno == ENFILE) {
                LOG_ERROR()
                    << "[Listener] fd limit reached: " << strerror(errno);
                break;
            }
            LOG_ERROR() << "[Listener] accept failed: " << strerror(errno);
            continue;  // try next iteration
        }
        // accept succeed, we set up client
        try {
            setNonBlocking(client_fd);
            Client* client = new Client(client_fd, loop_, resources_);

            loop_.addHandler(client, POLLIN);
        } catch (const std::exception& e) {
            close(client_fd);
            LOG_ERROR() << "[Listener] client setup failed: " << e.what();
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
        oss << "[listener] fcntl(F_SETFD) failed for fd " << fd;
        throw std::runtime_error(oss.str());
    }
}

bool Listener::isDone() const {
    return false;
}

const char* Listener::name() const {
    return "Listener";
}

// listeners never time out but it's part of the IEventHandler interface
// so we just set to false
bool Listener::isTimedOut() const {
    return false;
}
