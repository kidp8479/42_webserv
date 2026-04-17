#ifndef SERVER_HPP
#define SERVER_HPP

#include "Client.hpp"
#include <vector>
#include <map>
#include "../config/Config.hpp"

class Server {
public:
	// why this number? 4k is typical page size and still safe on stack
	// reduces syscall overhead vs 1K
	static const size_t kBufferSize = 4096;

	Server(const Config& config);
	~Server();

	bool					start(); // bind, listen, enter main loop
	void					stop(); // close sockets
	const std::vector<int>& getSockets() const;

    // TODO: comment this out before validatin
    // Grant access to private members to the test fixture
    friend class ServerTestFixture;

private:
	// cannot be copied or assigned because server owns sockets_
	// and clients_. this prevents double close on sockets and 
	// double delete on clients.
	Server(const Server&);
	Server& operator=(const Server&);

	void	setupSocket(int socket);
	void	setNonBlocking(int fd);
	int		acceptClient();
	void	handleRead(Client& client);
	void	handleWrite(Client& client);
	void	serverError(const std::string& msg);

	const Config&			config_;
	std::vector<int>		sockets_;
	// Pointer used because Client is non-copyable and owned dynamically.
	// Server is responsible for lifetime management (new/delete).
	std::map<int, Client*>	clients_;
};

#endif
