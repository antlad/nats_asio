#pragma once

#include <nats_asio/fwd.hpp>

#include <spdlog/spdlog.h>

#include <boost/asio/io_context.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ssl/context.hpp>


#include <memory>

namespace nats_asio {

typedef boost::asio::io_context aio;
typedef std::shared_ptr<spdlog::logger> logger;
typedef boost::asio::yield_context ctx;
typedef std::shared_ptr<boost::asio::ip::tcp::socket> socket_sptr;


typedef std::function<void(string_view subject, optional<string_view> reply_to, const char* raw, std::size_t n, ctx c)> on_message_cb;

}
