#include "Timeout.hpp"

Timeout::Timeout() :
	last_activity_(std::time(NULL)),
	//  if we have a handler where timeout is optional or not yet configured,
	//  expired() safely returns false without any special-casing at the call site.
	limit_seconds_(-1) {
}

Timeout::Timeout(int limit_seconds) :
	last_activity_(std::time(NULL)),
	limit_seconds_(limitseconds) {
}

Timeout::Timeout(const Timeout& other) :
	last_activity_(other.last_activity_),
	limit_seconds_(other.limit_seconds) {
}

Timeout& Timeout::operator=(const Timeout& other) {
	if (this != &other) {
		last_activity_(other.last_activity_);
		limit_seconds_(other.limit_seconds_);
	}
	return *this;
}

Timeout::~Timeout() {
}

void Timeout::reset() {
	last_activity_ = std::time(NULL);
}

bool Timeout::expired() const {
	if (limit_seconds_ < 0) {
		return false;
	}
	return std::difftime(std::time(NULL), last_activity_) > limit_seconds_;
}

int Timeout::limit() const {
	return limit_seconds_;
}
