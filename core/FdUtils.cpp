#include "FdUtils.hpp"

#include <fcntl.h>

#include <sstream>
#include <stdexcept>

namespace FdUtils {

/**
 * @brief Configures a file descriptor for event-driven use.
 *
 * Enables non-blocking mode and sets the close-on-exec flag.
 *
 * @param fd File descriptor to configure.
 * @throws std::runtime_error If any fcntl() call fails.
 */
void setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        std::ostringstream oss;
        oss << "[FdUtils] fcntl(F_GETFL) failed for fd " << fd;
        throw std::runtime_error(oss.str());
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        std::ostringstream oss;
        oss << "[FdUtils] fcntl(F_SETFL) failed for fd " << fd;
        throw std::runtime_error(oss.str());
    }
    if (fcntl(fd, F_SETFD, FD_CLOEXEC) == -1) {
        std::ostringstream oss;
        oss << "[FdUtils] fcntl(F_SETFD) failed for fd " << fd;
        throw std::runtime_error(oss.str());
    }
}

}  // namespace FdUtils
