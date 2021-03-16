/*
MIT License

Copyright (c) 2019 Vladislav Troinich

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
                                                              copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

       THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
        SOFTWARE.
*/

#pragma once

#include "nats_asio/interface.hpp"

#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/streambuf.hpp>

#include <nlohmann/json.hpp>

#include <boost/algorithm/string.hpp>

#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

template <> struct fmt::formatter<nats_asio::string_view> {
    template <typename ParseContext> constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

    template <typename FormatContext> auto format(const nats_asio::string_view& d, FormatContext& ctx) {
        return format_to(ctx.out(), "{}", d.data());
    }
};

namespace nats_asio {

using boost::asio::ip::tcp;
namespace ssl = boost::asio::ssl;

constexpr auto sep = "\r\n";

typedef boost::asio::ip::tcp::socket raw_socket;
typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_socket;

template <class Socket> auto& take_raw_ref(Socket&);

template <> auto& take_raw_ref(ssl_socket& s) { return s.next_layer(); }

template <> auto& take_raw_ref(raw_socket& s) { return s; }

template <class Socket> struct uni_socket {
    uni_socket(aio& io) : m_socket(io) {}

    uni_socket(aio& io, boost::asio::ssl::context& ctx) : m_socket(io, ctx) {}

    void async_connect(const boost::asio::ip::tcp::endpoint& endpoint, ctx c);

    void async_handshake(ctx c);

    template <class Buf> void async_read_until(Buf& buf, ctx c) {
        boost::asio::async_read_until(m_socket, buf, sep, c);
    }

    template <class Buf> void async_read_until_raw(Buf& buf, ctx c) {
        boost::asio::async_read_until(take_raw_ref(m_socket), buf, sep, c);
    }

    template <class Buf, class Transfer> void async_read(Buf& buf, const Transfer& until, ctx c) {
        boost::asio::async_read(m_socket, buf, until, c);
    }

    template <class Buf, class Transfer> void async_write(const Buf& buf, const Transfer& until, ctx c) {
        boost::asio::async_write(m_socket, buf, until, c);
    }

    void async_shutdown(ctx c);

    void close(boost::system::error_code& ec);

    Socket m_socket;
};

template <> void uni_socket<raw_socket>::close(boost::system::error_code& ec) { m_socket.close(ec); }

template <> void uni_socket<ssl_socket>::close(boost::system::error_code& ec) { m_socket.lowest_layer().close(ec); }

template <> void uni_socket<raw_socket>::async_handshake(ctx /*c*/) {}

template <> void uni_socket<ssl_socket>::async_handshake(ctx c) {
    m_socket.async_handshake(boost::asio::ssl::stream_base::client, c);
}

template <> void uni_socket<raw_socket>::async_shutdown(ctx /*c*/) {}

template <> void uni_socket<ssl_socket>::async_shutdown(ctx c) { m_socket.async_shutdown(c); }

template <> void uni_socket<raw_socket>::async_connect(const boost::asio::ip::tcp::endpoint& endpoint, ctx c) {
    m_socket.async_connect(endpoint, c);
}

template <> void uni_socket<ssl_socket>::async_connect(const boost::asio::ip::tcp::endpoint& endpoint, ctx c) {
    m_socket.lowest_layer().async_connect(endpoint, c);
}

struct parser_observer {
    virtual ~parser_observer() = default;

    virtual void on_ping(ctx c) = 0;

    virtual void on_pong(ctx c) = 0;

    virtual void on_ok(ctx c) = 0;

    virtual void on_error(string_view err, ctx c) = 0;

    virtual void on_info(string_view info, ctx c) = 0;

    virtual void on_message(string_view subject, string_view sid, optional<string_view> reply_to, std::size_t n,
                            ctx c) = 0;

