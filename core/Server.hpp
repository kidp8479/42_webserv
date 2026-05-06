#ifndef SERVER_HPP
#define SERVER_HPP

#include <map>
#include <vector>
#include <string>

#include "../config/Config.hpp"
#include "Client.hpp"
#include "EventLoop.hpp"
#include <set>
#include "Listener.hpp"
#include "ServerResources.hpp"

class Server {
public:
    Server(const Config& config);
    ~Server();

    bool start();

private:
    Server(const Server&);
    Server& operator=(const Server&);

	void setupListeners();

    const Config& config_;
	ServerResources resources_;
	EventLoop loop_;

	std::vector<Listener*> listeners_;
};

#endif
