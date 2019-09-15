#pragma once

#include <nats-asio/fwd.hpp>
#include <nats-asio/defs.hpp>
#include <nats-asio/common.hpp>

#include <string>
#include <optional>
#include <functional>

namespace nats_asio {




class connection
{
public:
    connection(const logger& log, aio& io);

    status connect(std::string_view address, uint16_t port, ctx c);

    status publish(std::string_view subject, const char* raw, std::size_t n, std::optional<std::string> reply_to = {});

    std::tuple<subscription_sptr,status> subscribe(std::string_view subject, on_message_cb cb, ctx c);



    subscription_sptr subscribe_queue(std::string_view subject, std::string_view queue, on_message_cb cb, ctx c);

    void run(ctx c);
private:

    status process_message(std::string_view v, ctx c);

    status process_subscription_message(std::string_view v, ctx c);

    std::string prepare_info(const options &o);

    uint64_t next_sid();

    uint64_t m_sid;
    std::size_t m_max_payload;
    logger m_log;
    aio& m_io;

    std::unordered_map<uint64_t, subscription_sptr> m_subs;
    boost::asio::ip::tcp::socket m_socket;
    boost::system::error_code ec;
};



}


