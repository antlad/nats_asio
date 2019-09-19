#pragma once

#include <nats-asio/common.hpp>

#include <string_view>

namespace nats_asio {

struct parser_observer
{
    virtual ~parser_observer() = default;

    virtual void on_ping() = 0;

    virtual void on_pong() = 0;

    virtual void on_ok() = 0;

    virtual void on_error(std::string_view err) = 0;

    virtual void on_info(std::string_view info) = 0;

    virtual void on_message(std::string_view subject, std::string_view sid, std::optional<std::string_view> reply_to, const char* raw, std::size_t n) = 0;

};

std::tuple<std::size_t, status> parse_message(std::string_view buffer, parser_observer* observer);

}




