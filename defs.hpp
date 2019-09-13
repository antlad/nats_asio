#pragma once

#include <spdlog/spdlog.h>

#include <boost/asio/io_context.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ssl/context.hpp>

#include <memory>

namespace nats_asio {


typedef boost::asio::io_context aio;
typedef std::shared_ptr<spdlog::logger> logger;
typedef boost::asio::yield_context ctx;


}
