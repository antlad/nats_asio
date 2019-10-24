#pragma once

#include "nats_asio/interface.hpp"

#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/read_until.hpp>

namespace nats_asio {

using boost::asio::ip::tcp;
namespace ssl = boost::asio::ssl;

constexpr auto sep = "\r\n";

typedef boost::asio::ip::tcp::socket raw_socket;
typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_socket;

template<class Socket>
auto& take_raw_ref(Socket&);

template<>
auto& take_raw_ref(ssl_socket& s)
{
	return s.next_layer();
}

template<>
auto& take_raw_ref(raw_socket& s)
{
	return s;
}


template<class Socket>
struct uni_socket
{
	uni_socket(aio& io)
		: m_socket(io)
	{}

	uni_socket(aio& io, boost::asio::ssl::context& ctx)
		: m_socket(io, ctx)
	{
	}

	void async_connect(const boost::asio::ip::tcp::endpoint& endpoint, ctx c);

	void async_handshake(ctx c);

	template<class Buf>
	void async_read_until(Buf& buf, ctx c)
	{
		boost::asio::async_read_until(m_socket, buf, sep, c);
	}

	template<class Buf>
	void async_read_until_raw(Buf& buf, ctx c)
	{
		boost::asio::async_read_until(take_raw_ref(m_socket), buf, sep, c);
	}

	template<class Buf, class Transfer>
	void async_read(Buf& buf, const Transfer& until, ctx c)
	{
		boost::asio::async_read(m_socket, buf, until, c);
	}

	template<class Buf, class Transfer>
	void async_write(const Buf& buf, const Transfer& until, ctx c)
	{
		boost::asio::async_write(m_socket, buf, until, c);
	}

	void async_shutdown(ctx c);

	void close(boost::system::error_code& ec);

	Socket m_socket;
};

template<>
void uni_socket<raw_socket>::close(boost::system::error_code& ec)
{
	m_socket.close(ec);
}

template<>
void uni_socket<ssl_socket>::close(boost::system::error_code& ec)
{
	m_socket.lowest_layer().close(ec);
}

template<>
void uni_socket<raw_socket>::async_handshake(ctx /*c*/)
{
}

template<>
void uni_socket<ssl_socket>::async_handshake(ctx c)
{
	m_socket.async_handshake(boost::asio::ssl::stream_base::client, c);
}


template<>
void uni_socket<raw_socket>::async_shutdown(ctx /*c*/)
{
}

template<>
void uni_socket<ssl_socket>::async_shutdown(ctx c)
{
	m_socket.async_shutdown(c);
}

template<>
void uni_socket<raw_socket>::async_connect(const boost::asio::ip::tcp::endpoint& endpoint, ctx c)
{
	m_socket.async_connect(endpoint, c);
}

template<>
void uni_socket<ssl_socket>::async_connect(const boost::asio::ip::tcp::endpoint& endpoint, ctx c)
{
	m_socket.lowest_layer().async_connect(endpoint, c);
}

} // namespace nats_asio
