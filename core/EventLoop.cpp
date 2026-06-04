#include "EventLoop.hpp"

#include <poll.h>

#include <cerrno>
#include <cstring>
#include <vector>

#include "../logger/Logger.hpp"
#include "../utils/LogUtils.hpp"
#include "IEventHandler.hpp"

/**
 * @brief Constructs an empty event loop.
 */
EventLoop::EventLoop() {
}

/**
 * @brief Destroys the event loop and all remaining handlers.
 *
 * Removes any registered handlers and releases resources before
 * shutdown.
 */
EventLoop::~EventLoop() {
    LOG_INFO() << BR_CYN "[EventLoop] shutting down, cleaning up "
               << handlers_.size() << " handlers" << RESET;
    for (size_t i = 0; i < handlers_.size(); i++) {
        LOG_DEBUG() << BR_YEL "[EventLoop] destroying handler="
                    << handlers_[i]->name() << " fd=" << poll_fds_[i].fd
                    << RESET;
        delete handlers_[i];
    }
    handlers_.clear();
    poll_fds_.clear();
}

/**
 * @brief Registers a handler with the event loop.
 * Adds the handler's file descriptor to the poll set and associates
 * it with the corresponding IEventHandler instance.
 *
 * @note poll_fds_ and handlers_ are parallel containers and must
 * always remain synchronized by index.
 *
 * @param handler Handler to register.
 * @param events Poll events to monitor.
 */
void EventLoop::addHandler(IEventHandler* handler, short events) {
    pollfd p;
    p.fd = handler->getFd();
    p.events = events;
    p.revents = 0;

    poll_fds_.push_back(p);
    handlers_.push_back(handler);

    LOG_DEBUG() << BR_YEL "[EventLoop] add fd=" << p.fd
                << " events=" << LogUtils::pollToStr(events)
                << " handler=" << handler->name() << RESET;
}

/**
 * @brief Updates the events monitored for a registered handler.
 * Searches for the handler and modifies its corresponding poll
 * registration.
 *
 * @note poll_fds_ and handlers_ are parallel containers and must
 * always remain synchronized by index.
 *
 * @param handler Handler to update.
 * @param events New poll events to monitor.
 */
void EventLoop::modifyHandler(IEventHandler* handler, short events) {
    for (size_t i = 0; i < handlers_.size(); i++) {
        if (handlers_[i] == handler) {
            LOG_DEBUG() << BR_YEL "[EventLoop] modify fd=" << poll_fds_[i].fd
                        << " events=" << LogUtils::pollToStr(events) << RESET;

            poll_fds_[i].events = events;
            return;
        }
    }
}

/**
 * @brief Removes and destroys a registered handler.
 * Unregisters the handler from the poll set, keeps the internal
 * containers compact, and deletes the handler object.
 *
 * @note poll_fds_ and handlers_ are parallel containers and must
 * always remain synchronized by index.
 *
 * @param handler Handler to remove.
 */
void EventLoop::removeHandler(IEventHandler* handler) {
    for (size_t i = 0; i < handlers_.size(); i++) {
        if (handlers_[i] == handler) {
            LOG_INFO() << BR_CYN "[EventLoop] remove fd=" << poll_fds_[i].fd
                       << " handler=" << handler->name() << RESET;
            size_t last = handlers_.size() - 1;
            if (i != last) {
                handlers_[i] = handlers_[last];
                poll_fds_[i] = poll_fds_[last];
            }
            handlers_.pop_back();
            poll_fds_.pop_back();
            delete handler;
            return;
        }
    }
}

/**
 * @brief Waits for I/O events using poll().
 * Blocks for up to the specified timeout and returns the number of
 * ready file descriptors. Interrupted system calls are handled and
 * treated as non-fatal.
 *
 * @param timeout Maximum time to wait in milliseconds.
 * @return Number of ready descriptors, 0 on timeout/interruption,
 * or -1 on poll failure.
 */
int EventLoop::wait(int timeout) {
    if (poll_fds_.empty())
        return 0;
    int ret = poll((&poll_fds_[0]), poll_fds_.size(), timeout);
    if (ret == -1) {
        if (errno == EINTR) {
            LOG_WARNING() << "[EventLoop] system call interrupted";
            return 0;
        }
        LOG_ERROR() << "[EventLoop] poll() failed: " << strerror(errno);
        return -1;
    }
    LOG_DEBUG() << BR_YEL "[EventLoop] poll returned ready_fds=" << ret
                << RESET;
    return ret;
}

/**
 * @brief Dispatches ready events to registered handlers.
 *
 * Invokes handle() on each handler whose file descriptor has
 * pending poll events. Exceptions thrown by handlers are caught
 * and logged to prevent the event loop from terminating.
 */
void EventLoop::dispatch() {
    for (size_t i = 0; i < poll_fds_.size(); i++) {
        if (poll_fds_[i].revents != 0) {
            try {
                handlers_[i]->handle(poll_fds_[i].revents);
            } catch (const std::exception& e) {
                LOG_ERROR() << "[EventLoop] handler exception: " << e.what();
            }
        }
    }
}

/**
 * @brief Removes completed and timed-out handlers.
 * Calls onTimeout() for handlers whose timeout has expired,
 * then destroys and unregisters handlers marked as done or
 * timed out.
 *
 * @note poll_fds_ and handlers_ are parallel containers and must
 * always remain synchronized by index.
 */
void EventLoop::cleanup() {
    for (size_t i = 0; i < handlers_.size();) {
        bool dead = handlers_[i]->isDone();
        bool timeout = handlers_[i]->isTimedOut();

        if (dead || timeout) {
            if (timeout && !dead) {
                LOG_WARNING() << "[EventLoop] timeout fd=" << poll_fds_[i].fd
                              << " handler=" << handlers_[i]->name();
                try {
                    handlers_[i]->onTimeout();
                } catch (const std::exception& e) {
                    LOG_ERROR() << "[EventLoop] timeout handler exception: "
                                << e.what();
                }
            }
            LOG_DEBUG() << "[EventLoop] removing fd=" << poll_fds_[i].fd
                        << " handler=" << handlers_[i]->name();
            delete handlers_[i];

            size_t last = handlers_.size() - 1;
            if (i != last) {
                handlers_[i] = handlers_[last];
                poll_fds_[i] = poll_fds_[last];
            }
            handlers_.pop_back();
            poll_fds_.pop_back();
        } else {
            i++;
        }
    }
}
