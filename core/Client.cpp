#include "Client.hpp"

#include <sys/socket.h>

#include "../handlers/Handler.hpp"
#include "../handlers/Router.hpp"
#include "../logger/Logger.hpp"
#include "../utils/LogUtils.hpp"
#include "CgiProcess.hpp"

/**
 * @brief Converts client state enum to string (logging only).
 */
static const char* stateToStr(Client::State s) {
    switch (s) {
        case Client::kReading:
            return "kReading";
        case Client::kWaitingCgi:
            return "kWaitingCgi";
        case Client::kWriting:
            return "kWriting";
        case Client::kDone:
            return "kDone";
        default:
            return "unknown";
    }
}

/**
 * @brief Initializes client session and request limits.
 */
Client::Client(int fd, EventLoop& loop, ServerResources& resources,
               const std::string& peer_ip)
    : fd_(fd),
      loop_(loop),
      resources_(resources),
      bytes_sent_(0),
      state_(kReading),
      keep_alive_(true),
      timeout_(TimeoutSeconds::kClient),
      pending_cgi_(NULL),
      peer_ip_(peer_ip) {
    request_.setMaxBodySize(resources_.getServerConfig().getMaxBodySize());
}

/**
 * @brief Destroys the Client.
 * Releases owned resources (socket managed by Fd).
 */
Client::~Client() {
    if (pending_cgi_) {
        loop_.removeHandler(pending_cgi_);
        pending_cgi_ = NULL;
    }
}

/**
 * @brief Returns the client socket file descriptor.
 * @return The underlying socket fd
 */
int Client::getFd() const {
    return fd_.getFd();
}

/**
 * @brief Processes poll events for the client connection.
 *
 * Handles socket errors, reads requests, dispatches completed requests,
 * manages CGI execution state, writes responses, and performs cleanup on
 * exceptions. Switches between kReading, kWaitingCgi, and kWriting states
 * based on request and response progress.
 */
void Client::handle(short revents) {
    try {
        LOG_DEBUG() << BR_YEL "[Client] ENTER handle fd=" << fd_.getFd()
                    << " state=" << stateToStr(state_)
                    << " events=" << LogUtils::pollToStr(revents) << RESET;
        if (revents & (POLLERR | POLLNVAL)) {
            return closeConnection("socket error/hangup", "WARNING");
        }
        bool peer_closed = false;
        if (revents & POLLHUP) {
            LOG_INFO() << BR_CYN "[Client] POLLHUP fd=" << fd_.getFd() << RESET;
            peer_closed = true;
        }
        if (revents & POLLIN && state_ == kReading) {
            LOG_DEBUG() << BR_YEL "[Client] POLLIN detected" RESET;
            read();
        }
        if (state_ == kReading && request_.isComplete()) {
            keep_alive_ = request_.shouldKeepAlive();
            LOG_INFO() << BR_CYN "[Client] request complete fd=" << fd_.getFd()
                       << " switching " << stateToStr(state_) << " -> kWriting"
                       << RESET;

            const LocationConfig fallback;
            const LocationConfig& loc =
                request_.isError()
                    ? fallback
                    : resources_.getRouter().resolve(request_.getPath());

            bool response_ready = Handler::run(request_, loc, *this);
            if (response_ready) {
                if (!keep_alive_) {
                    response_.setHeader("Connection", "close");
                }
                state_ = kWriting;
                LOG_DEBUG() << BR_YEL "[Client] enabling POLLOUT" RESET;
                loop_.modifyHandler(this, POLLOUT);
            } else {
                state_ = kWaitingCgi;
                loop_.modifyHandler(this, 0);
            }
        }
        if (revents & POLLOUT && state_ == kWriting) {
            LOG_DEBUG() << BR_YEL "[Client] write triggered" RESET;
            write();
        }
        if (peer_closed && state_ == kReading) {
            return closeConnection("peer disconnected during read");
        }
    } catch (const std::exception& e) {
        LOG_ERROR() << "[Client] exception: " << e.what();
        if (state_ == kReading) {
            receiveError(HttpConstants::kInternalServerError);
        } else {
            cleanup();
        }
    } catch (...) {
        LOG_ERROR() << "[Client] Internal Server Error";
        if (state_ == kReading) {
            receiveError(HttpConstants::kInternalServerError);
        } else {
            cleanup();
        }
    }
}

