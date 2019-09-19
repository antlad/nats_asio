#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <nats-asio/parser.hpp>

#include <iostream>

using namespace nats_asio;

struct parser_mock : public parser_observer {
    MOCK_METHOD0(on_ok, void());
    MOCK_METHOD0(on_pong, void());
    MOCK_METHOD0(on_ping, void());
    MOCK_METHOD1(on_error, void(std::string_view));
    MOCK_METHOD1(on_info, void(std::string_view));
    MOCK_METHOD5(on_message, void(std::string_view, std::string_view, std::optional<std::string_view>, const char*, std::size_t));
};

TEST(small_messages, ping)
{
    parser_mock m;
    constexpr std::string_view payload = "PING\r\n";
    EXPECT_CALL(m, on_ping()).Times(1);
    auto [n, s] = parse_message(payload, &m);
    EXPECT_EQ(payload.size(), n);
    EXPECT_EQ(false, s.failed());
}

TEST(small_messages, pong)
{
    parser_mock m;
    constexpr std::string_view payload = "PONG\r\n";
    EXPECT_CALL(m, on_pong()).Times(1);
    auto [n, s] = parse_message(payload, &m);
    EXPECT_EQ(payload.size(), n);
    EXPECT_EQ(false, s.failed());
}

TEST(small_messages, ok)
{
    parser_mock m;
    constexpr std::string_view payload = "+OK\r\n";
    EXPECT_CALL(m, on_ok()).Times(1);
    auto [n, s] = parse_message(payload, &m);
    EXPECT_EQ(payload.size(), n);
    EXPECT_EQ(false, s.failed());
}

TEST(small_messages, err)
{
    parser_mock m;
    constexpr std::string_view msg = "some big error";
    auto payload = fmt::format("-ERR {}\r\n", msg);
    EXPECT_CALL(m, on_error(std::string_view(msg))) .Times(1);
    auto [n, s] = parse_message(payload, &m);
    EXPECT_EQ(payload.size(), n);
    EXPECT_EQ(false, s.failed());
}

TEST(small_messages, info)
{
//    parser_mock m;
//    constexpr std::string_view info_msg = R"({"verbose":false,"pedantic":false,"tls_required":false})";
//    EXPECT_CALL(m, on_error(std::string_view(info_msg))) .Times(1);
//    auto [n, s] = parse_message(fmt::format("INFO {}\r\n", info_msg), &m);
//    EXPECT_EQ(false, s.failed());
}