    virtual void consumed(std::size_t n) = 0;
};

status parse_header(std::string& header, std::istream& is, parser_observer* observer, ctx c);

enum class mt { INFO, CONNECT, PUB, SUB, UNSUB, MSG, PING, PONG, OK, ERR };

const std::map<std::string, mt, std::less<>> message_types_map{
    {"INFO", mt::INFO}, {"CONNECT", mt::CONNECT}, {"PUB", mt::PUB},   {"SUB", mt::SUB}, {"UNSUB", mt::UNSUB},
    {"MSG", mt::MSG},   {"PING", mt::PING},       {"PONG", mt::PONG}, {"+OK", mt::OK},  {"-ERR", mt::ERR},
};

std::vector<string_view> split_sv(string_view str, string_view delims = " ") {
    std::vector<string_view> output;
    output.reserve(str.size() / 2);

    for (auto first = str.data(), second = str.data(), last = first + str.size(); second != last && first != last;
         first = second + 1) {
        second = std::find_first_of(first, last, std::cbegin(delims), std::cend(delims));

        if (first != second) {
            output.emplace_back(first, second - first);
        }
    }

    return output;
}

status parse_header(std::string& header, std::istream& is, parser_observer* observer, ctx c) {
    if (!std::getline(is, header)) {
        return {"can't get line"};
    }

    if (header.size() < 4) // TODO: maybe delte this check?
    {
        return {"too small header"};
    }

    if (header[header.size() - 1] != '\r') {
        return {"unexpected len of server message"};
    }

    header[header.size() - 1] = 0;
    header.resize(header.size() - 1);
    auto v = string_view(header);
    auto p = v.find_first_of(" ");

    if (p == string_view::npos) {
        if (header.size() != 4 && header.size() != 3) // ok or ping/pong
        {
            return {"protocol violation from server"};
        }

        p = header.size();
    }

    auto it = message_types_map.find(v.substr(0, p));

    if (it == message_types_map.end()) {
        return {"unknown message"};
    }

    switch (it->second) {
    case mt::INFO: {
        p += 1; // space
        auto info_msg = v.substr(p, v.size() - p);
        observer->on_info(info_msg, c);
        break;
    }

    case mt::MSG: {
        p += 1;
        auto info = v.substr(p, v.size() - p);
        auto results = split_sv(info, " ");

        if (results.size() < 3 || results.size() > 4) {
            return {"unexpected message format"};
        }

        bool replty_to = results.size() == 4;
        std::size_t bytes_id = replty_to ? 3 : 2;
        std::size_t bytes_n = 0;

        try {
            bytes_n = static_cast<std::size_t>(std::stoll(results[bytes_id].data(), nullptr, 10));
        } catch (const std::exception& e) {
            return {fmt::format("can't parse int in headers: {}", e.what())};
        }

        if (replty_to) {
            observer->on_message(results[0], results[1], results[2], bytes_n, c);
        } else {
            observer->on_message(results[0], results[1], optional<string_view>(), bytes_n, c);
        }

        observer->consumed(bytes_n + 2);
        return {};
    }

    case mt::PING: {
        observer->on_ping(c);
        break;
    }

    case mt::PONG: {
        observer->on_pong(c);
        break;
    }

    case mt::OK: {
        observer->on_ok(c);
        break;
    }

    case mt::ERR: {
        p += 1; // space
        auto err_msg = v.substr(p, v.size() - p);
        observer->on_error(err_msg, c);
        break;
    }

    default: {
        return {"unexpected message type"};
    }
    }

    return {};
}

struct subscription : public isubscription, private boost::asio::detail::noncopyable {
    subscription(uint64_t sid, const on_message_cb& cb);

    virtual void cancel() override;

    virtual uint64_t sid() override;

    bool m_cancel;
    on_message_cb m_cb;
    uint64_t m_sid;
};
typedef std::shared_ptr<subscription> subscription_sptr;

subscription::subscription(uint64_t sid, const on_message_cb& cb) : m_cancel(false), m_cb(cb), m_sid(sid) {}

void subscription::cancel() { m_cancel = true; }

uint64_t subscription::sid() { return m_sid; }

template <class SocketType>
class connection : public iconnection, public parser_observer, private boost::asio::detail::noncopyable {
public:
    connection(aio& io, const logger& log, const on_connected_cb& connected_cb,
               const on_disconnected_cb& disconnected_cb, const std::shared_ptr<ssl::context>& ctx);

    connection(aio& io, const logger& log, const on_connected_cb& connected_cb,
               const on_disconnected_cb& disconnected_cb);

    virtual void start(const connect_config& conf) override;

    virtual void stop() override { m_stop_flag = true; }

    virtual bool is_connected() override { return m_is_connected; }

    virtual status publish(string_view subject, const char* raw, std::size_t n, optional<string_view> reply_to,
                           ctx c) override;

