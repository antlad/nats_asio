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

typedef std::shared_ptr<isubscription> isubscription_sptr;

struct iconnection {
    virtual ~iconnection() = default;

    virtual status connect(std::string_view address, uint16_t port, ctx c) = 0;

    virtual status publish(std::string_view subject, const char* raw, std::size_t n, std::optional<std::string_view> reply_to, ctx c) = 0;

    virtual status unsubscribe(const isubscription_sptr& p, ctx c) = 0;

    virtual std::tuple<isubscription_sptr,status> subscribe(std::string_view subject, std::optional<std::string_view> queue, on_message_cb cb, ctx c) = 0;
};
typedef std::shared_ptr<iconnection> iconnection_sptr;

typedef std::function<void()> on_connected_cb;
typedef std::function<void()> on_disconnected_cb;

iconnection_sptr create_connection(const logger& log, aio& io);

}
