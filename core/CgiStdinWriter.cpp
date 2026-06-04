#include "CgiStdinWriter.hpp"

#include <poll.h>
#include <unistd.h>

#include "../logger/Logger.hpp"

/**
 * @brief Initializes CGI stdin writer for request body streaming.
 */
CgiStdinWriter::CgiStdinWriter(int write_fd, const std::string& body,
                               EventLoop& loop)
    : write_fd_(write_fd),
      body_(body),
      bytes_written_(0),
      loop_(loop),
      done_(false) {
}

/**
 * @brief Cleans up writer resources.
 */
CgiStdinWriter::~CgiStdinWriter() {
}

/**
 * @brief Returns write file descriptor.
 */
int CgiStdinWriter::getFd() const {
    return write_fd_.getFd();
}

/**
 * @brief Writes pending body data to CGI stdin (non-blocking).
 * Handles partial writes, completion, and pipe errors.
 */
void CgiStdinWriter::handle(short revents) {
    LOG_DEBUG() << "[CGI] stdin writer triggered";
    if (!(revents & POLLOUT) || done_) {
        return;
    }
    LOG_DEBUG() << "[CGI] body size = " << body_.size();
    ssize_t n = write(write_fd_.getFd(), body_.c_str() + bytes_written_,
                      body_.size() - bytes_written_);
    LOG_DEBUG() << "[CGI] wrote " << n << " bytes";
    if (n <= 0) {
        LOG_WARNING() << "[CgiStdinWriter] write failed, child likely died";
        done_ = true;
        write_fd_.reset();
        return;
    }
    bytes_written_ += n;
    if (bytes_written_ >= body_.size()) {
        write_fd_.reset();
        done_ = true;
    }
}

/**
 * @brief Returns true if writing is complete or failed.
 */
bool CgiStdinWriter::isDone() const {
    return done_;
}

/**
 * @brief Always false (timeout handled by CGI process).
 */
bool CgiStdinWriter::isTimedOut() const {
    return false;
}

/**
 * @brief Returns handler identifier.
 */
const char* CgiStdinWriter::name() const {
    return "CgiStdinWriter";
}
