#ifndef IEVENTHANDLER_HPP
#define IEVENTHANDLER_HPP

/**
 * @brief Interface for objects managed by the EventLoop.
 * Implementations provide a file descriptor, handle poll events,
 * report completion and timeout status, and optionally react to
 * timeout expiration.
 */
class IEventHandler {
public:
    virtual ~IEventHandler() {
    }

    virtual int getFd() const = 0;
    virtual void handle(short revents) = 0;
    virtual bool isDone() const = 0;
    virtual bool isTimedOut() const = 0;
    virtual const char* name() const = 0;

    virtual void onTimeout() {
    }
};

#endif
