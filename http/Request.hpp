#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>

/**
 * @brief Temporary HTTP request container and parser stub.
 *
 * Stores raw incoming HTTP request data received from the network.
 *
 * Provides minimal functionality required by the current server pipeline:
 * - Appending streamed data from the socket
 * - Detecting request completion (header termination)
 *
 * @note This is a temporary stub implementation used to unblock server
 *       development. Full HTTP parsing (method, URI, headers, body parsing)
 *       will be implemented later in the dedicated HTTP parsing layer
 *       (Charlie’s part).
 */
class Request {
public:
    Request();
    ~Request();

    // temporary stub API (enough for Client/Server)
    void append(const char* data, size_t len);
    bool isComplete() const;

private:
    std::string raw_;
};

#endif
