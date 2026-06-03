#include "Listener.hpp"

#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <cerrno>
#include <cstring>
#include <stdexcept>

#include "../logger/Logger.hpp"
#include "Client.hpp"
#include "Fd.hpp"
#include "FdUtils.hpp"

/**
 * @brief Creates and registers a listening socket.
 *
 * Initializes the server socket, enables address reuse,
 * binds and listens through setupSocket(), configures the
 * socket as non-blocking, and registers it with the EventLoop.
 *
 * @param loop Event loop that monitors the listener.
 * @param resources Shared server configuration and resources.
 *
 * @throws std::runtime_error If socket creation or configuration fails.
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
    FdUtils::setNonBlocking(fd_.getFd());
    loop_.addHandler(this, POLLIN);
}

/**
 * @brief Destroys the listener.
 * The underlying file descriptor is managed by Fd and will be
 * released automatically.
 */
Listener::~Listener() {
}

/**
 * @brief Binds and starts listening on the configured address.
 * Resolves host/port using getaddrinfo(), binds the socket,
 * and enables listening mode (SOMAXCONN).
 *
 * @note Uses ServerConfig from ServerResources for host/port
 * resolution. All system errors are escalated as runtime exceptions.
 *
 * @throws std::runtime_error If getaddrinfo, bind, or listen fails.
 */
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

/**
 * @brief Returns the listening socket file descriptor.
 * @return The underlying socket fd.
 */
int Listener::getFd() const {
    return fd_.getFd();
}

/**
 * @brief Handles incoming connection events.
 * Accepts new client connections when the listening socket
 * becomes readable (POLLIN).
 */
void Listener::handle(short revents) {
    if (revents & POLLIN) {
        acceptClients();
    }
}

/**
 * @brief Accepts incoming client connections.
 * Continuously accepts new connections until the socket is
 * non-blocking exhausted (EAGAIN/EWOULDBLOCK). Each client
 * is configured as non-blocking, wrapped in a Client handler,
 * and registered with the EventLoop.
 *
 * @note Handles system limits (EMFILE/ENFILE) gracefully and
 * logs errors without crashing the server.
 *
 * @note Each accepted client is immediately handed over to
 * the EventLoop, which assumes ownership of the Client object.
 */
void Listener::acceptClients() {
    while (true) {
        struct sockaddr_in peer_addr;
        socklen_t peer_len = sizeof(peer_addr);
        int client_fd =
            accept(fd_.getFd(), (struct sockaddr*)&peer_addr, &peer_len);

        if (client_fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            if (errno == EMFILE || errno == ENFILE) {
                LOG_ERROR()
                    << "[Listener] fd limit reached: " << strerror(errno);
                break;
            }
            LOG_ERROR() << "[Listener] accept failed: " << strerror(errno);
            continue;
        }
        try {
            FdUtils::setNonBlocking(client_fd);
            unsigned int ip = ntohl(peer_addr.sin_addr.s_addr);
            std::ostringstream oss;
            oss << ((ip >> 24) & 0xFF) << "." << ((ip >> 16) & 0xFF) << "."
                << ((ip >> 8) & 0xFF) << "." << ((ip >> 0) & 0xFF);
            Client* client =
                new Client(client_fd, loop_, resources_, oss.str());
            loop_.addHandler(client, POLLIN);
        } catch (const std::exception& e) {
            close(client_fd);
            LOG_ERROR() << "[Listener] client setup failed: " << e.what();
        }
    }
}

/**
 * @brief Indicates the listener is always active.
 * @return Always false (listener never terminates).
 */
bool Listener::isDone() const {
    return false;
}

/**
 * @brief Returns the handler name used for logging.
 * @return "Listener"
 */
const char* Listener::name() const {
    return "Listener";
}

/**
 * @brief Indicates whether the listener has timed out.
 * @return Always false (listener does not timeout).
 */
bool Listener::isTimedOut() const {
    return false;
}
