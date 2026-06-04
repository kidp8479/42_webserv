#ifndef TIMEOUT_HPP
#define TIMEOUT_HPP

#include <ctime>

/**
 * @brief Timeout configuration constants (seconds).
 * Defines standard timeout durations used across the server.
 * - kClient: client inactivity timeout
 * - kCGI: CGI process timeout
 */
namespace TimeoutSeconds {
static const int kClient = 60;  // set at 60, reduce value for testing
static const int kCGI = 30;
}  // namespace TimeoutSeconds

/**
 * @brief Timeout configuration constants (milliseconds).
 * Used for poll() wait intervals.
 * @note Separated from seconds to avoid unit mixups at call sites.
 */
namespace TimeoutMs {
static const int kPollHeartbeat = 10000;  // set at 10000, reduce for testing
}

/**
 * @brief Simple inactivity timeout tracker.
 * Tracks last activity time and determines whether a handler
 * has exceeded its allowed idle duration.
 */
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
