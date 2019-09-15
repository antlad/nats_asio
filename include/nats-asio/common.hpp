#pragma once

#include <stdexcept>
#include <optional>

#include <fmt/format.h>

namespace nats_asio {

class status {
public:
    status() = default;

    status(const std::string& error);

    virtual ~status() = default;

    bool failed() const;

    std::string error() const;
private:
    std::optional<std::string> m_error;
};


class detailed_exception
    : public std::exception {
public:

    detailed_exception(const std::string& msg , const std::string& file, int line);

    virtual const char* what() const _GLIBCXX_USE_NOEXCEPT override;
private:
    std::string m_msg;
};

}

#define THROW_EXP(MSG) \
    throw nats_asio::detailed_exception(MSG, __FILE__, __LINE__);
