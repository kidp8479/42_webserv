#include "Client.hpp"
#include "../logger/Logger.hpp"

Client::Client(int fd) : fd_(fd), bytes_sent_(0), state_(kReading)
{
	LOG_DEBUG() << "fd: " << fd_.getFd() << ", bytes sent: " << bytes_sent_
				<< ", state: " << state_;
}

Client::~Client() {}

// getter
int	Client::getFd() const {
	return fd_.getFd();
}

size_t	Client::getBytesSent() const {
	return bytes_sent_;
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

//setter
void Client::setState(State new_state) {
	state_ = new_state;
}

void	Client::addBytesSent(size_t n) {
	bytes_sent_ += n;
}
