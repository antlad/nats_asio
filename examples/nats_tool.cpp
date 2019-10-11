
#include <nats_asio/interface.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <fmt/format.h>
#include "cxxopts.hpp"

#include <iostream>
#include <tuple>


const std::string grub_mode = "grub";
const std::string gen_mode = "gen";

enum class mode { grubber, generator };

class worker {
public:
	worker(boost::asio::io_context& ioc,
		   std::shared_ptr<spdlog::logger>& console,
		   int stats_interval);

	void stats_timer(boost::asio::yield_context ctx);

protected:
	int m_stats_interval;
	std::size_t m_counter;
	boost::asio::io_context& m_ioc;
	std::shared_ptr<spdlog::logger> m_log;
};

class generator

	: public worker
{
public:
	generator(boost::asio::io_context& ioc,
			  std::shared_ptr<spdlog::logger>& console,
			  const nats_asio::iconnection_sptr& conn,
			  const std::string& topic,
			  int stats_interval,
			  int publish_interval_ms);

	void publish(boost::asio::yield_context ctx);

private:
	int m_publish_interval_ms;
	std::string m_topic;
	nats_asio::iconnection_sptr m_conn;
};

class grubber : public worker
{
public:
	grubber(boost::asio::io_context& ioc,
			std::shared_ptr<spdlog::logger>& console,
			int stats_interval,
			bool print_to_stdout);

	void on_message(nats_asio::string_view,
					nats_asio::optional<nats_asio::string_view>,
					const char* raw,
					std::size_t /*n*/,
					nats_asio::ctx);

private:
	bool m_print_to_stdout;
};

int main(int argc, char* argv[])
{
	try {
		cxxopts::Options options(argv[0], " - filters command line options");
		nats_asio::connect_config conf;
		conf.address = "127.0.0.1";
		conf.port = 4222;
		conf.ssl = true;
		std::string username;
		std::string password;
		std::string mode;
		std::string topic;
		int stats_interval = 1;
		int publish_interval = -1;
		bool print_to_stdout = false;
		options.add_options()("h,help", "Print help")("d,debug", "Enable debugging")(
			"address",
			"Address of NATS server",
			cxxopts::value<std::string>(
				conf.address))("port",
							   "Port of NATS server",
							   cxxopts::value<uint16_t>(
								   conf.port))("user",
											   "Username",
											   cxxopts::value<std::string>(
												   username))("pass",
															  "Password",
															  cxxopts::value<std::string>(password))(
			"mode", "mode", cxxopts::value<std::string>(mode))("topic",
															   "topic",
															   cxxopts::value<std::string>(topic))(
			"stats_interval",
			"stat interval seconds",
			cxxopts::value<int>(stats_interval))("publish_interval",
												 "publish interval seconds in ms",
												 cxxopts::value<int>(
													 publish_interval))("print",
																		"print messages to stdout",
																		cxxopts::value<bool>(
																			print_to_stdout));
		options.parse_positional({"mode"});
		auto result = options.parse(argc, argv);

		if (result.count("help")) {
			std::cout << options.help() << std::endl;
			return 0;
		}

		if (result.count("mode") == 0) {
			std::cerr << "Please specify mode" << std::endl;
			return 1;
		}

		if (topic.empty()) {
			topic = "topic";
		}

		mode = result["mode"].as<std::string>();

		if (mode != grub_mode && mode != gen_mode) {
			std::cerr << fmt::format("Invalid mode. Could be `{}` or `{}`", grub_mode, gen_mode)
					  << std::endl;
			return 1;
		}

		auto m = mode::generator;

		if (mode == grub_mode) {
			m = mode::grubber;
			publish_interval = -1;
		} else {
			if (publish_interval < 0) {
				publish_interval = 1000;
			}
		}

		auto console = spdlog::stdout_color_mt("console");

		if (result.count("debug")) {
			console->set_level(spdlog::level::debug);
		}

		boost::asio::io_context ioc;
		std::shared_ptr<grubber> grub_ptr;
		std::shared_ptr<generator> gen_ptr;

		if (m == mode::grubber) {
			grub_ptr = std::make_shared<grubber>(ioc, console, stats_interval, print_to_stdout);
		}

		nats_asio::iconnection_sptr conn;
		conn = nats_asio::create_connection(
			console,
			ioc,
			[&](nats_asio::iconnection& /*c*/, nats_asio::ctx ctx) {
				console->info("on connected");

				if (m == mode::grubber) {
					using namespace std::placeholders;
					auto r = conn->subscribe(
						topic,
						{},
						std::bind(&grubber::on_message, grub_ptr.get(), _1, _2, _3, _4, _5),
						ctx);

					if (r.second.failed()) {
						console->error("failed to subscribe with error: {}", r.second.error());
					}
				}
			},
			[&console](nats_asio::iconnection&, nats_asio::ctx) {
				console->info("on disconnected");
			});
		conn->start(conf);

		if (m == mode::generator) {
			gen_ptr = std::make_shared<generator>(ioc,
												  console,
												  conn,
												  topic,
												  stats_interval,
												  publish_interval);
		}

		ioc.run();
	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return 1;
	} catch (...) {
		std::cerr << "unknown exception" << std::endl;
		return 1;
	}

	return 0;
}

