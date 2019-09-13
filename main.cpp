#include <iostream>
#include "client.hpp"
#include "defs.hpp"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <fmt/format.h>

int main()
{


    auto console = spdlog::stdout_color_mt("console");
    console->set_level(spdlog::level::debug);



    boost::asio::io_context ioc;
    boost::asio::io_context::work w(ioc);

    auto c = std::make_shared<nats_asio::client>(console, ioc);
    std::string host = "127.0.0.1";
    uint16_t port = 4222;
    boost::asio::spawn(ioc, std::bind(&nats_asio::client::connect, c.get(), host, port, std::placeholders::_1));


    ioc.run();
//    fmt::format("")

//    std::cout << "!" << std::endl;
    return 0;
}
