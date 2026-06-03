#ifndef CGISTDINWRITER_HPP
#define CGISTDINWRITER_HPP

#include <string>

#include "Fd.hpp"
#include "IEventHandler.hpp"

class EventLoop;

/**
 * @brief Writes CGI request body to CGI process stdin.
 * Non-blocking event handler that streams the HTTP request body
 * into the CGI process pipe using write events.
 *
 * Handles partial writes, completion detection, and timeout cleanup.
 */
class CgiStdinWriter : public IEventHandler {
public:
    CgiStdinWriter(int write_fd, const std::string& body, EventLoop& loop);
    ~CgiStdinWriter();

    int getFd() const;
    void handle(short revents);
    bool isDone() const;
    bool isTimedOut() const;
    const char* name() const;

private:
    CgiStdinWriter(const CgiStdinWriter&);
    CgiStdinWriter& operator=(const CgiStdinWriter&);

    Fd write_fd_;
    std::string body_;
    size_t bytes_written_;
    EventLoop& loop_;
    bool done_;
};

#endif
