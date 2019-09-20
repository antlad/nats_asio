#pragma once

#include <nats-asio/common.hpp>
#include <nats-asio/defs.hpp>

#include <string_view>

namespace nats_asio {

struct parser_observer
{
    virtual ~parser_observer() = default;

    virtual void on_ping(ctx c) = 0;

    virtual void on_pong(ctx c) = 0;

    virtual void on_ok(ctx c) = 0;

    virtual void on_error(std::string_view err, ctx c) = 0;

    virtual void on_info(std::string_view info, ctx c) = 0;

    virtual void on_message(std::string_view subject, std::string_view sid, std::optional<std::string_view> reply_to, const char* raw, std::size_t n, ctx c) = 0;

};

std::tuple<std::size_t, status> parse_message(std::string_view buffer, parser_observer* observer, ctx c);

}




