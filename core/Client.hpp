#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

#include "../http/Request.hpp"
#include "../http/Response.hpp"
#include "../utils/HttpConstants.hpp"
#include "EventLoop.hpp"
#include "Fd.hpp"
#include "IEventHandler.hpp"
#include "ServerResources.hpp"
#include "Timeout.hpp"

class Request;
class Response;
class CgiProcess;

/**
 * @brief Represents a single HTTP client connection.
 *
 * Handles request parsing, response generation, CGI execution,
 * and non-blocking I/O through the event-driven EventLoop.
 *
 * Each Client owns its connection state (read/write/CGI lifecycle)
 * while sharing ServerResources with other clients on the same server.
 *
 * @note ServerResources is shared across all clients to enable
 * session persistence (e.g. cookies). It is guaranteed to outlive
 * all Client instances via Listener ownership.
 *
 * @note CGI execution is asynchronous and delegated to CgiProcess.
 * The client transitions through internal states (kReading,
 * kWaitingCgi, kWriting, kDone) depending on I/O and CGI lifecycle.
 */
class Client : public IEventHandler {
public:
    enum State { kReading, kWaitingCgi, kWriting, kDone };
    static const size_t kBufferSize = 4096;

    explicit Client(int fd, EventLoop& loop, ServerResources& resources,
                    const std::string& peer_ip);
    ~Client();

    int getFd() const;
    bool isDone() const;
    void handle(short revents);
    bool isTimedOut() const;
    const char* name() const;

    EventLoop& getLoop();
    Response& getResponse();
    const ServerResources& getResources() const;
    ServerResources&
    getResources();  // non-const overload for session management
    const ServerConfig& getServerConfig() const;
    const Router& getRouter() const;
    std::string getPeerIp() const;

    void onCgiFinished(const std::string& raw_cgi_output);
    void onTimeout();

    void receiveError(HttpConstants::HttpError error);
    void setPendingCgi(CgiProcess* cgi);

private:
    Client(const Client&);
    Client& operator=(const Client&);

    void read();
    void write();
    void cleanup();
    void closeConnection(const std::string& reason, const char* level = "INFO");

    Fd fd_;
    EventLoop& loop_;
    // reference to Listener's resources_ — all Clients on the same server share
    // the same ServerResources instance, which is required for cookie session
    // data (sessions_) to be visible across connections. Listener outlives all
    // its Clients so the reference is always valid.
    ServerResources& resources_;

    size_t bytes_sent_;
    State state_;

    Request request_;
    Response response_;
    bool keep_alive_;
    Timeout timeout_;
    CgiProcess* pending_cgi_;
    std::string peer_ip_;
};

#endif
