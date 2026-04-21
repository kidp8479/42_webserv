#include "Server.hpp"

#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <string>

#include "../logger/Logger.hpp"
#include "Client.hpp"

// maybe put this in a utils.hpp file?
static std::string toString(int value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}
/**
 * @brief Constructs the Server using a validated configuration.
 *
 * Stores a reference to the configuration which defines all server behavior
 * (ports, routes, etc.). The Server does not own the configuration.
 *
 * @param config Parsed and validated server configuration
 *
 * @note The reference must outlive the Server instance.
 */
Server::Server(const Config& config) : config_(config) {
}

/**
 * @brief Destroys the Server and releases all owned resources.
 *
 * Ensures all sockets and client connections are properly closed by
 * delegating cleanup to stop().
 *
 * @note stop() is safe to call multiple times.
 */
Server::~Server() {
    stop();
}

/**
 * @brief Starts the server main loop (blocking call).
 *
 * Initializes all configured listening sockets, then enters the main event loop
 * responsible for accepting clients and handling I/O.
 *
 * The server operates in a single-threaded, state-driven loop:
 * - Accept new client connections
 * - Dispatch read/write based on client state
 * - Clean up completed clients
 *
 * Clients are dynamically allocated (new/delete) because:
 * - Client is non-copyable (owns a file descriptor via RAII)
 * - Server must maintain stable object addresses in a map
 * - Ownership is explicitly controlled to avoid accidental copies or double
 * frees
 *
 * @note This implementation currently handles only a single request per client.
 *       Persistent connections and multiple pipelined requests will be
 *       implemented in a later iteration.
 *
 * @return true if the server exits cleanly, false on fatal error
 */
bool Server::start() {
    LOG_INFO() << "Server starting...";
    try {
        const std::vector<ServerConfig>& servers = config_.getServerBlock();

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
            // accept new incoming connections
            int raw_fd = acceptClient();
            if (raw_fd >= 0) {
                LOG_DEBUG() << "Accepted new connection, raw_fd: " << raw_fd;

                // Store client by fd: maps raw socket fd to its owning Client
                // instance
                clients_.insert(std::make_pair(raw_fd, new Client(raw_fd)));
                LOG_DEBUG()
                    << "Client created and stored in map, fd: " << raw_fd;
            }

            // iterate over all active clients continuously
            for (std::map<int, Client*>::iterator it = clients_.begin();
                 it != clients_.end();) {
                Client& client = *it->second;

                // handle client based on its current state
                if (client.getState() == Client::kReading) {
                    // read incoming data (append to requeest buffer)
                    handleRead(client);
                } else if (client.getState() == Client::kWriting) {
                    // send response data (may be partial, tracked by
                    // bytes_sent_)
                    handleWrite(client);
                }
                // cleanup finished clients
                if (client.getState() == Client::kDone) {
                    delete it->second;
                    clients_.erase(it++);
                } else {
                    ++it;
                }
            }
        }
    } catch (const std::exception& e) {
        return false;
    }
    return true;
}

/**
 * @brief Server shutdown routine responsible for deterministic cleanup.
 *
 * The server owns:
 * - dynamically allocated Client objects
 * - listening socket file descriptors
 *
 * This function enforces explicit ownership cleanup to prevent:
 * - file descriptor leaks
 * - double-close scenarios
 * - dangling Client pointers
 *
 * @note Safe to call multiple times.
 */
void Server::stop() {
    for (std::map<int, Client*>::iterator it = clients_.begin();
         it != clients_.end(); ++it) {
        LOG_DEBUG() << "Client*[" << it->second->getFd() << " ] deleted";
        delete it->second;
    }
    clients_.clear();

    // since the transfer of the fd has been passed to sockets_ ,
    // we still need to close manually here
    for (size_t i = 0; i < sockets_.size(); i++) {
        close(sockets_[i]);
        LOG_DEBUG() << "sockets_[" << i << " ] deleted";
    }
    sockets_.clear();
}

/**
 * @brief Returns the list of active listening socket file descriptors.
 *
 * Exposes the server's listening sockets for testing and diagnostics.
 *
 * @return Constant reference to the internal vector of socket file descriptors.
 */
const std::vector<int>& Server::getSockets() const {
    return sockets_;
}

/**
 * @brief Creates, configures, and registers a listening socket.
 *
 * Sets up a TCP listening socket for the given port and configures it
 * for non-blocking operation and address reuse.
 *
 * The function performs all required steps to make the socket usable
 * by the server event loop:
 * - Creates a TCP socket
 * - Enables address reuse to allow fast restarts
 * - Binds the socket to the configured port
 * - Marks it as listening for incoming connections
 * - Sets it to non-blocking mode for event-driven I/O
 *
 * The resulting file descriptor is stored in sockets_ and managed by
 * the Server for later use and cleanup.
 *
 * @note Ownership is explicitly transferred from the local RAII wrapper
 *       to the Server to ensure controlled lifetime management.
 *
 * @warning Uses manual ownership transfer (Fd::release()) to prevent
 *          premature closure when the RAII object goes out of scope.
 */
