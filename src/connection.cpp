#include "connection.hpp"
#include "json.hpp"

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/read.hpp>

#include <utility>

namespace nats_asio {

struct subscription: public isubscription
{
    subscription(uint64_t sid, const on_message_cb& cb)
        : m_cancel(false)
        , m_cb(cb)
        , m_sid(sid)
    {
    }

    virtual void cancel() override;

    virtual uint64_t sid() override;

    bool m_cancel;
    on_message_cb m_cb;
    uint64_t m_sid;
};

std::string connection::prepare_info(const connect_config& o)
{
    constexpr auto connect_payload = "CONNECT {}\r\n";
    constexpr auto name = "nats_asio";
    constexpr auto lang = "cpp";
    constexpr auto version = "0.0.1";
    using nlohmann::json;
    json j =
    {
        {"verbose",      o.verbose},
        {"pedantic",     o.pedantic},
        {"ssl_required", o.ssl_required},
        {"name",             name },
        {"lang",             lang },
        {"version",          version},
    };

    if (o.user.has_value())
    {
        j["user"] = o.user.value();
    }

    if (o.password.has_value())
    {
        j["pass"] = o.password.value();
    }

    if (o.token.has_value())
    {
        j["auth_token"] = o.token.value();
    }

    auto info = j.dump();
    auto connect_data = fmt::format(connect_payload, info);
    m_log->debug("sending data on connect {}", info);
    return connect_data;
}

uint64_t connection::next_sid()
{
    return m_sid++;
}


connection::connection(const logger& log, aio& io, const on_connected_cb& connected_cb, const on_disconnected_cb& disconnected_cb)
    : m_sid(0)
    , m_log(log)
    , m_io(io)
    , m_is_connected(false)
    , m_stop_flag(false)
    , m_socket(m_io)
    , m_max_payload(0)
    , m_connected_cb(connected_cb)
    , m_disconnected_cb(disconnected_cb)
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

void connection::start(const connect_config& conf)
{
    boost::asio::spawn(m_io, std::bind(&connection::run, this, conf, std::placeholders::_1));
}

void connection::run(const connect_config& conf, ctx c)
{
    std::string header;

    for (;;)
    {
        if (m_stop_flag)
        {
            m_log->debug("stopping main connection loop");
            return;
        }

        if (!m_is_connected)
        {
            m_socket.async_connect(boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address(conf.address), conf.port), c[ec]);

            if (handle_error(c).failed())
            {
                continue;
            }

            boost::asio::async_read_until(m_socket, m_buf, "\r\n", c[ec]);
            auto s = handle_error(c);

            if (s.failed())
            {
                m_log->error("read server info failed {}", s.error());
                continue;
            }

            std::istream is(&m_buf);
            s = parse_header(header, is, this, c);

            if (s.failed())
            {
                m_log->error("process message failed with error: {}", s.error());
                continue;
            }

            auto info = prepare_info(conf);
            boost::asio::async_write(m_socket, boost::asio::buffer(info), boost::asio::transfer_exactly(info.size()),  c[ec]);
            s = handle_error(c);

            if (s.failed())
            {
                m_log->error("failed to write info {}", s.error());
                continue;
            }

            m_is_connected = true;

            if (m_connected_cb != nullptr)
            {
                m_connected_cb(*this, c);
            }
        }

        boost::asio::async_read_until(m_socket, m_buf, "\r\n", c[ec]);
        auto s = handle_error(c);

        if (s.failed())
        {
            m_log->error("failed to read {}", s.error());
            continue;
        }

        std::istream is(&m_buf);
        s = parse_header(header, is, this, c);

        if (s.failed())
        {
            m_log->error("process message failed with error: {}", s.error());
            continue;
        }
    }
}

status connection::handle_error(ctx c)
{
    if (ec.failed())
    {
        if ((ec == boost::asio::error::eof) || (boost::asio::error::connection_reset == ec))
        {
            m_is_connected = false;
            m_socket.close(ec);// TODO: handle it if error

            if (m_disconnected_cb != nullptr)
            {
                m_disconnected_cb(*this, std::move(c));
            }
        }

        return status(ec.message());
    }

    return {};
}

status connection::publish(string_view subject, const char* raw, std::size_t n, optional<string_view> reply_to, ctx c)
{
    if (!m_is_connected)
    {
        return status("not connected");
    }

    const std::string pub_header_payload("PUB {} {} {}\r\n");
    std::vector<boost::asio::const_buffer> buffers;
    std::string header;

    if (reply_to.has_value())
    {
        header = fmt::format(pub_header_payload, subject, reply_to.value(), n);
    }
    else
    {
        header = fmt::format(pub_header_payload, subject, "", n);
    }

    buffers.emplace_back(boost::asio::buffer(header.data(), header.size()));
    buffers.emplace_back(boost::asio::buffer(raw, n));
    buffers.emplace_back(boost::asio::buffer("\r\n", 2));
    std::size_t total_size = header.size() + n + 4;
    boost::asio::async_write(m_socket, buffers, boost::asio::transfer_exactly(total_size),  c[ec]);
    return  handle_error(c);
}