/**
 * @brief Reads incoming data from the client socket.
 * Appends received bytes to the request and resets the timeout on
 * successful reads. Closes the connection on disconnect or recv() failure.
 */
void Client::read() {
    char buffer[kBufferSize];

    LOG_DEBUG() << BR_YEL "[Client] read() fd=" << fd_.getFd() << RESET;
    ssize_t n = recv(fd_.getFd(), buffer, kBufferSize, 0);
    if (n == 0) {
        return closeConnection("client closed connection");
    }
    if (n < 0) {
        return closeConnection("recv error fd=", "ERROR");
    }
    timeout_.reset();
    LOG_DEBUG() << BR_YEL "[Client] read bytes=" << n << RESET;
    request_.append(buffer, n);
}

/**
 * @brief Writes response data to the client socket.
 *
 * Handles partial sends and tracks transmission progress. Once the
 * response is fully sent, either closes the connection or resets the
 * client for the next request on keep-alive connections.
 *
 * If pipelined request data is already available, resolves the target
 * location and immediately dispatches the next request. Requests that
 * cannot be resolved use a fallback location, while CGI requests switch
 * the client into the waiting state until the CGI response is ready.
 */
void Client::write() {
    const std::string& data = response_.getRaw();
    LOG_DEBUG() << BR_YEL "[Client] write() fd=" << fd_.getFd()
                << " sent=" << bytes_sent_ << "/" << data.size() << RESET;

    ssize_t n = send(fd_.getFd(), data.c_str() + bytes_sent_,
                     data.size() - bytes_sent_, 0);
    if (n <= 0) {
        return closeConnection("send error fd=", "ERROR");
    }
    timeout_.reset();
    bytes_sent_ += n;
    LOG_DEBUG() << BR_YEL "[Client] wrote bytes=" << n
                << " total=" << bytes_sent_ << RESET;

    if (bytes_sent_ >= data.size()) {
        LOG_INFO() << BR_CYN "[Client] response complete fd=" << fd_.getFd()
                   << RESET;
        if (!keep_alive_) {
            return closeConnection("closing connection");
        }
        LOG_INFO() << BR_CYN "[Client] keeping connection alive fd="
                   << fd_.getFd() << RESET;
        state_ = kReading;
        bytes_sent_ = 0;
        request_.resetData();
        response_.reset();
        if (request_.isComplete()) {
            keep_alive_ = request_.shouldKeepAlive();

            const LocationConfig fallback;
            const LocationConfig& loc =
                request_.isError()
                    ? fallback
                    : resources_.getRouter().resolve(request_.getPath());

            bool response_ready = Handler::run(request_, loc, *this);
            if (response_ready) {
                if (!keep_alive_) {
                    response_.setHeader("Connection", "close");
                }
                state_ = kWriting;
                loop_.modifyHandler(this, POLLOUT);
            } else {
                state_ = kWaitingCgi;
                loop_.modifyHandler(this, 0);
            }
        } else {
            loop_.modifyHandler(this, POLLIN);
        }
    }
}

/**
 * @brief Marks the client as finished.
 *
 * Transitions the client to the kDone state so it can be removed
 * during event loop cleanup.
 */
void Client::cleanup() {
    LOG_INFO() << BR_CYN "[Client] fd=" << fd_.getFd() << " switching "
               << stateToStr(state_) << " -> kDone" << RESET;
    state_ = kDone;
}

/**
 * @brief Checks whether the client has finished processing.
 * @return true if the client is in the kDone state.
 */
bool Client::isDone() const {
    return state_ == kDone;
}

/**
 * @brief Returns the handler name used for logging.
 * @return The string "Client".
 */
const char* Client::name() const {
    return "Client";
}

