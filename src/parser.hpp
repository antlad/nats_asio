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

    virtual void on_error(string_view err, ctx c) = 0;

    virtual void on_info(string_view info, ctx c) = 0;

    virtual void on_message(string_view subject, string_view sid, optional<string_view> reply_to, std::size_t n, ctx c) = 0;

    virtual void consumed(std::size_t n) = 0;

};

status parse_header(std::string& header, std::istream& is, parser_observer* observer, ctx c);

}




