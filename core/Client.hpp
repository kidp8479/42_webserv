#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "../http/Request.hpp"
#include "../http/Response.hpp"
#include "Fd.hpp"
#include "EventLoop.hpp"
#include "IEventHandler.hpp"

class Client : public IEventHandler {
public:
	enum State { kReading, kWriting, kDone };
    static const size_t kBufferSize = 4096;

    explicit Client(int fd, EventLoop& loop);
    ~Client();

    int getFd() const;
	void handle(short revents);
	const char* name() const;
	
	
private:
    Client(const Client&);
    Client& operator=(const Client&);

	bool isDone() const;
	void read();
	void write();
	void cleanup();

    Fd fd_;
	//reference to the server's loop_
	EventLoop& loop_;

    size_t bytes_sent_;
    State state_;

	Request request_;
    Response response_;
	bool keep_alive_;
};

#endif