/**
 * @brief Logs the reason for connection termination and closes it.
 * The log level determines whether the message is emitted as an
 * informational, warning, or error log before the client is marked
 * for cleanup.
 *
 * @param reason Reason the connection is being closed.
 * @param level Logging level to use.
 */
void Client::closeConnection(const std::string& reason, const char* level) {
    if (std::string(level) == "WARNING") {
        LOG_WARNING() << "[Client] " << reason << " fd=" << fd_.getFd();
    } else if (std::string(level) == "ERROR") {
        LOG_ERROR() << "[Client] " << reason << " fd=" << fd_.getFd();
    } else {
        LOG_INFO() << BR_CYN "[Client] " << reason << " fd=" << fd_.getFd()
                   << RESET;
    }
    cleanup();
}

/**
 * @brief Checks whether the client has exceeded its timeout.
 * @return true if the timeout has expired and the client is not already
 *         in the kDone state.
 */
bool Client::isTimedOut() const {
    if (state_ == kDone) {
        return false;
    }
    return timeout_.expired();
}

/**
 * @brief Returns the event loop managing this client.
 * @return Reference to the event loop.
 */
EventLoop& Client::getLoop() {
    return loop_;
}

/**
 * @brief Returns the response associated with this client.
 * @return Reference to the response object.
 */
Response& Client::getResponse() {
    return response_;
}

/**
 * @brief Returns shared server resources.
 * @return Const reference to the server resources.
 */
const ServerResources& Client::getResources() const {
    return resources_;
}

/**
 * @brief Returns shared server resources.
 * @return Reference to the server resources.
 */
ServerResources& Client::getResources() {
    return resources_;
}

/**
 * @brief Returns the active server configuration.
 * @return Const reference to the server configuration.
 */
const ServerConfig& Client::getServerConfig() const {
    return resources_.getServerConfig();
}

/**
 * @brief Returns the router associated with this server.
 * @return Const reference to the router.
 */
const Router& Client::getRouter() const {
    return resources_.getRouter();
}

/**
 * @brief Returns the client's IP address.
 * @return Client IP address as a string.
 */
std::string Client::getPeerIp() const {
    return peer_ip_;
}

/**
 * @brief Builds an error response and schedules it for transmission.
 * Disables connection reuse, prepares the error response, and switches
 * the client to write mode.
 *
 * @param error HTTP error to send to the client.
 */
void Client::receiveError(HttpConstants::HttpError error) {
    pending_cgi_ = NULL;
    keep_alive_ = false;
    response_.buildError(error);
    response_.setHeader("Connection", "close");
    bytes_sent_ = 0;
    state_ = kWriting;
    loop_.modifyHandler(this, POLLOUT);
}

/**
 * @brief Registers the active CGI process for this client.
 * @param cgi Pointer to the CGI process associated with the request.
 */
void Client::setPendingCgi(CgiProcess* cgi) {
    pending_cgi_ = cgi;
}

/**
 * @brief Handles completion of an asynchronous CGI request.
 * Parses the CGI output into an HTTP response and schedules the
 * connection for writing. Sends a 500 error if the CGI output is
 * empty or cannot be parsed.
 * @param raw_cgi_output Raw output returned by the CGI process.
 */
void Client::onCgiFinished(const std::string& raw_cgi_output) {
    pending_cgi_ = NULL;
    timeout_.reset();

    if (raw_cgi_output.empty()) {
        receiveError(HttpConstants::kInternalServerError);
        return;
    }
    try {
        response_.reset();
        Handler::applyCgiResponse(raw_cgi_output, response_);

        if (!keep_alive_) {
            response_.setHeader("Connection", "close");
        }
        bytes_sent_ = 0;
        state_ = kWriting;

        loop_.modifyHandler(this, POLLOUT);
    } catch (const std::exception& e) {
        LOG_ERROR() << "[Client] failed to apply CGI response: " << e.what();
        receiveError(HttpConstants::kInternalServerError);
    }
}

/**
 * @brief Handles client timeout events.
 * Marks the connection for cleanup.
 */
void Client::onTimeout() {
    cleanup();
}
