#include <nats_asio/common.hpp>

namespace nats_asio {

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
