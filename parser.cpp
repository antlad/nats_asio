#include <nats-asio/parser.hpp>

#include <boost/algorithm/string.hpp>

#include <map>
#include <vector>

namespace nats_asio {

enum class mt
{
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

std::vector<string_view> split_sv(string_view str, string_view delims = " ")
{
    std::vector<string_view> output;
    output.reserve(str.size() / 2);

    for (auto first = str.data(), second = str.data(), last = first + str.size(); second != last && first != last; first = second + 1)
    {
        second = std::find_first_of(first, last, std::cbegin(delims), std::cend(delims));

        if (first != second)
        {
            output.emplace_back(first, second - first);
        }
    }

    return output;
}


status parse_header(std::string& header, std::istream& is, parser_observer* observer, ctx c)
{
    if (!std::getline(is, header))
    {
        return {"can't get line"};
    }

    // observer->consumed(0);

    if (header.size() < 4)
    {
        return {"too small header"};
    }

    auto v = string_view(&header[0], header.size());
    auto p = v.find_first_of(" \r");

    if (p == string_view::npos)
    {
        return {"protocol violation from server"};
    }

    v = string_view(&header[0], header.size() - 1);
    auto it = message_types_map.find(v.substr(0, p));

    if (it == message_types_map.end())
    {
        return {"unknown message"};
    }

    switch (it->second)
    {
    case mt::INFO:
    {
        p += 1;//space
        auto info_msg = v.substr(p, v.size() - p);
        observer->on_info(info_msg, c);
        break;
    }

    case mt::MSG:
    {
        p += 1;
        auto info = v.substr(p, v.size() - p);
        auto results = split_sv(info, " ");

        if (results.size() < 3 || results.size() > 4)
        {
            return {"unexpected message format"};
        }

        bool replty_to = results.size() == 4;
        std::size_t bytes_id = replty_to ? 3 : 2;
        std::size_t bytes_n = 0;

        try
        {
            std::stoll(results[bytes_id].data(), &bytes_n);
        }
        catch (const std::exception& e)
        {
            return {"can't parse int in headers: {}", e.what()};
        }

        // observer->consumed(header.size() + 1);

        if (replty_to)
        {
            observer->on_message(results[0], results[1],  results[2], bytes_n, c);
        }
        else
        {
            observer->on_message(results[0], results[1], optional<string_view>(), bytes_n, c);
        }

        observer->consumed(bytes_n + 2);
        return  {};
    }

    case mt::PING:
    {
        observer->on_ping(c);
        break;
    }

    case mt::PONG:
    {
        observer->on_pong(c);
        break;
    }

    case mt::OK:
    {
        observer->on_ok(c);
        break;
    }

    case mt::ERR:
    {
        p += 1;//space
        auto err_msg = v.substr(p, v.size() - p);
        observer->on_error(err_msg, c);
        break;
    }

    default:
    {
        return {"unexpected message type"};
    }
    }

    // observer->consumed(header.size() + 1);
    return {};
}

}