void Server::setupSocket(int port) {
    Fd server_fd(socket(AF_INET, SOCK_STREAM, 0));
    if (!server_fd.valid()) {
        serverError("socket() failed");
    }

    int opt = 1;
    // an integer flag used to configure a socket option.
    // Allow this socket to reuse an address (port), even if it was recently
    // used. without this if we run server, stop and try to restart we'll get
    // bind() failed: address alreayd in use
    if (setsockopt(server_fd.getFd(), SOL_SOCKET, SO_REUSEADDR, &opt,
                   sizeof(opt)) < 0) {
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
    // use system defined minimum for backlog, the maximum number of
    // connections that can be waiting in the accept queue.
    if (listen(server_fd.getFd(), SOMAXCONN) < 0) {
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

/**
 * @brief Enables non-blocking mode on a file descriptor.
 *
 * Configures the given file descriptor to operate in non-blocking mode,
 * ensuring that I/O calls do not block the server event loop.
 *
 * This is required for event-driven architecture where read/write operations
 * must return immediately if no data is available.
 *
 * @param fd File descriptor to configure
 *
 * @warning This is essential for correct behavior in a poll/epoll-based
 *          server. Blocking descriptors would stall the entire event loop.
 */
void Server::setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);

    if (flags == -1) {
        serverError("fcntl(F_GETFL) failed for fd " + toString(fd));
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        serverError("fcntl(F_SETFL) failed for " + toString(fd));
    }
}

/**
 * @brief Accepts a new incoming client connection.
 *
 * Accepts a connection on the listening socket and prepares the resulting
 * client socket for non-blocking I/O.
 *
 * The accepted socket is configured to:
 * - Operate in non-blocking mode for event-driven handling
 * - Be managed by the Server after acceptance
 *
 * @return File descriptor of the accepted client socket,
 *         or -1 if no connection is available or an error occurs.
 *
 * @note Currently uses a single listening socket (sockets_[0]).
 *       Multi-socket support will be added with poll/epoll in a later phase.
 *
 * @warning Ownership of the returned file descriptor is transferred to the
 *          caller (Server), and must be tracked for proper cleanup.
 */
int Server::acceptClient() {
    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Single-socket, single-threaded setup for now: we only listen on
    // sockets_[0]. This will be extended later (e.g. with poll/epoll) to handle
    // multiple listening sockets and concurrent client readiness.
    Fd client_fd(
        accept(sockets_[0], (struct sockaddr*)&client_addr, &client_len));

    if (!client_fd.valid()) {
        // accept failed or no connection ready
        // ignore for now (poll will handle this properly later)
        return -1;
    }

    try {
        // Setting the client socket to non-blocking ensures that future recv()
        // and send() calls never stall the entire server if no data is ready or
        // the kernel buffer is full.
        setNonBlocking(client_fd.getFd());
        LOG_DEBUG() << "Setting client fd " << client_fd.getFd()
                    << " to non-blocking";
    } catch (const std::exception& e) {
        LOG_ERROR() << "Failed to set client fd " << client_fd.getFd()
                    << " to non-blocking";
        return -1;
    }

    LOG_INFO() << "Client " << client_fd.getFd() << " connected";
    // transfer ownership of fd here, since at the end of this function
    // Fd will go out of scope and it will close the fd, an we dont want that
    // we need it for later.
    return client_fd.release();
}

/**
 * @brief Handles incoming data for a connected client.
 *
 * Delegates the actual socket reading to the Client object and reacts
 * to its read state.
 *
 * When a complete HTTP request is received, the server transitions
 * from the reading phase to response generation.
 *
 * At this stage, the request is forwarded to the HTTP layer for parsing
 * and response construction.
 *
 * @note Currently supports only single-request processing per client.
 *       Persistent connections and request pipelining will be handled later.
 *
 * @warning CGI handling is planned but not yet implemented.
 */
void Server::handleRead(Client& client) {
    // this is where Charlie's part comes in - see notes in Client::read()
    Client::ReadResult result = client.read();

    if (result == Client::kReadComplete) {
        // later: if (isCGI(client.getRequest())) { dispatchCGI(client); return;
        // }
        client.getResponse().buildFrom(client.getRequest());
    }
}

/**
 * @brief Handles outgoing data for a connected client.
 *
 * Delegates response transmission to the Client object, which manages
 * partial writes and internal buffering.
 *
 * The server reacts to the Client's write state, but does not directly
 * manage buffering or send progress.
 *
 * @note Partial writes are expected in a non-blocking environment.
 *       The Client tracks progress across multiple calls.
 *
 * @note Return value is currently unused but will be required once
 *       poll/epoll integration is introduced to react to write readiness.
 */
void Server::handleWrite(Client& client) {
    Client::WriteResult result = client.write();
    // kWriteDone and kWriteError already set state_ = kDone inside Client
    // kWritePending: loop continues, try again next iteration
    (void)result;  // poll version will use this return value properly
}

/**
 * @brief Handles fatal server errors.
 *
 * Logs the error message and terminates the current operation by throwing
 * a runtime exception.
 *
 * This function is used for unrecoverable errors during server setup or
 * configuration, where continuing execution would leave the server in an
 * invalid state.
 *
 * @param msg Error description
 *
 * @throws std::runtime_error Always thrown with the formatted error message
 *
 * @warning This function does not attempt recovery. It is intended only for
 *          critical failures (e.g. socket, bind, listen errors).
 */
void Server::serverError(const std::string& msg) {
    std::string full = "Server: " + msg;
    LOG_ERROR() << full;
    throw std::runtime_error(full);
}
