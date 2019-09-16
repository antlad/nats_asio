//#include <nats-asio/client.hpp>
//#include <nats-asio/fwd.hpp>
//#include <nats-asio/defs.hpp>
//#include <nats-asio/connection.hpp>
#include <nats-asio/interface.hpp>


#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <fmt/format.h>

#include <iostream>

void main_async(const nats_asio::logger& log, const nats_asio::iconnection_sptr& conn, nats_asio::ctx ctx)
{
    uint16_t port = 4222;
    std::string host = "127.0.0.1";

    auto s = conn->connect(host, port, ctx);
    if (s.failed())
    {
        log->error("failed to connect {}", s.error());
        return;
    }
    auto f = [log](std::string_view subject, const char* raw, std::size_t n, nats_asio::ctx /*c*/){
        std::string payload(raw, n);
        log->debug("on new message: subject {}, payload: {}", subject, payload);
    };

    auto sub = conn->subscribe("output", nullptr, f, ctx);

}

int main()
{
    try
    {
        auto console = spdlog::stdout_color_mt("console");
        console->set_level(spdlog::level::trace);

        boost::asio::io_context ioc;
        boost::asio::io_context::work w(ioc);


        auto conn = nats_asio::create_connection(console, ioc);

        boost::asio::spawn(ioc, std::bind(&main_async, console, conn,  std::placeholders::_1));
        ioc.run();
    }
    catch (const std::exception& e)
    {
        std::cout << "unhadled exception " << e.what() << std::endl;
    }

    return 0;
}