status connection::unsubscribe(const isubscription_sptr& p, ctx c)
{
    auto it = m_subs.find(p->sid());

    if (it == m_subs.end())
    {
        return status("subscription not found {}", p->sid());
    }

    m_subs.erase(it);
    const std::string unsub_payload("UNSUB {}\r\n");
    boost::asio::async_write(m_socket, boost::asio::buffer(unsub_payload), boost::asio::transfer_exactly(unsub_payload.size()),  c[ec]);
    return handle_error(c);
}

iconnection_sptr create_connection(const logger& log, aio& io, const on_connected_cb& connected_cb, const on_disconnected_cb& disconnected_cb)
{
    return std::make_shared<connection>(log, io, connected_cb, disconnected_cb);
}

std::pair<isubscription_sptr, status> connection::subscribe(string_view subject,  optional<string_view> queue, on_message_cb cb, ctx c)
{
    if (!m_is_connected)
    {
        return  {isubscription_sptr(), status("not connected")};
    }

    const std::string sub_payload("SUB {} {} {}\r\n");
    auto sid = next_sid();
    std::string payload;

    if (queue.has_value())
    {
        payload = fmt::format(sub_payload, subject, queue.value(), sid);
    }
    else
    {
        payload = fmt::format(sub_payload, subject, "", sid);
    }

    boost::asio::async_write(m_socket, boost::asio::buffer(payload), boost::asio::transfer_exactly(payload.size()),  c[ec]);
    auto s = handle_error(c);

    if (s.failed())
    {
        return {isubscription_sptr(), s};
    }

    auto sub = std::make_shared<subscription>(sid, cb);
    m_subs.emplace(sid, sub);
    return {sub, {}};
}

void connection::on_ping(ctx c)
{
    m_log->trace("ping recived");
    const std::string pong("PONG\r\n");
    boost::asio::async_write(m_socket, boost::asio::buffer(pong), boost::asio::transfer_exactly(pong.size()),  c[ec]);
    handle_error(c);
}

void connection::on_pong(ctx)
{
    m_log->trace("pong recived");
}

void connection::on_ok(ctx)
{
    m_log->trace("ok recived");
}

void connection::on_error(string_view err, ctx)
{
    m_log->error("error message from server {}", err);
}

void connection::on_info(string_view info, ctx)
{
    using nlohmann::json;
    auto j = json::parse(info);
    m_log->debug("got info {}", j.dump());
    m_max_payload = j["max_payload"].get<std::size_t>();
    m_log->trace("info recived and parsed");
}

void connection::on_message(string_view subject, string_view sid_str, optional<string_view> reply_to, std::size_t n, ctx c)
{
    int bytes_to_transsfer = int(n) + 2 - int(m_buf.size());

    if (bytes_to_transsfer > 0)
    {
        boost::asio::async_read(m_socket, m_buf, boost::asio::transfer_at_least(std::size_t(bytes_to_transsfer)), c[ec]);
    }

    auto s = handle_error(c);

    if (s.failed())
    {
        m_log->error("failed to read {}", s.error());
        return;
    }

    std::size_t sid = 0;

    try
    {
        sid = static_cast<std::size_t>(std::stoll(sid_str.data(), nullptr, 10));
    }
    catch (const std::exception& e)
    {
        m_log->error("can't parse sid: {}", e.what());
        return;
    }

    auto it = m_subs.find(sid);

    if (it == m_subs.end())
    {
        m_log->trace("dropping message because subscription not found: topic: {}, sid: {}", subject, sid_str);
        return;
    }

    if (it->second->m_cancel)
    {
        m_log->trace("subscribtion canceled {}", sid);
        s = unsubscribe(it->second, c);

        if (s.failed())
        {
            m_log->error("unsubscribe failed: {}", s.error());
        }
    }

    auto b = m_buf.data();

    if (reply_to.has_value())
    {
        it->second->m_cb(subject, reply_to, static_cast<const char*>(b.data()), n, c);
    }
    else
    {
        it->second->m_cb(subject, {}, static_cast<const char*>(b.data()), n, c);
    }
}

void connection::consumed(std::size_t n)
{
    m_buf.consume(n);
}

void subscription::cancel()
{
    m_cancel = true;
}

uint64_t subscription::sid()
{
    return m_sid;
}

}


