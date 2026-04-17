#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <stddef.h>
#include "../http/Request.hpp"
#include "../http/Response.hpp"
#include "Fd.hpp"

class Client {
public:
	enum State {
		kReading,
		kWriting,
		kDone
	};

	//avoid unintentional construction by using explict keyword
	// w/o explicit this is allowed Client c = 42
	explicit Client(int fd);
	~Client();

	// getters
    int			getFd() const;
	size_t		getBytesSent() const;
	State		getState() const;
	Request&	getRequest();
	Response&	getResponse();
	
	// setter
	void	setState(State new_state);

void	addBytesSent(size_t n);

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
