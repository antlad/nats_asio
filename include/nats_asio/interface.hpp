#pragma once

#include <nats_asio/fwd.hpp>
#include <nats_asio/common.hpp>
#include <nats_asio/defs.hpp>

namespace nats_asio {


struct isubscription
{
	virtual ~isubscription() = default;

	virtual uint64_t sid() = 0;

	virtual void cancel() = 0;
};


struct iconnection;

typedef std::shared_ptr<isubscription> isubscription_sptr;

typedef std::function<void(iconnection* conn, ctx c)> on_connected;
typedef std::function<void(iconnection* conn, ctx c)> on_disconnected;


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

iconnection_sptr create_connection(const logger& log, aio& io, const on_connected_cb& connected_cb, const on_disconnected_cb& disconnected_cb, optional<ssl_config> ssl);

}
