#ifndef SERVER_HPP
#define SERVER_HPP

#include <vector>
#include "../config/Config.hpp"

class Server
{
private:
	const Config& config_;
	std::vector<int> sockets_;

	void	setupSocket(int socket);
	int		acceptClient();
	void	handleClient(int client_fd);
	void	sendResponse(int client_fd);
//	buildResponse()

public:
	Server(const Config& config);
	~Server();

	bool	start(); // bind, listen, enter main loop
	void	stop(); // close sockets
					//
	const std::vector<int>& getSockets() const;

    // Grant access to private members to the test fixture
	// TODO comment this out before validating
    friend class ServerTestFixture;
};

#endif
