#ifndef SERVER_HPP
#define SERVER_HPP

#include "Client.hpp"
#include <vector>
#include <map>
#include "../config/Config.hpp"

class Server {
public:
	// pulled this number from the man, in case anyone is wondering
	static const int kBACKLOG = 128;

	Server(const Config& config);
	~Server();

	bool	start(); // bind, listen, enter main loop
	void	stop(); // close sockets
	
	const std::vector<int>& getSockets() const;
    // TODO: comment this out before validatin
    // Grant access to private members to the test fixture
    friend class ServerTestFixture;

private:
	void	setNonBlocking(int fd);
	void	setupSocket(int socket);
	int		acceptClient();
	void	handleRead(Client& client);
	void	handleWrite(Client& client);

	//	helpers
	void serverError(const std::string& msg);

	const Config&			config_;
	std::vector<int>		sockets_;
	std::map<int, Client*>	clients_;
};

#endif
