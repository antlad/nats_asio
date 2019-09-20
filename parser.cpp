#include <nats-asio/parser.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <map>
#include <vector>

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
constexpr std::string_view info_header_tail = "INFO \r\n";
constexpr std::string_view err_header_tail = "-ERR \r\n";

std::vector<std::string_view> split_sv(std::string_view str, std::string_view delims = " ")
{
    std::vector<std::string_view> output;
    output.reserve(str.size() / 2);

    for (auto first = str.data(), second = str.data(), last = first + str.size(); second != last && first != last; first = second + 1) {
        second = std::find_first_of(first, last, std::cbegin(delims), std::cend(delims));

        if (first != second)
            output.emplace_back(first, second - first);
    }

    return output;
}


std::tuple<std::size_t, status> parse_message(std::string_view v, parser_observer *observer, ctx c)
{
    auto p = v.find_first_of(" \r\n");
    auto end_p = v.find("\r\n", p);
    auto end_size = end_p + 2;
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
        observer->on_info(info_msg, c);

        return {info_header_tail.size() + info_msg.size(), {}};
    }
    case mt::MSG:{
         p += 1;
        auto info = v.substr(p, end_p - p);
        auto results = split_sv(info, " ");

        if (results.size() < 3 || results.size() > 4)
        {
            return {0, {"unexpected message format"}};
        }
        bool replty_to = results.size() == 4;
        std::size_t bytes_id = replty_to ? 3 : 2;

        std::size_t bytes_n = 0;
        try
        {
            bytes_n = boost::lexical_cast<uint64_t>(results[bytes_id]);
        } catch (const std::exception& e) {
            return {0, {"can't parse int in headers: {}", e.what()}};
        }

        if ((end_size + bytes_n + 2) > v.size())
        {
            return {0, {"payload not yet in buffer"}};
        }

        const char * raw = &v[end_p + 2];
        if (replty_to)
        {
             observer->on_message(results[0], results[1],  results[2], raw, bytes_n, c);
        }
        else
        {
            observer->on_message(results[0], results[1],  std::optional<std::string_view>(), raw, bytes_n, c);
        }

        return {end_size + bytes_n + 2, {}};
    }
    case mt::PING:{
        observer->on_ping(c);
        return {ping.size(),{}};
    }
    case mt::PONG:{
        observer->on_pong(c);
        return { pong.size(), {}};
    }
    case mt::OK:{
         observer->on_ok(c);
         return { ok.size(), {}};
    }
    case mt::ERR:{
        p += 1;//space
        auto err_msg = v.substr(p, end_p - p);
        observer->on_error(err_msg, c);
        return {err_msg.size() + err_header_tail.size(), {}};
    }
    default:{
        return {0, {"unexpected message type"}};
    }
    }
}

}

