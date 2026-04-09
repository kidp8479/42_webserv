#ifndef SERVER_HPP
#define SERVER_HPP

#include <vector>
#include "../config/Config.hpp"

class Server
{
private:
	const Config& config_;
	std::vector<int> sockets_;

//	void	acceptConnections();
//	void	handleClient(int client_fd);
	void	setupSocket(int socket);

public:
	Server(const Config& config);
	~Server();

	bool	start(); // bind, listen, enter main loop
	void	stop(); // close sockets
};

#endif
