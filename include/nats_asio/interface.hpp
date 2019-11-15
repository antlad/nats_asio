/*
MIT License

Copyright (c) 2019 Vladislav Troinich

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
                                                              copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

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

#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include <boost/asio/io_context.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/deadline_timer.hpp>

#include <boost/concept/detail/general.hpp>
#include <boost/concept/detail/general.hpp>
#include <boost/optional.hpp>
#include <boost/utility/string_view.hpp>


#include <stdexcept>
#include <memory>

template <>
struct fmt::formatter<boost::string_view>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const boost::string_view& d, FormatContext& ctx)
    {
        return format_to(ctx.out(), "{}", d.data());
    }
};

namespace nats_asio {

using boost::optional;
using boost::string_view;

typedef boost::asio::io_context aio;
typedef std::shared_ptr<spdlog::logger> logger;
typedef boost::asio::yield_context ctx;

typedef std::function<void(string_view subject, optional<string_view> reply_to, const char* raw, std::size_t n, ctx c)> on_message_cb;

class status {
public:
    status() = default;

    status(const std::string& error)
        : m_error(error)
    {
    }

    template <typename S, typename... Args, typename Char = fmt::char_t<S>>
    status(const S& format_str, Args && ... args)
        : status(fmt::format(format_str, std::forward<Args>(args)...))
    {
    }

    virtual ~status() = default;

    bool failed() const
    {
        return m_error.has_value();
    }

    std::string error() const
    {
        if (!m_error.has_value())
            return {};

        return m_error.value();
    }
private:
    optional<std::string> m_error;
};

struct isubscription
{
	virtual ~isubscription() = default;

	virtual uint64_t sid() = 0;

	virtual void cancel() = 0;
};
typedef std::shared_ptr<isubscription> isubscription_sptr;

struct ssl_config
{
	std::string ssl_key;
	std::string ssl_cert;
	std::string ssl_ca;
	std::string ssl_dh;
	bool ssl_required = false;
	bool ssl_verify = true;
};

struct connect_config
{
	std::string address;
	uint16_t port;

	bool verbose = false;
	bool pedantic = false;

	optional<std::string> user;
	optional<std::string> password;
	optional<std::string> token;
};

struct iconnection
{
	virtual ~iconnection() = default;

	virtual void start(const connect_config& conf) = 0;

	virtual void stop() = 0;

	virtual bool is_connected() = 0;

	virtual status publish(string_view subject, const char* raw, std::size_t n, optional<string_view> reply_to, ctx c) = 0;

	virtual status unsubscribe(const isubscription_sptr& p, ctx c) = 0;

	virtual std::pair<isubscription_sptr, status> subscribe(string_view subject, optional<string_view> queue, on_message_cb cb, ctx c) = 0;

};
typedef std::shared_ptr<iconnection> iconnection_sptr;

typedef std::function<void(iconnection&, ctx)> on_connected_cb;
typedef std::function<void(iconnection&, ctx)> on_disconnected_cb;

iconnection_sptr create_connection(aio& io, const logger& log, const on_connected_cb& connected_cb, const on_disconnected_cb& disconnected_cb, optional<ssl_config> ssl_conf);

}
