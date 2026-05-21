#include "Client.hpp"

#include <sys/socket.h>

#include <string>

#include "../handlers/Handler.hpp"
#include "../handlers/Router.hpp"
#include "../logger/Logger.hpp"
#include "../utils/LogUtils.hpp"
#include "Timeout.hpp"

// helper for consistent logging
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

Client::Client(int fd, EventLoop& loop, const ServerResources& resources)
    : fd_(fd),
      loop_(loop),
      resources_(resources),
      bytes_sent_(0),
      state_(kReading),
      keep_alive_(true),
	  timeout_(TimeoutSeconds::kClient) {
    request_.setMaxBodySize(resources_.serverConfig().getMaxBodySize());
}

/**
 * @brief Destroys the Client.
 * Releases owned resources (socket managed by Fd).
 */
Client::~Client() {
}

/**
 * @brief Returns the client socket file descriptor.
 * @return The underlying socket fd
 */
int Client::getFd() const {
    return fd_.getFd();
}

void Client::handle(short revents) {
    try {
        LOG_DEBUG() << "[Client] ENTER handle fd=" << fd_.getFd()
                    << " state=" << stateToStr(state_)
                    << " events=" << LogUtils::pollToStr(revents);
        if (revents & (POLLERR | POLLNVAL)) {
            return closeConnection("socket error/hangup", "WARNING");
        }
        bool peer_closed = false;
        if (revents & POLLHUP) {
            LOG_INFO() << "[Client] POLLHUP fd=" << fd_.getFd();
            peer_closed = true;
        }
        // read available data first
        if (revents & POLLIN && state_ == kReading) {
            LOG_DEBUG() << "[Client] POLLIN detected";
            read();
        }
        // request finished parsing
        if (state_ == kReading && request_.isComplete()) {
            keep_alive_ = request_.shouldKeepAlive();
            LOG_INFO() << "[Client] request complete fd=" << fd_.getFd()
                       << " switching " << stateToStr(state_) << " → kWriting";
			
			const LocationConfig fallback;
            const LocationConfig& loc = request_.isError()
				? fallback
				: resources_.getRouter().resolve(request_.getPath());

            bool response_ready = Handler::run(request_, loc, *this);
            if (response_ready) {
				state_ = kWriting;
				LOG_DEBUG() << "[Client] enabling POLLOUT";
				loop_.modifyHandler(this, POLLOUT);
			} else {
				state_ = kWaitingCgi;
				loop_.modifyHandler(this, 0);
			}
		}
        // write response
        if (revents & POLLOUT && state_ == kWriting) {
            LOG_DEBUG() << "[Client] write triggered";
            write();
        }
        // if peer closed and we're still reading, we cant receive more bytes
        // anymore so if request is incomplete its a dead connection
        if (peer_closed && state_ == kReading) {
            return closeConnection("peer disconnected during read");
        }
    } catch (const std::exception& e) {

        LOG_ERROR() << "[Client] exception: " << e.what();
        if (state_ == kReading) {
			// Response is built manually here because this catch is outside Handler::run()
			// if Router::resolve() throws, no handler context exists, so we can't use
			// Handler::sendError(). last resort for misconfigured servers
			// missing a catch-all '/' location block.
			response_.setRaw(
				"HTTP/1.1 500 Internal Server Error\r\n"
				"Content-Length: 0\r\n\r\n"
				);
			keep_alive_ = false;
			bytes_sent_ = 0;
			state_ = kWriting;
			loop_.modifyHandler(this, POLLOUT);
		} else {
			cleanup();
		}
    } catch (...) {  // universal fall back
        cleanup();
    }
}

void Client::read() {
    char buffer[kBufferSize];

    LOG_DEBUG() << "[Client] read() fd=" << fd_.getFd();
    ssize_t n = recv(fd_.getFd(), buffer, kBufferSize, 0);
    // client disconnected cleanly
    if (n == 0) {
        return closeConnection("client closed connection");
    }
    // poll said ready, so failure here is a real error (no errno checks!)
    if (n < 0) {
        return closeConnection("recv error fd=", "ERROR");
    }
	timeout_.reset();
    LOG_DEBUG() << "[Client] read bytes=" << n;
    request_.append(buffer, n);
}

void Client::write() {
    const std::string& data = response_.getRaw();
    LOG_DEBUG() << "[Client] write() fd=" << fd_.getFd()
                << " sent=" << bytes_sent_ << "/" << data.size();

    ssize_t n = send(fd_.getFd(), data.c_str() + bytes_sent_,
                     data.size() - bytes_sent_, 0);
    if (n <= 0) {
        return closeConnection("send error fd=", "ERROR");
    }
	// reset on successful write
	timeout_.reset();
    bytes_sent_ += n;
    LOG_DEBUG() << "[Client] wrote bytes=" << n << " total=" << bytes_sent_;

    if (bytes_sent_ >= data.size()) {
        LOG_INFO() << "[Client] response complete fd=" << fd_.getFd();
        if (!keep_alive_) {
            return closeConnection("closing connection");
        }
        LOG_INFO() << "[Client] keeping connection alive fd=" << fd_.getFd();
        // reset for next request
        state_ = kReading;
        bytes_sent_ = 0;
        request_.resetData();  // keeps raw_, re parses any pipelined data
        response_.reset();
        // if raw_ had any leftover bytes from pipelined request
        if (request_.isComplete()) {
			keep_alive_ = request_.shouldKeepAlive();

            const LocationConfig fallback;
			const LocationConfig& loc = request_.isError()
				? fallback
				: resources_.getRouter().resolve(request_.getPath());

			Handler::run(request_, loc, *this);
            state_ = kWriting;
            loop_.modifyHandler(this, POLLOUT);
            return;
        }
        // switch back to read mode
        loop_.modifyHandler(this, POLLIN);
    }
}

void Client::cleanup() {
    LOG_INFO() << "[Client] fd=" << fd_.getFd() << " switching "
               << stateToStr(state_) << " → kDone";
    state_ = kDone;
}

bool Client::isDone() const {
    return state_ == kDone;
}

const char* Client::name() const {
    return "Client";
}

void Client::closeConnection(const std::string& reason, const char* level) {
    if (std::string(level) == "WARNING") {
        LOG_WARNING() << "[Client] " << reason << " fd=" << fd_.getFd();
    } else if (std::string(level) == "ERROR") {
        LOG_ERROR() << "[Client] " << reason << " fd=" << fd_.getFd();
    } else {
        LOG_INFO() << "[Client] " << reason << " fd=" << fd_.getFd();
    }
    cleanup();
}

bool Client::isTimedOut() const {
	if (state_ == kDone) {
		return false;
	}
	return timeout_.expired();
}

EventLoop& Client::getLoop() {
	return loop_;
}

Response& Client::getResponse() {
	return response_;
}

const ServerResources& Client::getResources() {
	return resources_;
}

void Client::receiveResponse(const std::string& raw) {
	response_.setRaw(raw);
	bytes_sent_ = 0;
	state_ = kWriting;
	loop_.modifyHandler(this, POLLOUT);
}

void Client::receiveError(const std::string& raw) {
	keep_alive_ = false; // dont reuse connection after cgi failure
	receiveResponse(raw);
}
