#ifndef HANDLER_HPP
#define HANDLER_HPP

#include "../http/Request.hpp"
#include "../http/Response.hpp"
#include "../config/LocationConfig.hpp"

class Handler {
public:
    static void run(const Request& request,
                    const LocationConfig& loc,
                    Response& response);
};

#endif
