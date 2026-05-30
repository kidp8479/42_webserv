#include "CgiStdinWriter.hpp"
#include "../logger/Logger.hpp"
//#include <cerrno>
#include <string.h>

CgiStdinWriter::CgiStdinWriter(int write_fd,
		const std::string& body, EventLoop& loop) :
	write_fd_(write_fd),
	body_(body),
	bytes_written_(0),
	loop_(loop),
	done_(false) {
}

CgiStdinWriter::~CgiStdinWriter() {}

int CgiStdinWriter::getFd() const {
	return write_fd_.getFd();
}

void CgiStdinWriter::handle(short revents) {
	LOG_DEBUG() << "[CGI] stdin writer triggered";
	if (!(revents & POLLOUT)) {
		return;
	}
	LOG_DEBUG() << "[CGI] body size = " << body_.size();
	ssize_t n = write(write_fd_.getFd(),
			body_.c_str() + bytes_written_,
			body_.size() - bytes_written_);
	LOG_DEBUG() << "[CGI] wrote " << n << " bytes";
	if (n < 0) {
	    LOG_ERROR() << "[CGI] write failed errno="
                << errno
                << " (" << strerror(errno) << ")";
		done_ = true; // child died pipe broken
		return;
	}
	bytes_written_ += n;

	if (bytes_written_ >= body_.size()) {
		// all body written, close write end so child gets EOF on stdin
		write_fd_.reset();
		done_ = true;
	}
}

bool CgiStdinWriter::isDone() const {
	return done_;
}

bool CgiStdinWriter::isTimedOut() const {
	return false; // time out handled by CgiProcess
}

const char* CgiStdinWriter::name() const {
	return "CgiStdinWriter";
}
