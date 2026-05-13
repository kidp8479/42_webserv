#ifndef HANDLER_HPP
#define HANDLER_HPP

#include "../config/LocationConfig.hpp"
#include "../http/Request.hpp"
#include "../http/Response.hpp"

class Handler {
public:
    static void run(const Request& request, const LocationConfig& loc,
                    Response& response);
};

#endif
