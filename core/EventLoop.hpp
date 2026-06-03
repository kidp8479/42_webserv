#ifndef EVENTLOOP_HPP
#define EVENTLOOP_HPP

#include <poll.h>

#include <map>
#include <set>
#include <vector>

class IEventHandler;

/**
 * @brief Poll-based event dispatcher.
 * Manages a collection of IEventHandler objects and monitors their
 * file descriptors using poll(). Ready events are dispatched to the
 * appropriate handlers, while completed or timed-out handlers are
 * removed during cleanup.
 *
 * @note The EventLoop does not own active handlers by default. Handlers
 * are registered and removed through addHandler(), modifyHandler(),
 * and removeHandler().
 */
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
