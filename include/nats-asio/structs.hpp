#pragma once

#include <string>

namespace nats_asio {

struct options
{
    bool verbose = false;
    bool pedantic = false;
    bool ssl_required = false;
    bool ssl = false;
    bool ssl_verify = true;
    std::string ssl_key;
    std::string ssl_cert;
    std::string ssl_ca;

    std::string user;
    std::string pass;
    std::string token;
};

}