    virtual status unsubscribe(const isubscription_sptr& p, ctx c) override;

    virtual std::pair<isubscription_sptr, status> subscribe(string_view subject, optional<string_view> queue,
                                                            on_message_cb cb, ctx c) override;

private:
    virtual void on_ping(ctx c) override;

    virtual void on_pong(ctx) override { m_log->trace("pong recived"); }

    virtual void on_ok(ctx) override { m_log->trace("ok recived"); }

    virtual void on_error(string_view err, ctx) override { m_log->error("error message from server {}", err); }

    virtual void on_info(string_view info, ctx c) override;

    virtual void on_message(string_view subject, string_view sid, optional<string_view> reply_to, std::size_t n,
                            ctx c) override;

    virtual void consumed(std::size_t n) override { m_buf.consume(n); }

    status do_connect(const connect_config& conf, ctx c);

    void run(const connect_config& conf, ctx c);

    status handle_error(ctx c);

    std::string prepare_info(const connect_config& o);

    uint64_t next_sid() { return m_sid++; }

    uint64_t m_sid;
    std::size_t m_max_payload;
    logger m_log;
    aio& m_io;

    bool m_is_connected;
    bool m_stop_flag;

    std::unordered_map<uint64_t, subscription_sptr> m_subs;
    on_connected_cb m_connected_cb;
    on_disconnected_cb m_disconnected_cb;
    boost::system::error_code ec;

    boost::asio::streambuf m_buf;

