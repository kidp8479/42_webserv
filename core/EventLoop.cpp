#include "EventLoop.hpp"
#include "../logger/Logger.hpp"
#include <cstring>
#include <poll.h>
#include <cerrno>
#include <vector>
#include "IEventHandler.hpp"
#include "../utils/LogUtils.hpp"

EventLoop::EventLoop() {}

EventLoop::~EventLoop() {}
/**
 * register an fd (can be client or listener) with the kernel with polling
 * associate that fd with a handler object
 * these two must always be kept in sync
 */
void EventLoop::addHandler(IEventHandler* handler, short events) {
	pollfd p;
	p.fd = handler->getFd();
	p.events = events;
	p.revents = 0;

	// push into vector
	poll_fds_.push_back(p);
	handlers_.push_back(handler);

	LOG_DEBUG() << "[EventLoop] add fd=" << p.fd
            << " events=" << LogUtils::pollToStr(events)
            << " handler=" << handler->name();
}

/**
 * Updates the events monitored by poll for a given handler.
 * Used to switch between read/write readiness during runtime.
 */
void EventLoop::modifyHandler(IEventHandler* handler, short events) {
	for (size_t i = 0; i < handlers_.size(); i++) {
		if (handlers_[i] == handler) {
			LOG_DEBUG() << "[EventLoop] modify fd="
			            << poll_fds_[i].fd
			            << " events=" << LogUtils::pollToStr(events);

			poll_fds_[i].events = events;
			return ;
		}
	}
}

void EventLoop::removeHandler(IEventHandler* handler) {
	// Loop through all registered handlers to find the target
	for (size_t i = 0; i < handlers_.size(); i++) {
		if (handlers_[i] == handler) {
			LOG_INFO() << "[EventLoop] remove fd="
			           << poll_fds_[i].fd
			           << " handler=" << handler->name();
			// Index of last element in vectors
			size_t last = handlers_.size() - 1;
			// If the element to remove is not the last one,
			// we swap it with the last element to keep vectors compact
			if (i != last) {
				handlers_[i] = handlers_[last];
				poll_fds_[i] = poll_fds_[last];
			}
			// remove the last element
			handlers_.pop_back();
			poll_fds_.pop_back();
			// delete handler object
			delete handler;
			return ;
		}
	}
}

/**
 * this is the core of where polling happens
 * we pass timeout:
 * -1 = sleep until 1 fd is ready (zero cpu usage while idle)
 *  0 = don't wait (non blocking)
 *  >0 = wait this many milliseconds
 * poll scans the entire array O(n) (using epoll/kqueue more efficient
 * but also more complex)
 */
int EventLoop::wait(int timeout) {
	if (poll_fds_.empty())
		return 0;
	// poll expect a pointer to an array for first arg
	// we give poll a list of fds, and it puts the thread to sleep
	// until any of them becomes ready,timesout, or signal interrupted.
	int ret = poll((&poll_fds_[0]), poll_fds_.size(), timeout);
	//under the hood when poll is called, the program jumps to the kernel
	// looks at each fd, and notes what we are waiting for (events)
	// checks are any of thes ready RIGHT NOW?, if yes it fills revents
	// the number returned is how many revents filled out
	if (ret == -1) {
		LOG_WARNING() << "System call interrupted";
		if (errno == EINTR) {
			return 0; // normal interrupt
		}
		LOG_ERROR() << "[EventLoop] poll() failed: " << strerror(errno);
		return -1; // real error
	}
	LOG_DEBUG() << "[EventLoop] poll returned ready_fds=" << ret;
	return ret;
}
/**
 * here is our polymorphism in action: reactor pattern
 * reacts differently depending on which object it is
 * dispatch calls handle() so depending on what the object is:
 * listener* -> calls Listener::handle() -> acceptClients()
 * client* -> calls Client::handle() -> calls read() or write()
 */
void EventLoop::dispatch() {
	for (size_t i = 0; i < poll_fds_.size(); i++) {
		if (poll_fds_[i].revents != 0) {
			try {
				handlers_[i]->handle(poll_fds_[i].revents);
			}
			catch (const std::exception& e) {
				LOG_ERROR() << "[EventLoop] handler exception: " << e.what();
			}
		}
	}
}

/**
 * we clean up any dead client fds here
 * listener isDone always returns false, as this stream needs to remain open
 * Client is created in Listener, but only EventLoop is allowed to delete
 * since it lives in handlers_ AND EventLoop knows the lifecycle of allowed
 * active Clients
 */
void EventLoop::cleanup() {
	for (size_t i = 0; i < handlers_.size(); ) {
		if (handlers_[i]->isDone()) {
			LOG_INFO() << "[EventLoop] removing fd="
			           << poll_fds_[i].fd
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
