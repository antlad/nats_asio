#pragma once

#include <nats-asio/fwd.hpp>

#include <fmt/format.h>
//#include <fmt/

#include <stdexcept>


template <>
struct fmt::formatter<boost::string_view>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const boost::string_view& d, FormatContext& ctx)
    {
        return format_to(ctx.out(), "{}", d.data());
    }
};

namespace nats_asio {

class status {
public:
    status() = default;

    status(const std::string& error);

    template <typename S, typename... Args, typename Char = fmt::char_t<S>>
    status(const S& format_str, Args && ... args)
        : status(fmt::format(format_str, std::forward<Args>(args)...))
    {
    }

    virtual ~status() = default;

    bool failed() const;

    std::string error() const;
private:
    optional<std::string> m_error;
};

class detailed_exception
    : public std::exception {
public:

    detailed_exception(const std::string& msg, const std::string& file, int line);

    virtual const char* what() const noexcept override;
private:
    std::string m_msg;
};


}

#define THROW_EXP(MSG) \
    throw nats_asio::detailed_exception(MSG, __FILE__, __LINE__);
