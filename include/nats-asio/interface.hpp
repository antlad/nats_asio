#pragma once

#include <nats-asio/fwd.hpp>
#include <nats-asio/common.hpp>
#include <nats-asio/defs.hpp>

namespace nats_asio {


struct isubscription{
    virtual ~isubscription() = default;

    virtual uint64_t sid() = 0;

    virtual void cancel() = 0;
};


struct iconnection;

typedef std::shared_ptr<isubscription> isubscription_sptr;

typedef std::function<void(iconnection* conn, ctx c)> on_connected;
typedef std::function<void(iconnection* conn, ctx c)> on_disconnected;

struct iconnection {
    virtual ~iconnection() = default;

    virtual void start(string_view address, uint16_t port) = 0;

    virtual void stop() = 0;

    virtual bool is_connected() = 0;

    virtual status publish(string_view subject, const char* raw, std::size_t n, optional<string_view> reply_to, ctx c) = 0;

    virtual status unsubscribe(const isubscription_sptr& p, ctx c) = 0;

    virtual std::tuple<isubscription_sptr,status> subscribe(string_view subject, optional<string_view> queue, on_message_cb cb, ctx c) = 0;

};
typedef std::shared_ptr<iconnection> iconnection_sptr;

typedef std::function<void(iconnection&, ctx)> on_connected_cb;
typedef std::function<void(iconnection&, ctx)> on_disconnected_cb;

iconnection_sptr create_connection(const logger& log, aio& io, const on_connected_cb& connected_cb, const on_disconnected_cb& disconnected_cb);

}
