#ifndef FD_HPP
#define FD_HPP

#include <unistd.h>

/**
 * @brief RAII wrapper managing a file descriptor lifetime.
 *
 * Ensures the fd is closed on destruction unless released.
 * Copying is disabled to prevent double-close (unique ownership).
 *
 * @note Explicit constructor avoids implicit int -> Fd conversion.
 */
class Fd {
public:
    explicit Fd(int fd = -1);
    ~Fd();

    int getFd() const;
    void reset(int fd = -1);
    int release();
    bool valid() const;

private:
    // no copying allowed
    Fd(const Fd&);
    Fd& operator=(const Fd&);

    int fd_;
};

#endif
