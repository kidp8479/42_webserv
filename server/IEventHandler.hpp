#ifndef IEVENTHANDLER_HPP
#define IEVENTHANDLER_HPP

class IEventHandler {
public:
	virtual ~IEventHandler() {}

	virtual int getFd() const = 0;
	virtual void handle(short revents) = 0;
	virtual bool isDone() const = 0;

	virtual const char* name() const = 0;
};

#endif
