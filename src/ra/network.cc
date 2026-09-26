// Network's constructor and destructor, defined where the transports it owns
// are complete types.

#include "ra/network.h"

#include "engine/net/serial/modemreg.h"  // IWYU pragma: keep
#include "ra/wolapiob.h"                 // IWYU pragma: keep
#include "ra/wsproto.h"                  // IWYU pragma: keep

Network::Network() = default;
Network::~Network() = default;
