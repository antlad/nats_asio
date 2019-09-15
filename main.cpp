#include <nats-asio/client.hpp>
#include <nats-asio/defs.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <fmt/format.h>

#include <iostream>

int main()
{
    auto console = spdlog::stdout_color_mt("console");
    console->set_level(spdlog::level::trace);

    std::string host = "127.0.0.1";
    uint16_t port = 4222;


    try {

        boost::asio::io_context ioc;
        boost::asio::io_context::work w(ioc);
        auto c = std::make_shared<nats_asio::client>(console, ioc);

        boost::asio::spawn(ioc, std::bind(&nats_asio::client::connect, c.get(), host, port, std::placeholders::_1));


        ioc.run();
    } catch (const std::exception& e)
    {

        console->error("unhadled exception {}", e.what());
    }

//    fmt::format("")

//    std::cout << "!" << std::endl;
    return 0;
}
