#ifndef CGIPROCESS_H
#define CGIPROCESS_H

#include <sys/types.h>

#include <string>

#include "Fd.hpp"
#include "IEventHandler.hpp"
#include "Timeout.hpp"

class Client;
class EventLoop;

/**
 * @class CgiProcess
 * @brief Manages the lifecycle of a CGI subprocess.
 *
 * This class is responsible for:
 * - Tracking a spawned CGI process (PID)
 * - Reading output from the CGI pipe
 * - Integrating CGI execution into the EventLoop
 * - Notifying the Client when execution is complete
 * - Handling timeout and cleanup logic
 *
 * It acts as an I/O event handler registered in the EventLoop,
 * monitoring the CGI pipe for readability until completion.
 */
class CgiProcess : public IEventHandler {
public:
    static const size_t kBufferSize = 4096;

    CgiProcess(pid_t pid, int read_fd, Client& client, EventLoop& loop);
    ~CgiProcess();

    int getFd() const;
    void handle(short revents);
    bool isDone() const;
    bool isTimedOut() const;
    void onTimeout();
    const char* name() const;

private:
    CgiProcess(const CgiProcess&);
    CgiProcess& operator=(const CgiProcess&);

    void finish();

    pid_t pid_;
    Fd read_fd_;
    Client& client_;
    EventLoop& loop_;
    std::string output_;
    Timeout timeout_;
    bool done_;
};

#endif
