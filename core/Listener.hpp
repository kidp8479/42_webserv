#ifndef LISTENER_HPP
#define LISTENER_HPP

#include "EventLoop.hpp"
#include "Fd.hpp"
#include "IEventHandler.hpp"
#include "ServerResources.hpp"

class Listener : public IEventHandler {
public:
    Listener(EventLoop& loop, const ServerResources& resources);
    ~Listener();

    int getFd() const;
    void handle(short revents);
    const char* name() const;

private:
    Listener(const Listener&);
    Listener& operator=(const Listener&);

    bool isDone() const;
    void setupSocket();
    void setNonBlocking(int fd);
    void acceptClients();
    bool isTimedOut() const;

    Fd fd_;
    // reference to server's loop_
    EventLoop& loop_;
    // non-const: Clients hold a reference to this instance, so cookie session
    // writes (createSession, getOrCreateSession) must be allowed on it
    ServerResources resources_;
};

#endif