    std::shared_ptr<ssl::context> m_ssl_ctx;
    uni_socket<SocketType> m_socket;
};

void load_certificates(const ssl_config& conf, ssl::context& ctx) {
    ctx.set_options(ssl::context::tls_client);

    if (conf.ssl_verify) {
        ctx.set_verify_mode(ssl::verify_peer);
    } else {
        ctx.set_verify_mode(ssl::verify_none);
    }

    if (!conf.ssl_cert.empty()) {
        ctx.use_certificate(boost::asio::buffer(conf.ssl_cert.data(), conf.ssl_cert.size()),
                            ssl::context::file_format::pem);
    }

    if (!conf.ssl_ca.empty()) {
        ctx.add_certificate_authority(boost::asio::buffer(conf.ssl_ca.data(), conf.ssl_ca.size()));
    }

    if (!conf.ssl_dh.empty()) {
        ctx.use_tmp_dh_file(conf.ssl_dh);
    }

    if (!conf.ssl_key.empty()) {
        ctx.use_private_key(boost::asio::buffer(conf.ssl_key.data(), conf.ssl_key.size()),
                            ssl::context::file_format::pem);
    }
}

iconnection_sptr create_connection(aio& io, const logger& log, const on_connected_cb& connected_cb,
                                   const on_disconnected_cb& disconnected_cb, optional<ssl_config> ssl_conf) {
    if (ssl_conf.has_value()) {
        auto ssl_ctx = std::make_shared<ssl::context>(ssl::context::tlsv12_client);
        load_certificates(ssl_conf.value(), *ssl_ctx);
        return std::make_shared<connection<ssl_socket>>(io, log, connected_cb, disconnected_cb, ssl_ctx);
    } else {
        return std::make_shared<connection<raw_socket>>(io, log, connected_cb, disconnected_cb);
    }
}

template <class SocketType>
connection<SocketType>::connection(aio& io, const logger& log, const on_connected_cb& connected_cb,
                                   const on_disconnected_cb& disconnected_cb, const std::shared_ptr<ssl::context>& ctx)
    : m_sid(0), m_max_payload(0), m_log(log), m_io(io), m_is_connected(false), m_stop_flag(false),
      m_connected_cb(connected_cb), m_disconnected_cb(disconnected_cb), m_ssl_ctx(ctx), m_socket(io, *ctx.get()) {}

template <class SocketType>
connection<SocketType>::connection(aio& io, const logger& log, const on_connected_cb& connected_cb,
                                   const on_disconnected_cb& disconnected_cb)
    : m_sid(0), m_max_payload(0), m_log(log), m_io(io), m_is_connected(false), m_stop_flag(false),
      m_connected_cb(connected_cb), m_disconnected_cb(disconnected_cb), m_socket(io) {}

template <class SocketType> void connection<SocketType>::start(const connect_config& conf) {
    boost::asio::spawn(m_io, std::bind(&connection::run, this, conf, std::placeholders::_1));
}

template <class SocketType>
status connection<SocketType>::publish(string_view subject, const char* raw, std::size_t n,
                                       optional<string_view> reply_to, ctx c) {
    if (!m_is_connected) {
        return status("not connected");
    }

    const std::string pub_header_payload("PUB {} {} {}\r\n");
    std::vector<boost::asio::const_buffer> buffers;
    std::string header;

    if (reply_to.has_value()) {
        header = fmt::format(pub_header_payload, subject, reply_to.value(), n);
    } else {
        header = fmt::format(pub_header_payload, subject, "", n);
    }

    buffers.emplace_back(boost::asio::buffer(header.data(), header.size()));
    buffers.emplace_back(boost::asio::buffer(raw, n));
    buffers.emplace_back(boost::asio::buffer("\r\n", 2));
    std::size_t total_size = header.size() + n + 2;
    m_socket.async_write(buffers, boost::asio::transfer_exactly(total_size), c[ec]);
    return handle_error(c);
}

template <class SocketType> status connection<SocketType>::unsubscribe(const isubscription_sptr& p, ctx c) {
    auto sid = p->sid();
    auto it = m_subs.find(sid);

    if (it == m_subs.end()) {
        return status(fmt::format("subscription not found {}", sid));
    }
    m_subs.erase(it);

    std::string unsub_payload(fmt::format("UNSUB {}\r\n", sid));
    m_socket.async_write(boost::asio::buffer(unsub_payload), boost::asio::transfer_exactly(unsub_payload.size()),
                         c[ec]);
    return handle_error(c);
}

template <class SocketType>
std::pair<isubscription_sptr, status>
connection<SocketType>::subscribe(string_view subject, optional<string_view> queue, on_message_cb cb, ctx c) {
    if (!m_is_connected) {
        return {isubscription_sptr(), status("not connected")};
    }

    const std::string sub_payload("SUB {} {} {}\r\n");
    auto sid = next_sid();
    std::string payload;

    if (queue.has_value()) {
        payload = fmt::format(sub_payload, subject, queue.value(), sid);
    } else {
        payload = fmt::format(sub_payload, subject, "", sid);
    }

    m_socket.async_write(boost::asio::buffer(payload), boost::asio::transfer_exactly(payload.size()), c[ec]);
    auto s = handle_error(c);

    if (s.failed()) {
        return {isubscription_sptr(), s};
    }

    auto sub = std::make_shared<subscription>(sid, cb);
    m_subs.emplace(sid, sub);
    return {sub, {}};
}

template <class SocketType> void connection<SocketType>::on_ping(ctx c) {
    m_log->trace("ping recived");
    const std::string pong("PONG\r\n");
    m_socket.async_write(boost::asio::buffer(pong), boost::asio::transfer_exactly(pong.size()), c[ec]);
    handle_error(c);
}

template <class SocketType> void connection<SocketType>::on_info(string_view info, ctx) {
    using nlohmann::json;
    auto j = json::parse(info);
    m_log->debug("got info {}", j.dump());
    m_max_payload = j["max_payload"].get<std::size_t>();
    m_log->trace("info recived and parsed");
}

template <class SocketType>
void connection<SocketType>::on_message(string_view subject, string_view sid_str, optional<string_view> reply_to,
                                        std::size_t n, ctx c) {
    int bytes_to_transsfer = int(n) + 2 - int(m_buf.size());

    if (bytes_to_transsfer > 0) {
        m_socket.async_read(m_buf, boost::asio::transfer_at_least(std::size_t(bytes_to_transsfer)), c[ec]);
    }

    auto s = handle_error(c);

    if (s.failed()) {
        m_log->error("failed to read {}", s.error());
        return;
    }

    std::size_t sid_u = 0;

    try {
        sid_u = static_cast<std::size_t>(std::stoll(sid_str.data(), nullptr, 10));
    } catch (const std::exception& e) {
        m_log->error("can't parse sid: {}", e.what());
        return;
    }

    auto it = m_subs.find(sid_u);

    if (it == m_subs.end()) {
        m_log->trace("dropping message because subscription not found: topic: {}, sid: {}", subject, sid_str);
        return;
    }

    if (it->second->m_cancel) {
        m_log->trace("subscribtion canceled {}", sid_str);
        s = unsubscribe(it->second, c);

        if (s.failed()) {
            m_log->error("unsubscribe failed: {}", s.error());
        }
    }

    auto b = m_buf.data();

    if (reply_to.has_value()) {
        it->second->m_cb(subject, reply_to, static_cast<const char*>(b.data()), n, c);
    } else {
        it->second->m_cb(subject, {}, static_cast<const char*>(b.data()), n, c);
    }
}

template <class SocketType> status connection<SocketType>::do_connect(const connect_config& conf, ctx c) {
    tcp::resolver res(m_io);
    auto it = res.async_resolve(tcp::resolver::query(conf.address, std::to_string(conf.port)), c[ec]);
    auto s = handle_error(c);

    if (s.failed()) {
        m_log->error("async resolve of {}:{} failed with error: {}", conf.address, conf.port, s.error());
        return s;
    }

    // TODO: how to get end here?
    m_socket.async_connect((*it).endpoint(), c[ec]);
    s = handle_error(c);

    if (s.failed()) {
        return s;
    }

    m_socket.async_read_until_raw(m_buf, c[ec]);
    s = handle_error(c);

    if (s.failed()) {
        m_log->error("read server info failed {}", s.error());
        return s;
    }

    std::string header;
    std::istream is(&m_buf);
    s = parse_header(header, is, this, c);

    if (s.failed()) {
        m_log->error("process message failed with error: {}", s.error());
        return s;
    }

    m_socket.async_handshake(c[ec]);
    s = handle_error(c);

    if (s.failed()) {
        m_log->error("async handshake failed {}", s.error());
        return s;
    }

    auto info = prepare_info(conf);
    m_socket.async_write(boost::asio::buffer(info), boost::asio::transfer_exactly(info.size()), c[ec]);
    s = handle_error(c);

    if (s.failed()) {
        m_log->error("failed to write info {}", s.error());
        return s;
    }

    return {};
}

template <class SocketType> void connection<SocketType>::run(const connect_config& conf, ctx c) {
    std::string header;

    for (;;) {
        if (m_stop_flag) {
            m_log->debug("stopping main connection loop");
            return;
        }

        if (!m_is_connected) {
            // TODO: make sleep if failed
            auto s = do_connect(conf, c);

            if (s.failed()) {
                m_log->error("connect failed with error {}", s.error());
                continue;
            }

            m_is_connected = true;

            if (m_connected_cb != nullptr) {
                m_connected_cb(*this, c);
            }
        }

        m_socket.async_read_until(m_buf, c[ec]);
        auto s = handle_error(c);

        if (s.failed()) {
            m_log->error("failed to read {}", s.error());
            continue;
        }

        std::istream is(&m_buf);
        s = parse_header(header, is, this, c);

        if (s.failed()) {
            m_log->error("process message failed with error: {}", s.error());
            continue;
        }
    }
}

template <class SocketType> status connection<SocketType>::handle_error(ctx c) {
    if (ec.failed()) {
        auto original_msg = ec.message();
        m_is_connected = false;
        m_socket.close(ec); // TODO: handle it if error

        if (ec.failed()) {
            m_log->error("error on socket close {}", ec.message());
        }

        if (m_disconnected_cb != nullptr) {
            m_disconnected_cb(*this, std::move(c));
        }

        return status(original_msg);
    }

    return {};
}

template <class SocketType> std::string connection<SocketType>::prepare_info(const connect_config& o) {
    constexpr auto connect_payload = "CONNECT {}\r\n";
    constexpr auto name = "nats_asio";
    constexpr auto lang = "cpp";
    constexpr auto version = "0.0.1";
    using nlohmann::json;
    json j = {
        {"verbose", o.verbose}, {"pedantic", o.pedantic}, {"name", name}, {"lang", lang}, {"version", version},
    };

    if (o.user.has_value()) {
        j["user"] = o.user.value();
    }

    if (o.password.has_value()) {
        j["pass"] = o.password.value();
    }

    if (o.token.has_value()) {
        j["auth_token"] = o.token.value();
    }

    auto info = j.dump();
    auto connect_data = fmt::format(connect_payload, info);
    m_log->debug("sending data on connect {}", info);
    return connect_data;
}

} // namespace nats_asio
