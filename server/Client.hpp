#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <stddef.h>
#include "../http/Request.hpp"
#include "../http/Response.hpp"
#include "Fd.hpp"
#include <sys/socket.h>

class Client {
public:
	enum State {
		kReading,
		kWriting,
		kDone
	};
	enum ReadResult {
		kReadPending,
		kReadComplete,
		kReadClosed,
		kReadError
	};
	enum WriteResult {
		kWritePending,
		kWriteDone,
		kWriteError
	};
	
	// why this number? 4k is typical page size and still safe on stack
	// reduces syscall overhead vs 1K
	static const size_t kBufferSize = 4096;

	//avoid unintentional construction by using explict keyword
	// w/o explicit this is allowed Client c = 42
	explicit Client(int fd);
	~Client();

    int			getFd() const;
	State		getState() const;
	Request&	getRequest();
	Response&	getResponse();

	// I/O
	ReadResult	read();
	WriteResult	write();

private:
	// no copying or assignment allowed, client owns fd
	// rule of thumb: any class that owns resrouces must not
	// be copyable
	Client(const Client&);
	Client& operator=(const Client&);

	Fd			fd_;
	size_t		bytes_sent_;
	State		state_;
	Request		request_;
	Response	response_;
};

#endif
