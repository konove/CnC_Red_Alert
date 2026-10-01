#include "engine/net/datagram_transport.h"

namespace engine::net {

// Out of line, so the vtable is emitted once.
DatagramTransport::~DatagramTransport() = default;

}  // namespace engine::net
