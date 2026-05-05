#include "Client.hpp"
#include <sys/socket.h>
#include <cerrno>
#include <string>
#include "../logger/Logger.hpp"
#include "../utils/LogUtils.hpp"

// helper for consistent logging
static const char* stateToStr(Client::State s) {
	switch (s) {
		case Client::kReading: return "kReading";
		case Client::kWriting: return "kWriting";
		case Client::kDone:    return "kDone";
		default:               return "unknown";
	}
}

Client::Client(int fd, EventLoop& loop) :
	fd_(fd),
	loop_(loop),
	bytes_sent_(0),
	state_(kReading),
	keep_alive_(true) 
{}

/**
 * @brief Destroys the Client.
 *
 * Releases owned resources (socket managed by Fd).
 */
Client::~Client() {
}

/**
 * @brief Returns the client socket file descriptor.
 *
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
		// conceptually since poll events are bit masks..
		// does revents contain the POLLIN bit? if yes the result is non zero
		if (revents & POLLIN && state_ == kReading) {
			LOG_DEBUG() << "[Client] POLLIN detected";
			read();
		}

		if (state_ == kReading && request_.isComplete()) {
			keep_alive_ = request_.shouldKeepAlive();

			LOG_INFO() << "[Client] request complete fd=" << fd_.getFd()
			           << " switching " << stateToStr(state_)
					   << " → kWriting";

			response_.buildFrom(request_);
			state_ = kWriting;

			LOG_DEBUG() << "[Client] enabling POLLOUT";
			loop_.modifyHandler(this, POLLOUT);
		}
		//if revents contain POLLOUT
		if (revents & POLLOUT && state_ == kWriting) {
			LOG_DEBUG() << "[Client] write triggered";
			write();
		}
	}
	catch (...) {
		cleanup();
	}
}

void Client::read() {
	char buffer[kBufferSize];

	LOG_DEBUG() << "[Client] read() fd=" << fd_.getFd();
	ssize_t n = recv(fd_.getFd(), buffer, kBufferSize, 0);
	//client disconnected cleanly
	if (n == 0) {
		LOG_INFO() << "[Client] client closed connection fd=" << fd_.getFd();
		cleanup();
		return ;
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
	            << " sent=" << bytes_sent_
	            << "/" << data.size();

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
		return;
	}

	bytes_sent_ += n;
	LOG_DEBUG() << "[Client] wrote bytes=" << n
	            << " total=" << bytes_sent_;

	if (bytes_sent_ >= data.size()) {
		LOG_INFO() << "[Client] response complete fd=" << fd_.getFd();
		if (!keep_alive_) {
			LOG_INFO() << "[Client] closing connection fd=" << fd_.getFd();
			cleanup();
			return ;
		}
		LOG_INFO() << "[Client] keeping connection alive fd=" << fd_.getFd();
		//reset for next request
		state_ = kReading;
		bytes_sent_ = 0;
		request_.reset();
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

