#include <nats_asio/interface.hpp>
#include <nats_asio/fwd.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <fmt/format.h>

#include <iostream>
#include <tuple>

int main()
{
    try
    {
        auto console = spdlog::stdout_color_mt("console");
        console->set_level(spdlog::level::debug);
        boost::asio::io_context ioc;
        boost::asio::io_context::work w(ioc);
        std::size_t counter = 0;
        auto conn = nats_asio::create_connection(console, ioc, [&console, &counter](nats_asio::iconnection & c, nats_asio::ctx ctx)
        {
            console->info("on connected");
            auto r = c.subscribe("output", {}, [&counter](nats_asio::string_view, nats_asio::optional<nats_asio::string_view>, const char* /*raw*/, std::size_t /*n*/, nats_asio::ctx)
            {
                counter++;
            }, ctx);

            if (r.second.failed())
            {
                console->error("failed to subscribe with error: {}", r.second.error());
            }
        }, [&console](nats_asio::iconnection&, nats_asio::ctx)
        {
            console->info("on disconnected");
        });
        boost::asio::spawn(ioc,
                           [&](boost::asio::yield_context ctx)
        {
            boost::asio::deadline_timer timer(ioc);
            boost::system::error_code error;
            const std::string msg {"{ \"something\": 123 }"};

            for (;;)
            {
                auto s = conn->publish("publish", msg.data(), msg.size(), {}, ctx);

                if (s.failed())
                {
                    console->error("publish failed {}", s.error());
                }

                console->info("on timer msgs: {}", counter);
                timer.expires_from_now(boost::posix_time::seconds(1));
                timer.async_wait(ctx[error]);
            }
        });
        nats_asio::connect_config conf;
        conf.address = "127.0.0.1";
        conf.port = 4222;
        conf.user = "admin";
        conf.password = "123";
        conn->start(conf);
        ioc.run();
    }
    catch (const std::exception& e)
    {
        std::cout << "unhandled exception " << e.what() << std::endl;
    }

    return 0;
}
