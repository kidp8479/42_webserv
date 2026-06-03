#include "FdUtils.hpp"
#include <fcntl.h>
#include <sstream>
#include <stdexcept>

namespace FdUtils {

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

}
