#include "Timeout.hpp"

/**
 * @brief Default timeout (inactive by default).
 * Initializes with no timeout limit (-1), meaning it never expires
 * unless explicitly configured.
 *
 * @note Used for optional timeout-enabled handlers.
 */
Timeout::Timeout() : last_activity_(std::time(NULL)), limit_seconds_(-1) {
}

/**
 * @brief Constructs a timeout with a fixed limit.
 * @param limit_seconds Maximum allowed inactivity duration.
 */
Timeout::Timeout(int limit_seconds)
    : last_activity_(std::time(NULL)), limit_seconds_(limit_seconds) {
}

/**
 * @brief Copy constructor.
 */
Timeout::Timeout(const Timeout& other)
    : last_activity_(other.last_activity_),
      limit_seconds_(other.limit_seconds_) {
}

/**
 * @brief Copy assignment operator.
 */
Timeout& Timeout::operator=(const Timeout& other) {
    if (this != &other) {
        last_activity_ = other.last_activity_;
        limit_seconds_ = other.limit_seconds_;
    }
    return *this;
}

/**
 * @brief Destructor.
 */
Timeout::~Timeout() {
}

/**
 * @brief Resets inactivity timer to current time.
 */
void Timeout::reset() {
    last_activity_ = std::time(NULL);
}

/**
 * @brief Checks whether the timeout has expired.
 * @return true if current time exceeds limit_seconds since last reset.
 *
 * @note If limit is negative, timeout is disabled and always returns false.
 */
bool Timeout::expired() const {
    if (limit_seconds_ < 0) {
        return false;
    }
    return std::difftime(std::time(NULL), last_activity_) > limit_seconds_;
}

/**
 * @brief Returns configured timeout limit in seconds.
 * @return Timeout limit, or -1 if disabled.
 */
int Timeout::limit() const {
    return limit_seconds_;
}
