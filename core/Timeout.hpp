#ifndef TIMEOUT_HPP
#define TIMEOUT_HPP

#include <ctime>

class Timeout {
public:
	Timeout();
	explicit Timeout(int limit_seconds);
	Timeout(const Timeout& other);
	Timeout& operator=(const Timeout& other);
	~Timeout() {}

	void reset();
	bool expired() const;
	int limit() const;

private:
	time_t last_activity_;
	int limit_seconds_;
};

#endif
