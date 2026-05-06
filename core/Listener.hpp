#ifndef LISTENER_HPP
#define LISTENER_HPP

#include "EventLoop.hpp"
#include "IEventHandler.hpp"
#include "Fd.hpp"
#include "ServerResources.hpp"

class Listener : public IEventHandler {
public:
	Listener(int port, EventLoop& loop, const ServerResources& resources);

	int getFd() const;
	void handle(short revents);
	const char* name() const;

private:
	bool isDone() const;
	void setNonBlocking(int fd);
	void acceptClients();

	Fd fd_;
	// reference to server's loop_
	EventLoop& loop_;
	const ServerResources& resources_;
};

#endif
