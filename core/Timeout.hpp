#ifndef TIMEOUT_HPP
#define TIMEOUT_HPP

#include <ctime>

namespace TimeoutSeconds {
	static const int kClient = 5; // set at 60, reduce value for testing
	static const int kCGI = 30; // for later
}

// poll expect time in milliseconds
// the separation of then namespaces prevents accidentally passing
// seconds where milliseconds are expected (and have silent failures)
namespace TimeoutMs {
	static const int kPollHeartbeat = 10000; //set at 10000, reduce for testing
}

class Timeout {
public:
	Timeout();
	explicit Timeout(int limit_seconds);
	Timeout(const Timeout& other);
	Timeout& operator=(const Timeout& other);
	~Timeout();

	void reset();
	bool expired() const;
	int limit() const;

private:
	time_t last_activity_;
	int limit_seconds_;
};

#endif
