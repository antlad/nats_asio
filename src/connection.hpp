#include "parser.hpp"
#include "socket.hpp"

#include <nats_asio/fwd.hpp>
#include <nats_asio/defs.hpp>
#include <nats_asio/common.hpp>
#include <nats_asio/interface.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/read.hpp>

#include <boost/asio/streambuf.hpp>

#include "json.hpp"

#include <string>
#include <functional>
#include <utility>

namespace nats_asio {


struct subscription: public isubscription
{
	subscription(uint64_t sid, const on_message_cb& cb)
		: m_cancel(false)
		, m_cb(cb)
		, m_sid(sid)

	{
	}

	virtual void cancel() override
	{
		m_cancel = true;
	}

	virtual uint64_t sid() override
	{
		return m_sid;
	}

	bool m_cancel;
	on_message_cb m_cb;
	uint64_t m_sid;
};

template<class SocketType>
class connection : public iconnection, public parser_observer {
public:
	connection(aio& io,
			   const logger& log,
			   const on_connected_cb& connected_cb,
			   const on_disconnected_cb& disconnected_cb,
			   uni_socket<SocketType>&& socket);


	virtual void start(const connect_config& conf) override;

	virtual void stop() override
	{
		m_stop_flag = true;
	}

	virtual bool is_connected() override
	{
		return m_is_connected;
	}

	virtual status publish(string_view subject, const char* raw, std::size_t n, optional<string_view> reply_to, ctx c) override;

	virtual status unsubscribe(const isubscription_sptr& p, ctx c) override;

	virtual std::pair<isubscription_sptr, status> subscribe(string_view subject,  optional<string_view> queue, on_message_cb cb, ctx c) override;


private:
	virtual void on_ping(ctx c) override;

	virtual void on_pong(ctx c) override
	{
		m_log->trace("pong recived");
	}

	virtual void on_ok(ctx c) override
	{
		m_log->trace("ok recived");
	}

	virtual void on_error(string_view err, ctx c) override
	{
		m_log->error("error message from server {}", err);
	}

	virtual void on_info(string_view info, ctx c) override;


	virtual void on_message(string_view subject, string_view sid, optional<string_view> reply_to, std::size_t n, ctx c) override;

	virtual void consumed(std::size_t n) override
	{
		m_buf.consume(n);
	}

	status do_connect(const connect_config& conf, ctx c);

	void run(const connect_config& conf, ctx c);

	//	void load_certificates(const ssl_config& conf);

	status handle_error(ctx c);

	std::string prepare_info(const connect_config& o);

	uint64_t next_sid()
	{
		return m_sid++;
	}

	uint64_t m_sid;
	std::size_t m_max_payload;
	logger m_log;
	aio& m_io;

	bool m_is_connected;
	bool m_stop_flag;

	std::unordered_map<uint64_t, subscription_sptr> m_subs;
	on_connected_cb m_connected_cb;
	on_disconnected_cb m_disconnected_cb;
	boost::system::error_code ec;

	boost::asio::streambuf m_buf;
	//	boost::asio::ssl::context m_ssl_ctx;

