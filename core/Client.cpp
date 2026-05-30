#include "Client.hpp"

#include <sys/socket.h>

#include <string>

#include "../handlers/Handler.hpp"
#include "../handlers/Router.hpp"
#include "../logger/Logger.hpp"
#include "../utils/LogUtils.hpp"
#include "Timeout.hpp"
#include "CgiProcess.hpp"

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
	  timeout_(TimeoutSeconds::kClient),
	  pending_cgi_(NULL) {
    request_.setMaxBodySize(resources_.getServerConfig().getMaxBodySize());
}

/**
 * @brief Destroys the Client.
 * Releases owned resources (socket managed by Fd).
 */
Client::~Client() {
	if (pending_cgi_) {
		loop_.removeHandler(pending_cgi_); // eventloop deletes it
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

void Client::handle(short revents) {
    try {
        LOG_DEBUG() << BR_YEL "[Client] ENTER handle fd=" << fd_.getFd()
                    << " state=" << stateToStr(state_)
                    << " events=" << LogUtils::pollToStr(revents) << RESET;
        // handle failures and disconnects (fatal socket states)
        if (revents & (POLLERR | POLLNVAL)) {
            return closeConnection("socket error/hangup", "WARNING");
        }
        bool peer_closed = false;
        if (revents & POLLHUP) {
            LOG_INFO() << BR_CYN "[Client] POLLHUP fd=" << fd_.getFd() << RESET;
            peer_closed = true;
        }
        // read available data first
        if (revents & POLLIN && state_ == kReading) {
            LOG_DEBUG() << BR_YEL "[Client] POLLIN detected" RESET;
            read();
        }
        // request finished parsing
        if (state_ == kReading && request_.isComplete()) {
            keep_alive_ = request_.shouldKeepAlive();
            LOG_INFO() << BR_CYN "[Client] request complete fd=" << fd_.getFd()
                       << " switching " << stateToStr(state_) << " -> kWriting"
                       << RESET;

            const LocationConfig fallback;
            const LocationConfig& loc = request_.isError()
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
        // write response
        if (revents & POLLOUT && state_ == kWriting) {
            LOG_DEBUG() << BR_YEL "[Client] write triggered" RESET;
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
			receiveError(HttpConstants::kInternalServerError);
		} else {
			cleanup();
		}
    } catch (...) {  // universal fall back
		LOG_ERROR() << "[Client] Internal Server Error";
		if (state_ == kReading) {
			receiveError(HttpConstants::kInternalServerError);
		} else {
			cleanup();
		}
	}
}

void Client::read() {
    char buffer[kBufferSize];

    LOG_DEBUG() << BR_YEL "[Client] read() fd=" << fd_.getFd() << RESET;
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
    LOG_DEBUG() << BR_YEL "[Client] write() fd=" << fd_.getFd()
                << " sent=" << bytes_sent_ << "/" << data.size() << RESET;

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
        LOG_INFO() << BR_CYN "[Client] response complete fd=" << fd_.getFd()
                   << RESET;
        if (!keep_alive_) {
            return closeConnection("closing connection");
        }
        LOG_INFO() << BR_CYN "[Client] keeping connection alive fd="
                   << fd_.getFd() << RESET;
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

			bool response_ready = Handler::run(request_, loc, *this);
			if (response_ready) {
				if (!keep_alive_) {
					response_.setHeader("Connection", "close");
				}
			}
            state_ = kWriting;
            loop_.modifyHandler(this, POLLOUT);
            return;
        }
        // switch back to read mode
        loop_.modifyHandler(this, POLLIN);
    }
}

void Client::cleanup() {
    LOG_INFO() << BR_CYN "[Client] fd=" << fd_.getFd() << " switching "
               << stateToStr(state_) << " -> kDone" << RESET;
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
        LOG_INFO() << BR_CYN "[Client] " << reason << " fd=" << fd_.getFd()
                   << RESET;
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

const ServerResources& Client::getResources() const {
	return resources_;
}

const ServerConfig& Client::getServerConfig() const {
	return resources_.getServerConfig();
}

const Router&Client::getRouter() const {
	return resources_.getRouter();
}

void Client::receiveError(HttpConstants::HttpError error) {
	keep_alive_ = false; // dont reuse connection after cgi failure
	response_.buildError(error);
	response_.setHeader("Connection", "close");
	bytes_sent_ = 0;
	state_ = kWriting;
	loop_.modifyHandler(this, POLLOUT);
}

void Client::setPendingCgi(CgiProcess* cgi) {
	pending_cgi_ = cgi;
}

void Client::onCgiFinished(const std::string& raw_cgi_output) {
	pending_cgi_ = NULL;
	timeout_.reset();
	
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