worker::worker(boost::asio::io_context& ioc,
			   std::shared_ptr<spdlog::logger>& console,
			   int stats_interval)
	: m_stats_interval(stats_interval)
	, m_counter(0)
	, m_ioc(ioc)
	, m_log(console)
{
	boost::asio::spawn(ioc, std::bind(&worker::stats_timer, this, std::placeholders::_1));
}
void worker::stats_timer(boost::asio::yield_context ctx)
{
	boost::asio::deadline_timer timer(m_ioc);
	boost::system::error_code error;

	for (;;) {
		m_log->info("stats: messages {} during {} seconds", m_counter, m_stats_interval);
		timer.expires_from_now(boost::posix_time::seconds(1 * m_stats_interval));
		m_counter = 0;
		timer.async_wait(ctx[error]);

		if (error.failed()) {
			return;
		}
	}
}
generator::generator(boost::asio::io_context& ioc,
					 std::shared_ptr<spdlog::logger>& console,
					 const nats_asio::iconnection_sptr& conn,
					 const std::string& topic,
					 int stats_interval,
					 int publish_interval_ms)
	: worker(ioc, console, stats_interval)
	, m_publish_interval_ms(publish_interval_ms)
	, m_topic(topic)
	, m_conn(conn)
{
	if (m_publish_interval_ms >= 0) {
		boost::asio::spawn(ioc, std::bind(&generator::publish, this, std::placeholders::_1));
	}
}
void generator::publish(boost::asio::yield_context ctx)
{
	boost::asio::deadline_timer timer(m_ioc);
	boost::system::error_code error;
	const std::string msg("{\"value\": 123}");

	for (;;) {
		auto s = m_conn->publish(m_topic, msg.data(), msg.size(), {}, ctx);

		if (s.failed()) {
			m_log->error("publish failed with error {}", s.error());
		} else {
			m_counter++;
		}

		timer.expires_from_now(boost::posix_time::milliseconds(1 * m_publish_interval_ms));
		timer.async_wait(ctx[error]);

		if (error.failed()) {
			return;
		}
	}
}
grubber::grubber(boost::asio::io_context& ioc,
				 std::shared_ptr<spdlog::logger>& console,
				 int stats_interval,
				 bool print_to_stdout)
	: worker(ioc, console, stats_interval)
	, m_print_to_stdout(print_to_stdout)
{
}
void grubber::on_message(boost::string_view, nats_asio::optional<boost::string_view>, const char* raw, std::size_t, nats_asio::ctx)
{
	m_counter++;

	if (m_print_to_stdout) {
		std::cout << raw << std::endl;
	}
}
