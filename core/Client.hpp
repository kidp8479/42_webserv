#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "../http/Request.hpp"
#include "../http/Response.hpp"
#include "CgiProcess.hpp"
#include "EventLoop.hpp"
#include "Fd.hpp"
#include "IEventHandler.hpp"
#include "ServerResources.hpp"
#include "Timeout.hpp"

class Client : public IEventHandler {
public:
    enum State { kReading, kWaitingCgi, kWriting, kDone };
    static const size_t kBufferSize = 4096;

    explicit Client(int fd, EventLoop& loop, const ServerResources& resources);
    ~Client();

    int getFd() const;
    bool isDone() const;
    void handle(short revents);
    bool isTimedOut() const;
    const char* name() const;

    // new getters Handler and cgi need
    EventLoop& getLoop();
    Response& getResponse();
    const ServerResources& getResources() const;
    const ServerConfig& getServerConfig() const;
    const Router& getRouter() const;

    // called by CgiProcess when it has a result
    void onCgiFinished(const std::string& raw_cgi_output);

    // called by CgiProcess on timeout or error
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
    // reference to the server's loop_
    EventLoop& loop_;
    ServerResources resources_;

    size_t bytes_sent_;
    State state_;

    Request request_;
    Response response_;
    bool keep_alive_;
    Timeout timeout_;
    CgiProcess* pending_cgi_;  // non-owning ptr but Client controls lifetime
};

#endif
