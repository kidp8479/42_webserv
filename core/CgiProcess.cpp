#include "CgiProcess.hpp"

#include <signal.h>
#include <sys/wait.h>

#include <cerrno>

#include "Client.hpp"
#include "EventLoop.hpp"

/**
 * @brief Creates a CGI process handler.
 *
 * Tracks a spawned CGI process and its output pipe, and registers it
 * into the event system with timeout management.
 *
 * @param pid Process ID of the CGI child process
 * @param read_fd Read-end of the CGI pipe
 * @param client Owning client
 * @param loop Event loop reference
 */
CgiProcess::CgiProcess(pid_t pid, int read_fd, Client& client, EventLoop& loop)
    : pid_(pid),
      read_fd_(read_fd),
      client_(client),
      loop_(loop),
      timeout_(TimeoutSeconds::kCGI),
      done_(false) {
}

/**
 * @brief Terminates the CGI process if it is still running.
 *
 * If the CGI process has not completed normally, it is forcefully
 * killed using SIGKILL to prevent orphan processes.
 */
CgiProcess::~CgiProcess() {
    if (!done_) {
        kill(pid_, SIGKILL);
    }
}

/**
 * @brief Retrieves the file descriptor used for CGI output.
 * @return Integer file descriptor associated with the CGI pipe
 */
int CgiProcess::getFd() const {
    return read_fd_.getFd();
}

/**
 * @brief Handles CGI pipe events.
 * Reads CGI output on POLLIN, detects EOF, and finalizes on HUP/ERR.
 * Also enforces timeout-based termination.
 * @param revents Poll event flags (POLLIN, POLLHUP, POLLERR)
 */
void CgiProcess::handle(short revents) {
    if (done_) {
        return;
    }
    if (timeout_.expired()) {
        kill(pid_, SIGKILL);
        read_fd_.reset();
        client_.receiveError(HttpConstants::kGatewayTimeout);
        done_ = true;
        return;
    }
    if (revents & POLLIN) {
        char buffer[kBufferSize];
        ssize_t n = read(read_fd_.getFd(), buffer, sizeof(buffer));

        if (n > 0) {
            output_.append(buffer, n);
        } else if (n == 0) {
            finish();
        }
    }
    if (revents & (POLLHUP | POLLERR)) {
        finish();
    }
}

/**
 * @brief Checks if CGI execution is completed.
 * @return True if process finished or was terminated
 */
bool CgiProcess::isDone() const {
    return done_;
}

/**
 * @brief Checks if CGI has exceeded its timeout.
 * @return True if timeout expired
 */
bool CgiProcess::isTimedOut() const {
    return timeout_.expired();
}

/**
 * @brief Forces CGI termination on timeout.
 * Kills the process, cleans resources, and notifies client
 * with a Gateway Timeout error.
 */
void CgiProcess::onTimeout() {
    kill(pid_, SIGKILL);
    read_fd_.reset();
    client_.receiveError(HttpConstants::kGatewayTimeout);
    done_ = true;
}

/**
 * @brief Returns handler name for logging/debugging.
 * @return Static string identifier
 */
const char* CgiProcess::name() const {
    return "CgiProcess";
}

/**
 * @brief Finalizes CGI execution.
 * Marks process complete, releases resources, and forwards
 * output to the client.
 */
void CgiProcess::finish() {
    if (done_) {
        return;
    }
    done_ = true;
    read_fd_.reset();
    client_.onCgiFinished(output_);
}
