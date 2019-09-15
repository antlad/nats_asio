#pragma once

#include <nats-asio/defs.hpp>
#include <nats-asio/common.hpp>

#include <boost/asio/ip/tcp.hpp>

namespace nats_asio {

struct options;

class client
{
public:
    client(const logger& m_log, aio& m_io);

    void connect(std::string_view address, uint16_t port, ctx c);

private:
    status process_message(boost::asio::ip::tcp::socket& socket, std::string_view v, ctx c);

    std::string prepare_info(const options &o);

    std::size_t m_max_payload;
    logger m_log;
    aio& m_io;
};

}



