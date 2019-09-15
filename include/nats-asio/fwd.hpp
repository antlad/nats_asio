#pragma once

#include <memory>

namespace nats_asio {
struct options;

struct subscription;
typedef std::shared_ptr<subscription> subscription_sptr;

class connection;
typedef std::shared_ptr<connection> connection_sptr;

}

namespace boost {
namespace asio {
namespace ip {

class address;
class tcp;

} // namespace ip
} // namespace asio
} // namespace boost