	uni_socket<SocketType> m_socket;
};


void load_certificates(const ssl_config& conf, boost::asio::ssl::context& ctx)
{
	ctx.set_options(
		boost::asio::ssl::context::default_workarounds |
		boost::asio::ssl::context::no_sslv2 |
		boost::asio::ssl::context::no_sslv3 |
		boost::asio::ssl::context::no_tlsv1 |
		boost::asio::ssl::context::single_dh_use |
		boost::asio::ssl::context::tls_client
	);

	if (conf.ssl_verify)
	{
		ctx.set_verify_mode(boost::asio::ssl::verify_peer);
	}
	else
	{
		ctx.set_verify_mode(boost::asio::ssl::verify_none);
	}

	if (!conf.ssl_cert.empty())
	{
		ctx.use_certificate(boost::asio::buffer(conf.ssl_cert.data(), conf.ssl_cert.size()), boost::asio::ssl::context::file_format::pem);
	}

	if (!conf.ssl_ca.empty())
	{
		ctx.use_certificate_chain(boost::asio::buffer(conf.ssl_ca.data(), conf.ssl_ca.size()));
	}

	if (!conf.ssl_dh.empty())
	{
		ctx.use_tmp_dh_file(conf.ssl_dh);
	}

	if (!conf.ssl_key.empty())
	{
		ctx.use_private_key(boost::asio::buffer(conf.ssl_key.data(), conf.ssl_key.size()), boost::asio::ssl::context::file_format::pem);
	}
}

iconnection_sptr create_connection(aio& io, const logger& log, const on_connected_cb& connected_cb, const on_disconnected_cb& disconnected_cb, optional<ssl_config> ssl_conf)
{
	if (ssl_conf.has_value())
	{
		boost::asio::ssl::context ssl_ctx(boost::asio::ssl::context::tls_client);
		load_certificates(ssl_conf.value(), ssl_ctx);
		return std::make_shared<connection<ssl_socket>>(io, log, connected_cb, disconnected_cb, uni_socket<ssl_socket>(io));
	}
	else
	{
		return std::make_shared<connection<raw_socket>>(io, log, connected_cb, disconnected_cb, uni_socket<raw_socket>(io));
	}
}

template<class SocketType>
connection<SocketType>::connection(aio& io, const logger& log, const on_connected_cb& connected_cb, const on_disconnected_cb& disconnected_cb, uni_socket<SocketType>&& socket)
	: m_sid(0)
	, m_max_payload(0)
	, m_log(log)
	, m_io(io)
	, m_is_connected(false)
	, m_stop_flag(false)
	, m_connected_cb(connected_cb)
	, m_disconnected_cb(disconnected_cb)
	, m_socket(std::forward<uni_socket<SocketType>>(socket))
{
}

template<class Socket>
void connection<Socket>::start(const connect_config& conf)
{
	boost::asio::spawn(m_io, std::bind(&connection::run, this, conf, std::placeholders::_1));
}

template<class Socket>
status connection<Socket>::publish(boost::string_view subject, const char* raw, std::size_t n, optional<boost::string_view> reply_to, ctx c)
{
	if (!m_is_connected)
	{
		return status("not connected");
	}

	const std::string pub_header_payload("PUB {} {} {}\r\n");
	std::vector<boost::asio::const_buffer> buffers;
	std::string header;

	if (reply_to.has_value())
	{
		header = fmt::format(pub_header_payload, subject, reply_to.value(), n);
	}
	else
	{
		header = fmt::format(pub_header_payload, subject, "", n);
	}

	buffers.emplace_back(boost::asio::buffer(header.data(), header.size()));
	buffers.emplace_back(boost::asio::buffer(raw, n));
	buffers.emplace_back(boost::asio::buffer("\r\n", 2));
	std::size_t total_size = header.size() + n + 4;
	m_socket.async_write(buffers, boost::asio::transfer_exactly(total_size), c[ec]);
	return handle_error(c);
}

template<class Socket>
status connection<Socket>::unsubscribe(const isubscription_sptr& p, ctx c)
{
	auto it = m_subs.find(p->sid());

	if (it == m_subs.end())
	{
		return status("subscription not found {}", p->sid());
	}

	m_subs.erase(it);
	const std::string unsub_payload("UNSUB {}\r\n");
	m_socket.async_write(boost::asio::buffer(unsub_payload),
						 boost::asio::transfer_exactly(unsub_payload.size()),
						 c[ec]);
	return handle_error(c);
}

template<class Socket>
std::pair<isubscription_sptr, status> connection<Socket>::subscribe(boost::string_view subject, optional<boost::string_view> queue, on_message_cb cb, ctx c)
{
	if (!m_is_connected)
	{
		return  {isubscription_sptr(), status("not connected")};
	}

	const std::string sub_payload("SUB {} {} {}\r\n");
	auto sid = next_sid();
	std::string payload;

	if (queue.has_value())
	{
		payload = fmt::format(sub_payload, subject, queue.value(), sid);
	}
	else
	{
		payload = fmt::format(sub_payload, subject, "", sid);
	}

	m_socket.async_write(boost::asio::buffer(payload),
						 boost::asio::transfer_exactly(payload.size()),
						 c[ec]);
	auto s = handle_error(c);

	if (s.failed())
	{
		return {isubscription_sptr(), s};
	}

	auto sub = std::make_shared<subscription>(sid, cb);
	m_subs.emplace(sid, sub);
	return {sub, {}};
}

template<class Socket>
void connection<Socket>::on_ping(ctx c)
{
	m_log->trace("ping recived");
	const std::string pong("PONG\r\n");
	m_socket.async_write(boost::asio::buffer(pong),
						 boost::asio::transfer_exactly(pong.size()),
						 c[ec]);
	handle_error(c);
}

template<class Socket>
void connection<Socket>::on_info(boost::string_view info, ctx c)
{
	using nlohmann::json;
	auto j = json::parse(info);
	m_log->debug("got info {}", j.dump());
	m_max_payload = j["max_payload"].get<std::size_t>();
	m_log->trace("info recived and parsed");
}

template<class Socket>
void connection<Socket>::on_message(boost::string_view subject, boost::string_view sid_str, optional<boost::string_view> reply_to, std::size_t n, ctx c)
{
	int bytes_to_transsfer = int(n) + 2 - int(m_buf.size());

	if (bytes_to_transsfer > 0)
	{
		m_socket.async_read(m_buf,
							boost::asio::transfer_at_least(std::size_t(bytes_to_transsfer)),
							c[ec]);
	}

	auto s = handle_error(c);

	if (s.failed())
	{
		m_log->error("failed to read {}", s.error());
		return;
	}

	std::size_t sid_u = 0;

	try
	{
		sid_u = static_cast<std::size_t>(std::stoll(sid_str.data(), nullptr, 10));
	}
	catch (const std::exception& e)
	{
		m_log->error("can't parse sid: {}", e.what());
		return;
	}

	auto it = m_subs.find(sid_u);

	if (it == m_subs.end())
	{
		m_log->trace("dropping message because subscription not found: topic: {}, sid: {}",
					 subject,
					 sid_str);
		return;
	}

	if (it->second->m_cancel)
	{
		m_log->trace("subscribtion canceled {}", sid_str);
		s = unsubscribe(it->second, c);

		if (s.failed())
		{
			m_log->error("unsubscribe failed: {}", s.error());
		}
	}

	auto b = m_buf.data();

	if (reply_to.has_value())
	{
		it->second->m_cb(subject, reply_to, static_cast<const char*>(b.data()), n, c);
	}
	else
	{
		it->second->m_cb(subject, {}, static_cast<const char*>(b.data()), n, c);
	}
}

template<class Socket>
status connection<Socket>::do_connect(const connect_config& conf, ctx c)
{
	m_socket.async_connect(conf.address, conf.port, c[ec]);
	auto s = handle_error(c);

	if (s.failed())
	{
		return s;
	}

	m_socket.async_handshake(c[ec]);
	s = handle_error(c);

	if (s.failed())
	{
		m_log->error("async handshake failed {}", s.error());
		return s;
	}

	m_socket.async_read_until(m_buf, c[ec]);
	s = handle_error(c);

	if (s.failed())
	{
		m_log->error("read server info failed {}", s.error());
		return s;
	}

	std::string header;
	std::istream is(&m_buf);
	s = parse_header(header, is, this, c);

	if (s.failed())
	{
		m_log->error("process message failed with error: {}", s.error());
		return s;
	}

	auto info = prepare_info(conf);
	m_socket.async_write(boost::asio::buffer(info),
						 boost::asio::transfer_exactly(info.size()),
						 c[ec]);
	s = handle_error(c);

	if (s.failed())
	{
		m_log->error("failed to write info {}", s.error());
		return s;
	}

	return {};
}

template<class Socket>
void connection<Socket>::run(const connect_config& conf, ctx c)
{
	std::string header;

	//		if (conf.ssl.has_value())
	//		{
	//			m_socket.set_use_ssl(true);
	//			load_certificates(conf.ssl.value());
	//		}

	for (;;)
	{
		if (m_stop_flag)
		{
			m_log->debug("stopping main connection loop");
			return;
		}

		if (!m_is_connected)
		{
			auto s = do_connect(conf, c);

			if (s.failed())
			{
				m_log->error("connect failed with error {}", s.error());
				continue;
			}

			m_is_connected = true;

			if (m_connected_cb != nullptr)
			{
				m_connected_cb(*this, c);
			}
		}

		m_socket.async_read_until(m_buf, c[ec]);
		auto s = handle_error(c);

		if (s.failed())
		{
			m_log->error("failed to read {}", s.error());
			continue;
		}

		std::istream is(&m_buf);
		s = parse_header(header, is, this, c);

		if (s.failed())
		{
			m_log->error("process message failed with error: {}", s.error());
			continue;
		}
	}
}

template<class Socket>
status connection<Socket>::handle_error(ctx c)
{
	if (ec.failed())
	{
		//		if ((ec == boost::asio::error::eof) || (boost::asio::error::connection_reset == ec))
		//		{
		auto original_msg = ec.message();
		m_is_connected = false;
		m_socket.close(ec); // TODO: handle it if error

		if (ec.failed())
		{
			m_log->error("error on socket close {}", ec.message());
		}

		if (m_disconnected_cb != nullptr)
		{
			m_disconnected_cb(*this, std::move(c));
		}

		//		}
		return status(original_msg);
	}

	return {};
}

template<class Socket>
std::string connection<Socket>::prepare_info(const connect_config& o)
{
	constexpr auto connect_payload = "CONNECT {}\r\n";
	constexpr auto name = "nats_asio";
	constexpr auto lang = "cpp";
	constexpr auto version = "0.0.1";
	using nlohmann::json;
	json j =
	{
		{"verbose",      o.verbose},
		{"pedantic",     o.pedantic},
		{"name",             name },
		{"lang",             lang },
		{"version",          version},
	};

	if (o.ssl.has_value())
	{
		j["ssl_required"] = o.ssl->ssl_required;
	}

	if (o.user.has_value())
	{
		j["user"] = o.user.value();
	}

	if (o.password.has_value())
	{
		j["pass"] = o.password.value();
	}

	if (o.token.has_value())
	{
		j["auth_token"] = o.token.value();
	}

	auto info = j.dump();
	auto connect_data = fmt::format(connect_payload, info);
	m_log->debug("sending data on connect {}", info);
	return connect_data;
}
} // namespace nats_asio

