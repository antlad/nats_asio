#include "parser.hpp"
#include "socket.hpp"

#include <nats_asio/fwd.hpp>
#include <nats_asio/defs.hpp>
#include <nats_asio/common.hpp>
#include <nats_asio/interface.hpp>

#include <boost/asio/streambuf.hpp>

#include <string>
#include <functional>

namespace nats_asio {

class connection : public iconnection, public parser_observer {
public:
	connection(const logger& log,
			   aio& io,
			   const on_connected_cb& connected_cb,
			   const on_disconnected_cb& disconnected_cb);


	virtual void start(const connect_config& conf) override;

	virtual void stop() override;

	virtual bool is_connected() override;

	virtual status publish(string_view subject, const char* raw, std::size_t n, optional<string_view> reply_to, ctx c) override;

	virtual status unsubscribe(const isubscription_sptr& p, ctx c) override;

	virtual std::pair<isubscription_sptr, status> subscribe(string_view subject,  optional<string_view> queue, on_message_cb cb, ctx c) override;


private:
	virtual void on_ping(ctx c) override;

	virtual void on_pong(ctx c) override;

	virtual void on_ok(ctx c) override;

	virtual void on_error(string_view err, ctx c) override;

	virtual void on_info(string_view info, ctx c) override;


	virtual void on_message(string_view subject, string_view sid, optional<string_view> reply_to, std::size_t n, ctx c) override;

	virtual void consumed(std::size_t n) override;

	status do_connect(const connect_config& conf, ctx c);

	void run(const connect_config& conf, ctx c);

	void load_certificates(const ssl_config& conf);

	status handle_error(ctx c);

	std::string prepare_info(const connect_config& o);

	uint64_t next_sid();

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
	boost::asio::ssl::context m_ssl_ctx;
	uni_socket m_socket;
};
} // namespace nats_asio

