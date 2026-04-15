#ifndef FD_HPP
#define FD_HPP

#include <unistd.h>

class Fd {
public:
	// this is a ctor with a default argument
	// at construction time if no fd is specified it is defaulted to -1
	// ex: Fd a;  fd = -1
	// Fd b(5); fd = 5
	// note: default arguments belong in declaration
	explicit Fd(int fd = -1);
	~Fd();

	int		getFd() const;

	void	reset(int fd = -1);
	int		release();
	bool	valid() const;

private:
	int	fd_;
};

#endif
