#pragma once

#include <nats_asio/defs.hpp>

#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/read_until.hpp>

namespace nats_asio {

using boost::asio::ip::tcp;
namespace ssl = boost::asio::ssl;

constexpr auto sep = "\r\n";

class uni_socket {
public:
	uni_socket(ssl::context& ctx, aio& io)
		: m_socket_ssl(io, ctx)
		, m_socket(io)
		, m_use_ssl(false)
	{}

	void async_connect(std::string address, uint16_t port, ctx c)
	{
		if (m_use_ssl)
		{
			m_socket_ssl.lowest_layer().async_connect(boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address(address), port), c);
		}
		else
		{
			m_socket.async_connect(boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address(address), port), c);
		}
	}

	void async_handshake(ctx c)
	{
		if (m_use_ssl)
		{
			m_socket_ssl.async_handshake(boost::asio::ssl::stream_base::client, c);
		}
	}

	void set_use_ssl(bool use_ssl)
	{
		m_use_ssl = use_ssl;
	}

	template<class Buf>
	void async_read_until(Buf& buf, ctx c)
	{
		if (m_use_ssl)
		{
			boost::asio::async_read_until(m_socket_ssl, buf, sep, c);
		}
		else
		{
			boost::asio::async_read_until(m_socket, buf, sep, c);
		}
	}

	template<class Buf, class Transfer>
	void async_read(Buf& buf, const Transfer& until, ctx c)
	{
		if (m_use_ssl)
		{
			boost::asio::async_read(m_socket_ssl, buf, until, c);
		}
		else
		{
			boost::asio::async_read(m_socket, buf, until, c);
		}
	}

	template<class Buf, class Transfer>
	void async_write(const Buf& buf, const Transfer& until, ctx c)
	{
		if (m_use_ssl)
		{
			boost::asio::async_write(m_socket_ssl, buf, until, c);
		}
		else
		{
			boost::asio::async_write(m_socket, buf, until, c);
		}
	}

	void async_shutdown(ctx c)
	{
		if (m_use_ssl)
		{
			m_socket_ssl.async_shutdown(c);
		}
	}

	void close(boost::system::error_code& ec)
	{
		if (m_use_ssl)
		{
			m_socket_ssl.lowest_layer().close(ec);
		}
		else
		{
			m_socket.close(ec);
		}
	}

private:
	boost::asio::ssl::stream<boost::asio::ip::tcp::socket> m_socket_ssl;
	boost::asio::ip::tcp::socket m_socket;
	bool m_use_ssl;
};

} // namespace nats_asio
