#include <nats-asio/connection.hpp>
#include <nats-asio/structs.hpp>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <nlohmann/json.hpp>



namespace nats_asio {

struct subscription: public isubscription
{
    subscription(uint64_t sid, const on_message_cb& cb)
        : m_sid(sid)
        , m_cb(cb)
    {
    }
    virtual status unsubscribe() override
    {
        return {};
    }

    uint64_t m_sid;
    on_message_cb m_cb;
};


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

const std::map<std::string, mt, std::less<>> message_types_map {
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



//PUB <subject> [reply-to] <#bytes>\r\n[payload]\r\n

std::string connection::prepare_info(const options& o)
{
    constexpr auto connect_payload = "CONNECT {}\r\n";
    constexpr auto name = "nats-asio";
    constexpr auto lang = "cpp";
    constexpr auto version = "0.0.1";

    using nlohmann::json;
    json j = {
        {"verbose", o.verbose ? true : false},
        {"pedantic" , o.pedantic ? true : false},
        {"ssl_required" , o.ssl_required ? true : false},
        {"name" , name },
        {"lang", lang },
        {"user" , o.user},
        {"pass", o.pass },
        {"version", version},
        {"auth_token" , o.token}
    };
    auto info = j.dump();
    auto connect_data = fmt::format(connect_payload, info);
    m_log->debug("sending data on connect {}", info);
    return connect_data;
}

uint64_t connection::next_sid()
{
    return m_sid++;
}

status connection::process_subscription_message(std::string_view v, ctx c)
{
    std::vector<std::string> results;
    auto p = v.find("\r\n") - 1;
    auto info = v.substr(1, p);

    boost::split(results, info, [](char c){return c == ' ';});

    if (results.size() < 3 || results.size() > 4)
    {
        return status("unexpected message format");
    }
    bool replty_to = results.size() == 4;
    std::size_t bytes_id = replty_to ? 3 : 2;

    std::size_t sid = 0;
    std::size_t bytes_n = 0;
    try {
       sid = boost::lexical_cast<uint64_t>(results[1]);
       bytes_n = boost::lexical_cast<uint64_t>(results[bytes_id]);
    } catch (const std::exception& e) {
        return status(fmt::format("can't parse int in headers: {}", e.what()));
    }

    if (bytes_n > (v.size() - p - 2))
    {
        return status("unexpected bytes count");
    }

    auto it = m_subs.find(sid);
    if (it == m_subs.end())
    {
        m_log->trace("dropping message because subscription not found: topic: {}, sid: {}", results[0], results[1]);
        return {};
    }

    it->second->m_cb(results[0], &v[p + 3], bytes_n, c);

    return {};
}

status connection::process_message(std::string_view v, ctx c)
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
        m_log->trace("info recived and parsed");
        break;
    }
    case mt::MSG:{
        return process_subscription_message(v.substr(p,v.size() - p), c);
    }
    case mt::PING:{
        m_log->trace("ping recived");
        boost::system::error_code ec;
        m_socket.async_write_some(boost::asio::buffer(pong), c[ec]);
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
         m_log->trace("ok recived");
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

    return {};
}

connection::connection(const logger &log, aio &io)
    : m_sid(0)
    , m_log(log)
    , m_io(io)
    , m_socket(m_io)
{
}

status connection::connect(std::string_view address, uint16_t port, ctx c)
{
    boost::system::error_code ec;
    m_socket.async_connect(boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address(address), port), c[ec]);
    if (ec.failed()) return status("connect failed");
    boost::asio::spawn(m_io, std::bind(&connection::run, this, std::placeholders::_1));
    return {};
}


void connection::run(ctx c)
{
    std::string data;
    boost::asio::dynamic_string_buffer buf(data);

    boost::asio::async_read_until(m_socket, buf, "\r\n", c[ec]);

    if (ec.failed())
    {
        m_log->error("connect failed {}", ec.message());
        return;
    }

    auto s = process_message(data, c);
    if (s.failed())
    {
        m_log->error("process message failed with error: {}", s.error());
    }
    buf.consume(data.size());

    m_socket.async_write_some(boost::asio::buffer(ping), c[ec]);
    if (ec.failed())
    {
        m_log->error("failed to write pong {}", ec.message());
    }

    for(;;)
    {
        boost::asio::async_read_until(m_socket, buf, "\r\n", c[ec]);

        if (ec.failed())
        {
            m_log->error("connect failed {}", ec.message());
            return;
        }
        m_log->trace("read done");
        s = process_message(data, c);
        if (s.failed())
        {
            m_log->error("process message failed with error: {}", s.error());
        }
        buf.consume(data.size());
    }
}

status connection::publish(std::string_view subject, const char *raw, std::size_t n, std::optional<std::string_view> reply_to, ctx c)
{
    constexpr auto pub_header_payload = "PUB {} {} {}\r\n";

    return {};
}

//subscription_sptr connection::subscribe_queue(std::string_view subject, std::string_view queue, on_message_cb cb, ctx c)
//{
//    return {};
//}

iconnection_sptr create_connection(const logger& log, aio& io)
{
    return std::make_shared<connection>(log, io);
}


std::tuple<isubscription_sptr, status> connection::subscribe(std::string_view subject,  std::optional<std::string_view> queue, on_message_cb cb, ctx c)
{
    constexpr auto sub_payload = "SUB {} {} {}\r\n";
    auto sid = next_sid();
    std::string payload;
    if (queue.has_value())
    {
        payload = fmt::format(sub_payload, subject, "", sid);
    } else
    {
        payload = fmt::format(sub_payload, subject, queue.value(), sid);
    }


    m_socket.async_write_some(boost::asio::buffer(payload), c[ec]);
    if (ec.failed())
    {
        return {isubscription_sptr(), status(fmt::format("failed to subscribe {}", ec.message()))};
    }
    m_log->trace("subscribe sent: {}", payload);

    auto sub = std::make_shared<subscription>(sid, cb);
    m_subs.emplace(sid, sub);

    return {sub, {}};
}

}


