#ifndef SERVER_HPP
#define SERVER_HPP

#include <map>
#include <vector>

#include "../config/Config.hpp"
#include "Client.hpp"

/**
 * @brief Core server responsible for socket lifecycle and client management.
 *
 * Manages listening sockets, accepts clients, and dispatches I/O handling.
 *
 * Clients are stored as raw pointers because:
 * - Client is non-copyable (owns fd and state)
 * - Server has exclusive ownership responsibility
 * - Explicit delete ensures controlled lifecycle management
 *
 * This avoids accidental copies and ensures clear ownership semantics.
 */
class Server {
public:
    Server(const Config& config);
    ~Server();

    bool start();  // bind, listen, enter main loop
    void stop();   // close sockets
    const std::vector<int>& getSockets() const;

    // Grant access to private members to the test fixture
    friend class ServerTestFixture;

private:
    // cannot be copied or assigned because server owns sockets_
    // and clients_. this prevents double close on sockets and
    // double delete on clients.
    Server(const Server&);
    Server& operator=(const Server&);

    void setupSocket(int socket);
    void setNonBlocking(int fd);
    int acceptClient();
    void handleRead(Client& client);
    void handleWrite(Client& client);
    void serverError(const std::string& msg);

    const Config& config_;
    std::vector<int> sockets_;
    // Pointer used because Client is non-copyable and owned dynamically.
    // Server is responsible for lifetime management (new/delete).
    std::map<int, Client*> clients_;
};

#endif
