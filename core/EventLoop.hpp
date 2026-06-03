#ifndef EVENTLOOP_HPP
#define EVENTLOOP_HPP

#include <poll.h>

#include <map>
#include <set>
#include <vector>

class IEventHandler;

class EventLoop {
public:
    EventLoop();
    ~EventLoop();

    void addHandler(IEventHandler* handler, short events);
    void modifyHandler(IEventHandler* handler, short events);
    void removeHandler(IEventHandler* handler);

    int wait(int timeout);

    void dispatch();
    void cleanup();

private:
    EventLoop(const EventLoop&);
    EventLoop& operator=(const EventLoop&);

    std::vector<pollfd> poll_fds_;
    std::vector<IEventHandler*> handlers_;
};
#endif
