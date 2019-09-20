#include <nats-asio/common.hpp>

namespace nats_asio {

detailed_exception::detailed_exception(const std::string& msg, const std::string& file, int line)
    : m_msg(fmt::format("exception:{}, file: {}, line: {}", msg, file, line))
{}

const char* detailed_exception::what() const noexcept
{
    return m_msg.c_str();
}

status::status(const std::string& error)
    : m_error(error)
{
}

bool status::failed() const
{
    return m_error.has_value();
}

std::string status::error() const
{
    if (!m_error.has_value())
        return {};

    return m_error.value();
}

}
