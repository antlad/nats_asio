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
        : m_cancel(false)
        , m_cb(cb)
        , m_sid(sid)
    {
    }

    virtual void cancel() override
    {
        m_cancel = true;
    }

    virtual uint64_t sid() override
    {
        return m_sid;
    }

    bool m_cancel;
    on_message_cb m_cb;
    uint64_t m_sid;
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
        return status("can't parse int in headers: {}", e.what());
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

    if (it->second->m_cancel)
    {
        m_log->trace("subscribtion canceled {}", sid);
        return unsubscribe(it->second, c);
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

        m_socket.async_write_some(boost::asio::buffer("PONG\r\n"), c[ec]);
        if (auto s = handle_error(); s.failed()) return s;
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
    , m_is_connected(false)
    , m_stop_flag(false)
    , m_socket(m_io)
{
}

void connection::stop()
{
    m_stop_flag = true;
}

bool connection::is_connected()
{
    return m_is_connected;
}

void connection::start(std::string_view address, uint16_t port)
{
    boost::asio::spawn(m_io, std::bind(&connection::run, this, address, port, std::placeholders::_1));
}

void connection::run(std::string_view address, uint16_t port, ctx c)
{
    std::string data;
    boost::asio::dynamic_string_buffer buf(data);

    for(;;)
    {
        if (m_stop_flag)
        {
            m_log->debug("stopping main connection loop");
            return;
        }

        if (!m_is_connected)
        {
            m_socket.async_connect(boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address(address), port), c[ec]);
            if (handle_error().failed())
            {
                continue;
            }
            boost::asio::async_read_until(m_socket, buf, "\r\n", c[ec]);
            if (auto s = handle_error(); s.failed())
            {
                m_log->error("read server info failed {}", s.error());
                continue;
            }

            auto s = process_message(data, c);
            if (s.failed())
            {
                m_log->error("process message failed with error: {}", s.error());
            }
            buf.consume(data.size());
            options o;
            auto info = prepare_info(o);
            m_socket.async_write_some(boost::asio::buffer(info), c[ec]);
            if (auto s = handle_error(); s.failed())
            {
                m_log->error("failed to write info {}", s.error());
                continue;
            }
        }

        boost::asio::async_read_until(m_socket, buf, "\r\n", c[ec]);
        if (auto s = handle_error(); s .failed())
        {
            m_log->error("failed to read {}", s.error());
            continue;
        }

        m_log->trace("read done");
        auto s = process_message(data, c);
        if (s.failed())
        {
            m_log->error("process message failed with error: {}", s.error());
        }
        buf.consume(data.size());
    }
}

status connection::handle_error()
{
    if (ec.failed())
    {
        if ((ec == boost::asio::error::eof) || (boost::asio::error::connection_reset == ec))
        {
            m_is_connected = false;
            m_socket.close();
        }
        return status(ec.message());
    }

    return {};
}

status connection::publish(std::string_view subject, const char *raw, std::size_t n, std::optional<std::string_view> reply_to, ctx c)
{
    constexpr auto pub_header_payload = "PUB {} {} {}\r\n";
    std::string header;
    if (reply_to.has_value())
    {
        header = fmt::format(pub_header_payload, subject, reply_to.value(), n);

    } else
    {
        header = fmt::format(pub_header_payload, subject, "", n);
    }

    m_socket.async_write_some(boost::asio::buffer(header), c[ec]);
    if (auto s = handle_error(); s.failed()) return s;

    m_socket.async_write_some(boost::asio::buffer(raw, n), c[ec]);
    if (auto s = handle_error(); s.failed()) return s;

    m_socket.async_write_some(boost::asio::buffer("\r\n"), c[ec]);
    if (auto s = handle_error(); s.failed()) return s;

    return {};
}

status connection::unsubscribe(const isubscription_sptr &p, ctx c)
{
    auto it = m_subs.find(p->sid());
    if (it == m_subs.end())
    {
        return status("subscription not found {}", p->sid());
    }
    m_subs.erase(it);

    constexpr auto unsub_payload = "UNSUB {}\r\n";
    m_socket.async_write_some(boost::asio::buffer(fmt::format(unsub_payload, p->sid())), c[ec]);
    if (auto s = handle_error(); s.failed()) return s;
    return {};
}

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
        payload = fmt::format(sub_payload, subject, queue.value(), sid);

    } else
    {
        payload = fmt::format(sub_payload, subject, "", sid);
    }

    m_socket.async_write_some(boost::asio::buffer(payload), c[ec]);
    if (auto s = handle_error(); s.failed())
    {
        return {isubscription_sptr(), s};
    }
    m_log->trace("subscribe sent: {}", payload);

    auto sub = std::make_shared<subscription>(sid, cb);
    m_subs.emplace(sid, sub);

    return {sub, {}};
}

}


