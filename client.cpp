#include <nats-asio/client.hpp>
#include <nats-asio/common.hpp>


//#include <boost/asio/co_spawn.hpp>
//#include <boost/asio/detached.hpp>
//#include <boost/asio/io_context.hpp>
#include <boost/unordered_map.hpp>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/read_until.hpp>
//#include <boost/asio/signal_set.hpp>
//#include <boost/asio/write.hpp>
#include <nlohmann/json.hpp>

using boost::asio::ip::tcp;

//using boost::asio::awaitable;
//using boost::asio::co_spawn;
//using boost::asio::detached;
//using boost::asio::use_awaitable;
//namespace this_coro = boost::asio::this_coro;

namespace nats_asio {

static const std::string name = "nats-asio";
static const std::string lang = "cpp";
static const std::string version = "0.0.1";



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


client::client(const logger &l, aio& io)
    : m_log(l)
    , m_io(io)
{
}


enum class mt {
    INFO,
    CONNECT,
    PUB,
    SUB,
    UNSUB,
    MSG,
    PING,
    PONG,
    OK,
    ERR
};


static const std::map<std::string, mt, std::less<>> message_types_map {
    {"INFO", mt::INFO},
    {"CONNECT", mt::CONNECT},
    {"PUB", mt::PUB},
    {"SUB", mt::SUB},
    {"UNSUB", mt::UNSUB},
    {"MSG", mt::MSG},
    {"PING", mt::PING},
    {"PONG", mt::PONG},
    {"+OK", mt::OK},
    {"-ERR", mt::ERR},
};

const std::string rn("\r\n");
const std::string ping("PING\r\n");
const std::string pong("PONG\r\n");
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

status client::process_message(tcp::socket& socket, std::string_view v, ctx c)
{

    auto p = v.find_first_of(" \r\n");
    if (p == std::string_view   ::npos){
        return status("protocol violation from server");
    }

    auto it = message_types_map.find(v.substr(0, p));
    if (it == message_types_map.end())
    {
        return status("unknown message");
    }
    switch (it->second)
    {
    case mt::INFO:{
        using nlohmann::json;
        auto j = json::parse(v.substr(p,v.size() - p));
        m_log->debug("got info {}", j.dump());
        m_max_payload = j["max_payload"].get<std::size_t>();
        break;
    }
    case mt::MSG:{
        break;
    }
    case mt::PING:{
        m_log->trace("ping recived");
        boost::system::error_code ec;
        socket.async_write_some(boost::asio::buffer(pong), c[ec]);
        if (ec.failed())
        {
            return status(fmt::format("failed to write pong {}", ec.message()));
        }
        m_log->trace("pong sent");
        break;
    }
    case mt::PONG:{
         m_log->trace("pong recived");
        break;
    }
    case mt::OK:{
        break;
    }
    case mt::ERR:{
        m_log->error("error message from server {}", v.substr(p,v.size() - p));
        break;
    }
    default:{
        return status("unexpected message type");
    }
    }

//    message_types_map.find()

//    using nlohmann::json;
//    auto j = json::parse(data);

//    l->debug("got info {}", j.dump());

//    options o;
//    auto info = prepare_info(o);

//    socket.async_write_some(boost::asio::buffer(info), c[ec]);
//    if (ec.failed())
//    {

//        l->error("failed to write info {}", ec.message());
//        return;
//    }
//    l->info("on connect info sent");
    return {};
}

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





std::string client::prepare_info(const options& o)
{

    using nlohmann::json;
    json j = {
        {"verbose", o.verbose ? true : false},
        {"pedantic" , o.pedantic ? true : false},
        {"ssl_required" , o.ssl_required ? true : false},
        {"name" , name },
        {"lang", lang },
        {"user" , o.user},
        {"pass", o.pass },
        {"auth_token" , o.token}
    };
    auto info = j.dump();
    auto connect_data = fmt::format("CONNECT {} \r\n", info);
    m_log->debug("sending data on connect {}", info);
    return connect_data;
}

}

