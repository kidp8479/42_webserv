#ifndef CGIPROCESS_H
#define CGIPROCESS_H

#include <sys/types.h>

#include <string>

#include "EventLoop.hpp"
#include "Fd.hpp"
#include "IEventHandler.hpp"
#include "Timeout.hpp"

class Client;

class CgiProcess : public IEventHandler {
public:
    CgiProcess(pid_t pid, int read_fd, Client& client, EventLoop& loop);
    ~CgiProcess();

    int getFd() const;
    void handle(short revents);
    bool isDone() const;
    bool isTimedOut() const;
	void onTimeout();
    const char* name() const;

private:
    // canonical form, copy, assignment
    CgiProcess(const CgiProcess&);
    CgiProcess& operator=(const CgiProcess&);

    void finish();  // parse output deliver to client, mark done

    pid_t pid_;
    Fd read_fd_;
    Client& client_;
    EventLoop& loop_;
    std::string output_;
    Timeout timeout_;
    bool done_;
};

#endif
