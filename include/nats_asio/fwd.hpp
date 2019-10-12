#pragma once

#include <memory>
#include <boost/optional.hpp>
#include <boost/utility/string_view_fwd.hpp>

namespace nats_asio {
struct options;


struct subscription;
typedef std::shared_ptr<subscription> subscription_sptr;

//class connection;
//typedef std::shared_ptr<connection> connection_sptr;

using boost::optional;
using boost::string_view;

}

namespace boost {
namespace asio {
namespace ip {

class address;
class tcp;

} // namespace ip
} // namespace asio
} // namespace boost
