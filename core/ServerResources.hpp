#ifndef SERVERRESOURCES_HPP
#define SERVERRESOURCES_HPP

#include <map>
#include <string>

#include "../config/Config.hpp"
#include "../handlers/Router.hpp"
#include "../logger/Logger.hpp"

/**
 * @brief Holds per-server runtime resources.
 *
 * Encapsulates routing logic and session storage for a single
 * server configuration instance.
 *
 * @note Router is built from ServerConfig and is immutable
 * after construction.
 * @note Session storage is shared across all clients using
 * this ServerResources instance.
 */
class ServerResources {
public:
    ServerResources(const ServerConfig& server_config);

    ServerResources(const ServerResources& other);
    ~ServerResources();

    const Router& getRouter() const;
    const ServerConfig& getServerConfig() const;

    // session management methods
    void createSession(const std::string& id);  // insert an empty session entry
                                                // for the given session ID
    bool hasSession(
        const std::string& id) const;  // returns true if a session with this ID
                                       // already exists in sessions_
    std::map<std::string, std::string>& getOrCreateSession(
        const std::string&
            id);  // returns a reference to the session data map
                  // for the given ID, inserting an empty entry if absent

private:
    ServerResources& operator=(const ServerResources&);

    ServerConfig server_config_;
    Router router_;
    // Maps a session_id to its session data, ex:
    // [id_a3f9] => {"username":"bob", "logged_in":"true", ...}
    std::map<std::string, std::map<std::string, std::string> > sessions_;
};

#endif
