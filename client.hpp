#pragma once

#include "defs.hpp"

namespace nats_asio {

class client
{
public:
    client(const logger& l, aio& io);

    void connect(const std::string &address, uint16_t port, ctx c);

private:
    logger l;
    aio& io;
};

}



