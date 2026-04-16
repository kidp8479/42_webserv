#include "Fd.hpp"
#include <unistd.h>

Fd::Fd(int fd) : fd_(fd) {}

Fd::~Fd() {
	if (fd_ >= 0) {
		close(fd_);
	}
}

int	Fd::getFd() const {
	return fd_;
}

void	Fd::reset(int new_fd) {
	if (fd_ != new_fd) {
		if (fd_ >= 0) {
			close(fd_);
		}
		fd_ = new_fd;
	}
}

int Fd::release () {
	int tmp = fd_;
	fd_ = -1;
	return tmp;
}

bool Fd::valid() const {
	return fd_ >= 0;
}
