#pragma once

#include <nats-asio/fwd.hpp>
#include <nats-asio/defs.hpp>
#include <nats-asio/common.hpp>
#include <nats-asio/interface.hpp>

#include <string>
#include <optional>
#include <functional>

namespace nats_asio {

class connection: public iconnection
{
public:
    connection(const logger& log, aio& io, const on_connected_cb& connected_cb, const on_disconnected_cb& disconnected_cb);

    virtual void start(std::string_view address, uint16_t port) override;

    virtual void stop() override;

    virtual bool is_connected() override;

    virtual status publish(std::string_view subject, const char* raw, std::size_t n, std::optional<std::string_view> reply_to, ctx c) override;

    virtual status unsubscribe(const isubscription_sptr& p, ctx c) override;

    virtual std::tuple<isubscription_sptr,status> subscribe(std::string_view subject,  std::optional<std::string_view> queue, on_message_cb cb, ctx c) override;

private:
    //status socket_write(const char* raw, std::size_t n, ctx c);

    void run(std::string_view address, uint16_t port, ctx c);

    status handle_error(ctx c);

    std::tuple<std::size_t, status> process_message(std::string_view v, ctx c);

    status process_subscription_message(std::string_view v, ctx c);

    std::string prepare_info(const options &o);

    uint64_t next_sid();

    uint64_t m_sid;
    std::size_t m_max_payload;
    logger m_log;
    aio& m_io;

    bool m_is_connected;
    bool m_stop_flag;

    std::unordered_map<uint64_t, subscription_sptr> m_subs;
    boost::asio::ip::tcp::socket m_socket;
    on_connected_cb m_connected_cb;
    on_disconnected_cb m_disconnected_cb;
    boost::system::error_code ec;

    std::size_t m_max_attempts = 10;
};



}


