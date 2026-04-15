#ifndef FD_HPP
#define FD_HPP

#include <unistd.h>

class Fd {
public:
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
