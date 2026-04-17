#include "Fd.hpp"
#include "../logger/Logger.hpp"
#include <unistd.h>

Fd::Fd(int fd) : fd_(fd) {
	LOG_DEBUG() << "Fd created: " << fd_;
}

Fd::~Fd() {
	if (fd_ >= 0) {
		LOG_DEBUG() << "Fd closed: " << fd_;
		close(fd_);
	}
}

int	Fd::getFd() const {
	return fd_;
}

void	Fd::reset(int new_fd) {
	if (fd_ != new_fd) {
		if (fd_ >= 0) {
			LOG_DEBUG() << "Fd reset: closing fd " << fd_;
			close(fd_);
		}
		LOG_DEBUG() << "Fd now holds: fd " << fd_;
		fd_ = new_fd;
	}
}

int Fd::release () {
	int tmp = fd_;
	fd_ = -1;
	LOG_DEBUG() << "Fd released: " << tmp;
	return tmp;
}

bool Fd::valid() const {
	return fd_ >= 0;
}
