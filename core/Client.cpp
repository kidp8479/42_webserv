#include "Client.hpp"
#include <sys/socket.h>
#include <cerrno>
#include <string>
#include "../logger/Logger.hpp"
#include "../utils/LogUtils.hpp"
#include "../handlers/Router.hpp"
#include "../handlers/Handler.hpp"

// helper for consistent logging
static const char* stateToStr(Client::State s) {
	switch (s) {
		case Client::kReading: return "kReading";
		case Client::kWriting: return "kWriting";
		case Client::kDone:    return "kDone";
		default:               return "unknown";
	}
}

Client::Client(int fd, EventLoop& loop, const ServerResources& resources) :
	fd_(fd),
	loop_(loop),
	resources_(resources),
	bytes_sent_(0),
	state_(kReading),
	keep_alive_(true) 
{}

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
		// handle failures and disconnects (fatal socket states)
		if (revents & (POLLERR | POLLNVAL)) {
			return closeConnection("socket error/hangup", "WARNING");
		}
		//POLLHUP means peer closed it's side of the connection so there
		//may still be unread bytes buffered in the kernel so we dont 
		//instantly clean up here.
		bool peer_closed = false;
		if (revents & POLLHUP) {
			LOG_INFO() << "[Client] POLLHUP fd=" << fd_.getFd();
			peer_closed = true;
		}
		// read avaliable data first
		if (revents & POLLIN && state_ == kReading) {
			LOG_DEBUG() << "[Client] POLLIN detected";
			read();
		}
		// request finished parsing
		if (state_ == kReading && request_.isComplete()) {
			// invalid request close connection immediately
			if (request_.isError()) {
				return closeConnection("invalid request", "WARNING");
			}
			keep_alive_ = request_.shouldKeepAlive();
			LOG_INFO() << "[Client] request complete fd=" << fd_.getFd()
			           << " switching " << stateToStr(state_) << " → kWriting";

			const LocationConfig& loc = resources_.router().resolve(request_);
			Handler::run(request_, loc, response_);
			state_ = kWriting;
			LOG_DEBUG() << "[Client] enabling POLLOUT";
			loop_.modifyHandler(this, POLLOUT);
		}
		// write response
		if (revents & POLLOUT && state_ == kWriting) {
			LOG_DEBUG() << "[Client] write triggered";
			write();
		}
		// if peer closed and we're still reading, we cant receive more bytes
		// anymore so if request is incomplete its a dead connection
		if (peer_closed && state_ == kReading) {
			LOG_INFO() << "[Client] peer disconnected during read fd=" << fd_.getFd();
			cleanup();
			return closeConnection("peer disconnected during read");
		}
	}
	catch (const std::exception& e) {
		LOG_ERROR() << "[Client] exception: " << e.what();
		cleanup();
	}
	catch (...) { // universal fall back
		cleanup();
	}
}

void Client::read() {
	char buffer[kBufferSize];

	LOG_DEBUG() << "[Client] read() fd=" << fd_.getFd();
	ssize_t n = recv(fd_.getFd(), buffer, kBufferSize, 0);
	//client disconnected cleanly
	if (n == 0) {
		return closeConnection("client closed connection");
	}

	if (n < 0) {
		//not an error just means no data available rn, try again
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			LOG_DEBUG() << "[Client] read EAGAIN / EWOULDBLOCK";
			return ;
		}
		// real error: connection reset, bad fd, kernel error
		// maybe log debug here
		LOG_ERROR() << "[Client] recv error fd=" << fd_.getFd()
		            << " errno=" << errno;
		cleanup();
		return;
	}
	LOG_DEBUG() << "[Client] read bytes=" << n;
	request_.append(buffer, n);
}

void Client::write() {
	const std::string& data = response_.getRaw();
	LOG_DEBUG() << "[Client] write() fd=" << fd_.getFd()
	            << " sent=" << bytes_sent_ << "/" << data.size();

	ssize_t n =
		send(fd_.getFd(), data.c_str() + bytes_sent_, data.size() - bytes_sent_, 0);

	if (n <= 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			LOG_DEBUG() << "[Client] write EAGAIN / EWOULDBLOCK";
			return ;
		}
		LOG_ERROR() << "[Client] send error fd=" << fd_.getFd()
		            << " errno=" << errno;
		cleanup();
		return ;
	}

	bytes_sent_ += n;
	LOG_DEBUG() << "[Client] wrote bytes=" << n << " total=" << bytes_sent_;

	if (bytes_sent_ >= data.size()) {
		LOG_INFO() << "[Client] response complete fd=" << fd_.getFd();
		if (!keep_alive_) {
			return closeConnection("closing connection");
		}
		LOG_INFO() << "[Client] keeping connection alive fd=" << fd_.getFd();
		//reset for next request
		state_ = kReading;
		bytes_sent_ = 0;
		request_.resetData();
		response_.reset();

		// switch back to read mode
		loop_.modifyHandler(this, POLLIN);
	}
}

void Client::cleanup() {
	LOG_INFO() << "[Client] fd=" << fd_.getFd()
	           << " switching " << stateToStr(state_)
	           << " → kDone";
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
			LOG_ERROR() << "[Client] " << reason << "fd=" << fd_.getFd();
	} else {
		LOG_INFO() << "[Client] " << reason << " fd=" << fd_.getFd();
	}
	cleanup();
}
