#include "Fd.hpp"

#include <unistd.h>

#include "../logger/Logger.hpp"

/**
 * @brief Constructs an Fd wrapper.
 *
 * Takes ownership of the given file descriptor.
 *
 * @param[in] fd File descriptor to manage (-1 means invalid)
 */
Fd::Fd(int fd) : fd_(fd) {
    // LOG_DEBUG() << "Fd created: " << fd_;
}

/**
 * @brief Destroys the Fd and closes the descriptor if valid.
 *
 * Ensures the owned file descriptor is closed on destruction.
 */
Fd::~Fd() {
    if (fd_ >= 0) {
        //  LOG_DEBUG() << "Fd closed: " << fd_;
        close(fd_);
    }
}

/**
 * @brief Returns the underlying file descriptor.
 *
 * @return The managed file descriptor value
 */
int Fd::getFd() const {
    return fd_;
}

/**
 * @brief Replaces the managed file descriptor.
 *
 * Closes the current descriptor if valid and different, then
 * takes ownership of the new one.
 *
 * @param[in] new_fd New file descriptor to manage (-1 means invalid)
 */
void Fd::reset(int new_fd) {
    if (fd_ != new_fd) {
        if (fd_ >= 0) {
            //          LOG_DEBUG() << "Fd reset: closing fd " << fd_;
            close(fd_);
        }
        //        LOG_DEBUG() << "Fd now holds: fd " << new_fd;
        fd_ = new_fd;
    }
}
/**
 * @brief Releases ownership of the file descriptor.
 *
 * After calling this, the Fd no longer manages the descriptor.
 *
 * @return The previously managed file descriptor
 */
int Fd::release() {
    int tmp = fd_;
    fd_ = -1;
    //    LOG_DEBUG() << "Fd released: " << tmp;
    return tmp;
}

/**
 * @brief Checks if the file descriptor is valid.
 *
 * @return true if fd >= 0, false otherwise
 */
bool Fd::valid() const {
    return fd_ >= 0;
}
