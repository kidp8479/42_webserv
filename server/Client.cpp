#include "Client.hpp"

#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <string>

#include "../logger/Logger.hpp"
/**
 * @brief Constructs a Client from a socket fd.
 *
 * Takes ownership of the file descriptor and initializes the
 * client in the reading state with a fresh request/response context.
 *
 * @param[in] fd Connected socket file descriptor
 */
Client::Client(int fd) : fd_(fd), bytes_sent_(0), state_(kReading) {
    LOG_DEBUG() << "fd: " << fd_.getFd() << ", bytes sent: " << bytes_sent_
                << ", state: " << state_;
    // request_ object default constructed automatically
    // response_ object also default constructed automatically
}

/**
 * @brief Destroys the Client.
 *
 * Releases owned resources (socket managed by Fd).
 */
Client::~Client() {
}

/**
 * @brief Returns the client socket file descriptor.
 *
 * @return The underlying socket fd
 */
int Client::getFd() const {
    return fd_.getFd();
}
/**
 * @brief Returns the current client state.
 *
 * @return Current state (reading, writing, or done)
 */
Client::State Client::getState() const {
    return state_;
}

/**
 * @brief Provides access to the request object.
 *
 * @return Reference to the associated Request
 */
Request& Client::getRequest() {
    return request_;
}

/**
 * @brief Provides access to the response object.
 *
 * @return Reference to the associated Response
 */
Response& Client::getResponse() {
    return response_;
}

// the client request comes in as a stream of raw data, that can come
// in partial chunks or split messages - these raw data must be parsed
// and reconstructed at the HTTP layer (Charlie's part). My job
// here is just to store the raw data
// NOTE: at this point the client handles a single request only
// we will handle multiple requests for a single client in the next PR

/**
 * @brief Reads incoming data from the client socket.
 *
 * Appends received bytes to the request buffer and checks
 * whether the HTTP request is complete.
 *
 * This implementation currently supports a single HTTP request
 * per client connection. Persistent connections and multiple
 * pipelined requests will be handled in a later iteration.
 *
 * Transitions:
 * - kReading -> kWriting when request is complete
 * - kReading -> kDone on disconnect or error
 *
 * @return ReadResult indicating progress or termination
 *
 * @note Handles partial reads; request data may arrive in chunks
 * @see Request::isComplete()
 */
Client::ReadResult Client::read() {
    char buffer[kBufferSize];
    ssize_t bytes = recv(fd_.getFd(), buffer, sizeof(buffer), 0);
    LOG_DEBUG() << "Received " << bytes << " bytes from fd " << fd_.getFd();

    if (bytes > 0) {
        // i store the request directly in request object raw_ for Charlie
        request_.append(buffer, bytes);
        // we use Charlie's isComplete to check if the request was compelte
        // isComplete when "\r\n\r\n" is found (+ full body if Content-Length is
        // set)
        if (request_.isComplete()) {
            state_ = kWriting;
            LOG_DEBUG() << "Client fd " << fd_.getFd()
                        << " switching to WRITING state";
            return kReadComplete;
        }
        return kReadPending;
    } else if (bytes == 0) {  // connection finished, stream closed
        state_ = kDone;
        return kReadClosed;  // clean disconnect, not error
    } else {
        state_ = kDone;
        LOG_INFO() << "Client fd " << fd_.getFd() << " disconnected or error";
        return kReadError;
    }
}

/**
 * @brief Sends the HTTP response to the client socket.
 *
 * The response is sent incrementally because send() may write only
 * part of the buffer in non-blocking mode.
 *
 * @note bytes_sent_ tracks partial writes across multiple calls.
 *
 * @return WriteResult indicating:
 * - kWritePending: response partially sent
 * - kWriteDone: response fully sent
 * - kWriteError: socket failure or disconnect
 *
 * @warning Any send() failure is currently treated as a closed
 * connection (pre-polling limitation).
 */
Client::WriteResult Client::write() {
    const std::string response = response_.getRaw();  // copy safe for CGI later

    if (bytes_sent_ >= response.size()) {
        state_ = kDone;
        return kWriteDone;
    }
    // send() may write only part of the data, so we resume from bytes already
    // sent
    ssize_t sent = send(fd_.getFd(), response.c_str() + bytes_sent_,
                        response.size() - bytes_sent_, 0);
    if (sent > 0) {
        // track how many bytes have been successfully sent so far
        bytes_sent_ += static_cast<size_t>(sent);
        // if entire response has been sent, mark client as done
        if (bytes_sent_ >= response.size()) {
            state_ = kDone;
            return kWriteDone;
        }
        return kWritePending;
    } else {
        // w/o polling, treat any failure as a closed connection
        state_ = kDone;
        return kWriteError;
    }
}
