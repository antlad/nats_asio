#include "client.hpp"

//#include <boost/asio/co_spawn.hpp>
//#include <boost/asio/detached.hpp>
//#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
//#include <boost/asio/signal_set.hpp>
//#include <boost/asio/write.hpp>

using boost::asio::ip::tcp;

//using boost::asio::awaitable;
//using boost::asio::co_spawn;
//using boost::asio::detached;
//using boost::asio::use_awaitable;
//namespace this_coro = boost::asio::this_coro;

namespace nats_asio {

client::client(const logger &l, aio& io)
    : l(l)
    , io(io)
{

}

void client::connect(const std::string& address, uint16_t port, ctx c)
{
    std::array<char, 128> data;
    //std::string data
    auto socket = std::make_shared<tcp::socket>(io);
    boost::system::error_code ec;
    socket->async_connect(boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address(address), port), c[ec]);
    if (ec.failed())
    {
        l->error("connect failed");
        return;
    }

    for(;;)
    {
        socket->async_read_some(boost::asio::buffer(data), c[ec]);
        if (ec.failed())
        {
            l->error("connect failed");
            return;
        }

        l->debug("read done {}", std::string_view(&data[0]));
    }


    //std::shared_ptr<std::vector<char> > buffer_;
}

}

