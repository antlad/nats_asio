#include <nats-asio/client.hpp>
#include <nats-asio/common.hpp>
#include <nats-asio/connection.hpp>

#include <boost/unordered_map.hpp>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/read_until.hpp>

#include <nlohmann/json.hpp>


namespace nats_asio {

//using boost::asio::ip::tcp;

//client::client(const logger &l, aio& io)
//    : m_log(l)
//    , m_io(io)
//{
//}

//std::tuple<connection_sptr, status> client::connect(std::string_view address, uint16_t port, ctx c)
//{
//    auto socket = std::make_shared<tcp::socket>(m_io);
//    boost::system::error_code ec;
//    socket->async_connect(boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address(address), port), c[ec]);
//    if (ec.failed())
//    {
//        std::make_tuple(connection_sptr(),status("connect failed"));
//    }
//    auto con = std::make_shared<connection>(socket, m_log, m_io);
//    boost::asio::spawn(m_io, std::bind(&connection::run, con.get(), std::placeholders::_1));
//    return {con, {}};
//}


/*

INFO 	Server 	Sent to client after initial TCP/IP connection
        CONNECT 	Client 	Sent to server to specify connection information
            PUB 	Client 	Publish a message to a subject, with optional reply subject
            SUB 	Client 	Subscribe to a subject (or subject wildcard)
                UNSUB 	Client 	Unsubscribe (or auto-unsubscribe) from subject
        MSG 	Server 	Delivers a message payload to a subscriber
        PING 	Both 	PING keep-alive message
        PONG 	Both 	PONG keep-alive response
        +OK 	Server 	Acknowledges well-formed protocol message in verbose mode
        -ERR 	Server 	Indicates a protocol error. May cause client disconnect.
*/

//"client_id":5,"go":"go1.11.12","host":"0.0.0.0","max_payload":1048576,"port":4222,"proto":1,"server_id":"NDXSA3VQMTDHJBC2VN2LO5FZ2IXJATCZAK7DFZPDD2ISKPGIJP3JUSDA","version":"2.0.2"
/*

void client::connect(std::string_view address, uint16_t port, ctx c)
{
    std::string data;
    boost::asio::dynamic_string_buffer buf(data);

    tcp::socket socket(m_io);
    boost::system::error_code ec;
    socket.async_connect(boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address(address), port), c[ec]);
    if (ec.failed())
    {
        m_log->error("connect failed");
        return;
    }

    boost::asio::async_read_until(socket, buf, "\r\n", c[ec]);

    if (ec.failed())
    {
        m_log->error("connect failed {}", ec.message());
        return;
    }


    auto s = process_message(socket, data, c);
    if (s.failed())
    {
        m_log->error("process message failed with error: {}", s.error());
    }
    buf.consume(data.size());

    socket.async_write_some(boost::asio::buffer(ping), c[ec]);
    if (ec.failed())
    {
        m_log->error("failed to write pong {}", ec.message());
    }

    for(;;)
    {
        boost::asio::async_read_until(socket, buf, "\r\n", c[ec]);

        if (ec.failed())
        {
            m_log->error("connect failed {}", ec.message());
            return;
        }
        m_log->trace("read done");
        s = process_message(socket, data, c);
        if (s.failed())
        {
            m_log->error("process message failed with error: {}", s.error());
        }
        buf.consume(data.size());


    }


    //std::shared_ptr<std::vector<char> > buffer_;
}
*/
//void client::publish(std::string_view subject, const char *raw, std::size_t n, std::optional<std::string> reply_to)
//{

//}






}

