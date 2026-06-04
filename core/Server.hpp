#ifndef SERVER_HPP
#define SERVER_HPP

#include <map>
#include <set>
#include <string>
#include <vector>

#include "../config/Config.hpp"
#include "Client.hpp"
#include "EventLoop.hpp"
#include "Listener.hpp"
#include "ServerResources.hpp"

/**
 * @brief High-level server orchestrator.
 *
 * Manages the event loop and listener sockets for all configured
 * server blocks. Responsible for initializing listeners and
 * starting the main event-driven runtime.
 * Owns Listener objects and drives the EventLoop lifecycle.
 */
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
    EventLoop loop_;
    std::vector<Listener*> listeners_;
};

#endif
