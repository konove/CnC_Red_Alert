// Network's constructor and destructor, defined where the transports it owns
// are complete types.

#include "ra/net/network.h"

#include "engine/net/serial/modemreg.h"  // IWYU pragma: keep
#include "ra/net/wsproto.h"              // IWYU pragma: keep
#include "ra/wol/wolapiob.h"             // IWYU pragma: keep

namespace ra {

Network::Network() = default;
Network::~Network() = default;

}  // namespace ra
