#ifndef EVENTLOOP_HPP
#define EVENTLOOP_HPP

#include <poll.h>
#include <vector>
#include <map>
#include <set>

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

	// see the struct in the notes below
	std::vector<pollfd> poll_fds_;
	// this is where Listener* and/or Client* is stored
	std::vector<IEventHandler*> handlers_;
};
#endif

/**NOTE:
 *  struct pollfd {
 *  int   fd;         file descriptor
 *  short events;     requested events (input)
 *  short revents;    returned events (output) <-- kernel fills this out
 *  };
 *  POLLIN   = 1   (0x0001)
 *  POLLOUT  = 4   (0x0004)
 *  POLLERR  = 8   (0x0008)
 *  POLLHUP  = 16  (0x0010)
 *  POLLNVAL = 32  (0x0020)
 *  events are bitmasks:
 *  POLLIN  → data to read
 *  POLLOUT → ready to write
 *  POLLERR → error occurred
 *  POLLHUP → connection closed
 *  POLLNVAL → invalid fd
 */

