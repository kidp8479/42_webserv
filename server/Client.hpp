#ifndef CLIENT_HPP
#define CLIENT_HPP

class Client {
public:
	enum class State {
		kReading,
		kWriting,
		kDone
	};

	//avoid unintentional construction by using explict keyword
	// w/o explicit this is allowed Client c = 42
	explict Client(int fd);
	~Client();

	// getters
    int		getFd() const;
	State	getState() const;

	// request handling
	void	appendRequest(const char* data, size_t size);
	bool	isRequestComplete() const;

	// response handling
	void	setResponse(const std::string& response);
	bool	hasResponse() const;
	void	sendResponse();

private:
	void	setState(State new_state);

	int			fd_;
	std::string	request_buffer_;
	std::string	response_buffer_;
	size_t		bytes_sent_;
	State		state_;
};

#endif
