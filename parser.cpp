#include <nats-asio/parser.hpp>

#include <map>

namespace nats_asio {

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

const std::map<std::string, mt, std::less<>> message_types_map
{
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

constexpr std::string_view pong = "PONG\r\n";
constexpr std::string_view ping = "PING\r\n";
constexpr std::string_view ok = "+OK\r\n";

std::tuple<std::size_t, status> parse_message(std::string_view v, parser_observer *observer)
{
    auto p = v.find_first_of(" \r\n");
    auto end_p = v.find("\r\n", p);
    if (p == std::string_view::npos || end_p == std::string_view::npos)
    {
        return {0, {"protocol violation from server"}};
    }

    auto it = message_types_map.find(v.substr(0, p));
    if (it == message_types_map.end())
    {
        return {0, {"unknown message"}};
    }
    switch (it->second)
    {
    case mt::INFO:{
        p += 1;//space
        auto info_msg = v.substr(p, end_p - p);
        observer->on_info(info_msg);
//        using nlohmann::json;
//        auto j = json::parse(v.substr(p,v.size() - p));
//        m_log->debug("got info {}", j.dump());
//        m_max_payload = j["max_payload"].get<std::size_t>();
//        m_log->trace("info recived and parsed");
        break;
    }
    case mt::MSG:{


        return {};//{0, process_subscription_message(v.substr(p,v.size() - p), c)};
    }
    case mt::PING:{
        observer->on_ping();
        return {ping.size(),{}};
//        m_log->trace("ping recived");
//        boost::system::error_code ec;

//        constexpr std::string_view pong = "PONG\r\n";
//        boost::asio::async_write(m_socket, boost::asio::buffer(pong), boost::asio::transfer_exactly(pong.size()),  c[ec]);
//        if (auto s = handle_error(c); s.failed()) return {0, s};
//        m_log->trace("pong sent");
    }
    case mt::PONG:{
        observer->on_pong();
        return { pong.size(), {}};
    }
    case mt::OK:{
        observer->on_ok();
         return { ok.size(), {}};
    }
    case mt::ERR:{
        p += 1;//space
        auto err_msg = v.substr(p, end_p - p);
        observer->on_error(err_msg);
        constexpr std::string_view err_header_tail = "-ERR \r\n";
        return {err_msg.size() + err_header_tail.size(), {}};
    }
    default:{
        return {0, {"unexpected message type"}};
    }
    }
    return {};
}

}

