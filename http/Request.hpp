#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>
// place holder class
class Request {
public:
    Request();
    ~Request();

    // temporary stub API (enough for Client/Server)
    void	append(const char* data, size_t len);
	bool	isComplete() const { return true; }

private:
    std::string raw_;
};

#endif
