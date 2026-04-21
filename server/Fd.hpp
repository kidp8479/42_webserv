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
    // this is a ctor with a default argument
    // at construction time if no fd is specified it is defaulted to -1
    // ex: Fd a;  fd = -1
    // Fd b(5); fd = 5
    // note: default arguments belong in declaration
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
