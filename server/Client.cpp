#include "Client.hpp"
#include "../logger/Logger.hpp"

Client::Client(int fd) : fd_(fd), bytes_sent_(0), state_(kReading)
{
	LOG_DEBUG() << "fd: " << fd_.getFd() << ", bytes sent: " << bytes_sent_
				<< ", state: " << state_;
	// request_ object default constructed automatically
	// response_ object also default constructed automatically
}

Client::~Client() {}

// getter
int	Client::getFd() const {
	return fd_.getFd();
}

Client::State Client::getState() const {
	return state_;
}

Request& Client::getRequest() {
	return request_;
}

Response& Client::getResponse() {
	return response_;
}

// the client request comes in as a stream of raw data, that can come
// in partial chunks or split messages - these raw data must be parsed
// and reconstructed at the HTTP layer (Charlie's part). My job
// here is just to store the raw data
Client::ReadResult Client::read() {
	char buffer[kBufferSize];
	ssize_t bytes = recv(fd_.getFd(), buffer, sizeof(buffer), 0);
	LOG_DEBUG() << "Received " << bytes << " bytes from fd " << fd_.getFd();

	if (bytes > 0) {
		// i store the request directly in request object raw_ for Charlie
		request_.append(buffer, bytes);
		// charlie then parses the header and body and check to see if the request
		// isComplete when "\r\n\r\n" is found (+ full body if Content-Length is set)
		if (request_.isComplete()) {
			state_ = kWriting;
			LOG_DEBUG() << "Client fd " << fd_.getFd()
						<< " switching to WRITING state";
			return kReadComplete;
		}
		return kReadPending;
	} else if (bytes == 0) { // connection finished, stream closed
		state_ = kDone;
		return kReadClosed; // clean disconnect, not error
	} else {
		state_ = kDone;
		LOG_INFO() << "Client fd " << fd_.getFd() << " disconnected or error";
		return kReadError;
	}
}

Client::WriteResult Client::write() {
	const std::string response = response_.getRaw(); // copy safe for CGI later
													 //
	if (bytes_sent_ >= response.size()) {
		state_ = kDone;
		return kWriteDone;
	}
	// send() may write only part of the data, so we resume from bytes already sent
	ssize_t sent = send(fd_.getFd(),
			response.c_str() + bytes_sent_,
			response.size() - bytes_sent_, 0);
	if (sent > 0) {
		// track how many bytes have been successfully sent so far
		bytes_sent_ += static_cast<size_t>(sent);
		// if entire response has been sent, mark client as done
		if (bytes_sent_ >= response.size()) {
			state_ = kDone;
			return kWriteDone;
		}
		return kWritePending;
	} else {
		// w/o polling, treat any failure as a closed connection
		state_ = kDone;
		return kWriteError;
	}
}
