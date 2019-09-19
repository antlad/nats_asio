#include <nats-asio/interface.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <fmt/format.h>

#include <iostream>


void publish(nats_asio::iconnection * c, nats_asio::ctx ctx)
{
   // c->publish()
}

int main()
{
    try
    {
        auto console = spdlog::stdout_color_mt("console");
        console->set_level(spdlog::level::trace);

        boost::asio::io_context ioc;
        boost::asio::io_context::work w(ioc);
        auto conn = nats_asio::create_connection(console, ioc,[&console](nats_asio::iconnection& c, nats_asio::ctx ctx){
                console->info("on connected");
               auto [sub, s] = c.subscribe("output", nullptr, [&console](std::string_view , std::optional<std::string_view>, const char* raw, std::size_t n, nats_asio::ctx ){
                        std::string_view view(raw, n);
                        console->info("on new message:  {}", view);
                    }, ctx);
               if (s.failed())
               {
                   console->error("failed to subscribe with error: {}", s.error());
               }

            }, [&console](nats_asio::iconnection&, nats_asio::ctx){
                console->info("on disconnected");
            });

        conn->start("127.0.0.1", 4222);

        ioc.run();
    }
    catch (const std::exception& e)
    {
        std::cout << "unhadled exception " << e.what() << std::endl;
    }

    return 0;
}
