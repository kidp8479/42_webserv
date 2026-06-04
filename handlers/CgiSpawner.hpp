#ifndef CGISPAWNER_HPP
#define CGISPAWNER_HPP

#include <string>
#include <vector>

#include "../core/EventLoop.hpp"
#include "Handler.hpp"

/**
 * @brief CGI execution manager.
 *
 * Responsible for validating CGI requests, preparing execution
 * environment, and spawning CGI processes with proper pipe setup.
 *
 * @note Uses EventLoop to register stdin/stdout handlers for async CGI.
 */
class CgiSpawner {
public:
    explicit CgiSpawner(EventLoop& loop);
    ~CgiSpawner();

    bool spawn(const Request& request, const LocationConfig& location,
               Client& client);

private:
    CgiSpawner(const CgiSpawner&);
    CgiSpawner& operator=(const CgiSpawner&);

    bool createPipes(int stdin_pipe[2], int stdout_pipe[2], bool has_body);
    bool validateScript(const Request& request, const LocationConfig& config,
                        Client& client, std::string& out_script_path);
    std::string resolveInterpreter(const Request& request,
                                   const LocationConfig& location);
    std::string buildScriptPath(const Request& request,
                                const LocationConfig& config);
    std::vector<std::string> buildEnvStrings(const Request& request,
                                             const std::string& script_path,
                                             const Client& client);
    std::vector<char*> buildEnvp(const std::vector<std::string>& env_strings);

    EventLoop& loop_;
};

#endif
