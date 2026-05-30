#ifndef CGISTDINWRITER_HPP
#define CGISTDINWRITE_HPP

#include "IEventHandler.hpp"
#include <string>
#include "EventLoop.hpp"
#include "Fd.hpp"

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
	CgiStdinWriter operator=(const CgiStdinWriter&);

	Fd write_fd_;
	std::string body_;
	size_t bytes_written_;
	EventLoop& loop_;
	bool done_;
};

#endif
