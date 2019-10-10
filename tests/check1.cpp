#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "parser.hpp"
#include <nats_asio/defs.hpp>

#include <sstream>
#include <iostream>

using namespace nats_asio;

struct parser_mock : public parser_observer
{
	MOCK_METHOD1(consumed, void(std::size_t));
	MOCK_METHOD1(on_ok, void(ctx));
	MOCK_METHOD1(on_pong, void(ctx));
	MOCK_METHOD1(on_ping, void(ctx));
	MOCK_METHOD2(on_error, void(string_view, ctx));
	MOCK_METHOD2(on_info, void(string_view, ctx));
	MOCK_METHOD5(on_message, void(string_view, string_view, optional<string_view>, std::size_t, ctx));
};


void async_process(const std::function<void(ctx c)>& f)
{
	boost::asio::io_context ioc;
	boost::asio::spawn(ioc, std::bind(f, std::placeholders::_1));
	ioc.run();
}

TEST(small_messages, ping)
{
	parser_mock m;
	std::string payload("PING\r\n");
	std::string header;
	EXPECT_CALL(m, on_ping(testing::_)).Times(1);
	async_process([&](auto c)
	{
		std::stringstream ss(payload);
		auto s = parse_header(header, ss, &m, c);
		EXPECT_EQ(false, s.failed());
	});
}

TEST(small_messages, pong)
{
	parser_mock m;
	std::string payload("PONG\r\n");
	std::string header;
	EXPECT_CALL(m, on_pong(testing::_)).Times(1);
	async_process([&](auto c)
	{
		std::stringstream ss(payload);
		auto s = parse_header(header, ss, &m, c);
		EXPECT_EQ(false, s.failed());
	});
}

TEST(small_messages, ok)
{
	parser_mock m;
	std::string payload("+OK\r\n");
	std::string header;
	EXPECT_CALL(m, on_ok(testing::_)).Times(1);
	async_process([&](auto c)
	{
		std::stringstream ss(payload);
		auto s = parse_header(header, ss, &m, c);
		EXPECT_EQ(false, s.failed());
	});
}

TEST(payload_messages, err)
{
	parser_mock m;
	string_view msg("some big error");
	auto payload = fmt::format("-ERR {}\r\n", msg);
	std::string header;
	EXPECT_CALL(m, on_error(string_view(msg), testing::_)) .Times(1);
	async_process([&](auto c)
	{
		std::stringstream ss(payload);
		auto s = parse_header(header, ss, &m, c);
		EXPECT_EQ(false, s.failed());
	});
}

TEST(payload_messages, info)
{
	parser_mock m;
	string_view info_msg(R"({"verbose":false,"pedantic":false,"tls_required":false})");
	EXPECT_CALL(m, on_info(info_msg, testing::_)) .Times(1);
	auto payload = fmt::format("INFO {}\r\n", info_msg);
	std::string header;
	async_process([&](auto c)
	{
		std::stringstream ss(payload);
		auto s = parse_header(header, ss, &m, c);
		EXPECT_EQ(false, s.failed());
	});
}

TEST(payload_messages, info_with_overflow)
{
	parser_mock m;
	string_view info_msg(R"({"verbose":false,"pedantic":false,"tls_required":false})");
	EXPECT_CALL(m, on_info(info_msg, testing::_)) .Times(1);
	std::string header;
	auto payload = fmt::format("INFO {}\r\n", info_msg);
	auto payload_over = payload + "-ERR abrakadabra\r\n";
	async_process([&](auto c)
	{
		std::stringstream ss(payload);
		auto s = parse_header(header, ss, &m, c);
		EXPECT_EQ(false, s.failed());
	});
}

TEST(payload_messages, on_message)
{
	parser_mock m;
	const char* msg = R"(subscription payload)";
	auto msg_size = strlen(msg);
	string_view sid("6789654");
	string_view subject("sub1.1");
	string_view reply_to("some_reply_to");
	std::string header;
	std::string payload = fmt::format("MSG {} {} {}\r\n{}\r\n", subject, sid, msg_size, msg);
	std::string payload2 = fmt::format("MSG {} {} {} {}\r\n{}\r\n", subject, sid, reply_to, msg_size, msg);
	EXPECT_CALL(m, on_message(subject, sid, optional<string_view>(), msg_size, testing::_)).Times(1);
	EXPECT_CALL(m, on_message(subject, sid, optional<string_view>(reply_to), msg_size, testing::_)).Times(1);
	EXPECT_CALL(m, consumed(msg_size + 2)).Times(2);
	async_process([&](auto c)
	{
		std::stringstream ss(payload);
		auto s1 = parse_header(header, ss, &m, c);
		EXPECT_EQ(false, s1.failed());
		std::stringstream ss2(payload2);
		auto  s2 = parse_header(header, ss2, &m, c);
		EXPECT_EQ(false, s2.failed());
	});
}


TEST(payload_messages, on_message_binary)
{
	parser_mock m;
	string_view sid("6789654");
	string_view subject("sub1.1");
	std::size_t msg_size = 10;
	std::vector<char> binary_payload(msg_size);

	for (std::size_t i = 0; i < msg_size; ++i)
	{
		binary_payload[i] = static_cast<char>(i);
	}

	std::vector<char> buffer;
	auto payload_header = fmt::format("MSG {} {} {}\r\n", subject, sid, msg_size);
	std::copy(payload_header.begin(), payload_header.end(), std::back_inserter(buffer));
	std::copy(binary_payload.begin(), binary_payload.end(), std::back_inserter(buffer));
	buffer.push_back('\r');
	buffer.push_back('\n');
	std::string header;
	EXPECT_CALL(m, on_message(subject, sid, optional<string_view>(), msg_size, testing::_)).Times(1);
	EXPECT_CALL(m, consumed(msg_size + 2)).Times(1);
	async_process([&](auto c)
	{
		std::stringstream ss2(payload_header);
		auto s1 = parse_header(header, ss2, &m, c);
		EXPECT_EQ(false, s1.failed());
	});
}


TEST(payload_messages, on_message_not_full_no_sep)
{
	parser_mock m;
	std::string payload("MSG abra abra");
	std::string header;
	async_process([&](auto c)
	{
		std::stringstream ss(payload);
		auto s1 = parse_header(header, ss, &m, c);
		EXPECT_EQ(true, s1.failed());
	});
}

TEST(payload_messages, on_message_not_full)
{
	parser_mock m;
	const char* msg = R"(subscription payload)";
	auto msg_size = strlen(msg);
	string_view sid("6789654");
	string_view subject("sub1.1");
	std::string header;
	auto payload = fmt::format("MSG {} {} {}\r\n{}", subject, sid, msg_size, msg);
	EXPECT_CALL(m, on_message(subject, sid, optional<string_view>(), msg_size, testing::_)).Times(1);
	EXPECT_CALL(m, consumed(msg_size + 2)).Times(1);
	async_process([&](auto c)
	{
		std::stringstream ss(payload);
		auto s1 = parse_header(header, ss, &m, c);
		EXPECT_EQ(false, s1.failed());
	});
}
