#include "CgiProcess.hpp"

#include <signal.h>
#include <sys/wait.h>

#include <cerrno>

#include "Client.hpp"

CgiProcess::CgiProcess(pid_t pid, int read_fd, Client& client, EventLoop& loop)
    : pid_(pid),
      read_fd_(read_fd),
      client_(client),
      loop_(loop),
      timeout_(TimeoutSeconds::kCGI),
      done_(false) {
}

CgiProcess::~CgiProcess() {
	if (!done_) {
        kill(pid_, SIGKILL);
    }
}

int CgiProcess::getFd() const {
    return read_fd_.getFd();
}

void CgiProcess::handle(short revents) {
    if (done_) {
        return;
    }
    // event loop checks isTimedout() in cleanup(), but handle() may get
    // called before clenaup() runs
    if (timeout_.expired()) {
        kill(pid_, SIGKILL);
        read_fd_.reset();
        client_.receiveError(HttpConstants::kGatewayTimeout);
        done_ = true;
        return;
    }
    if (revents & POLLIN) {
        char buffer[4096];
        ssize_t n = read(read_fd_.getFd(), buffer, sizeof(buffer));

        if (n > 0) {
            output_.append(buffer, n);
        } else if (n == 0) {
            // EOF. cgi script finished, we have all output
            finish();
        }
    }
    if (revents & (POLLHUP | POLLERR)) {
        finish();
    }
}

bool CgiProcess::isDone() const {
    return done_;
}

bool CgiProcess::isTimedOut() const {
    return timeout_.expired();
}

const char* CgiProcess::name() const {
    return "CgiProcess";
}

void CgiProcess::finish() {
    if (done_) {
        return;
    }
    done_ = true;
    read_fd_.reset();
    client_.onCgiFinished(output_);
}
