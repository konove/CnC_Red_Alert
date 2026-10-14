// Network's constructor and destructor, defined where the transports it owns
// are complete types.

#include "ra/net/network.h"

#include <cstddef>
#include <span>

#include "absl/log/check.h"
#include "base/buffer.h"
#include "engine/net/ipxaddr.h"
#include "engine/net/ipxmgr.h"
#include "engine/net/serial/modemreg.h"  // IWYU pragma: keep
#include "ra/core/defines.h"
#include "ra/map/world.h"
#include "ra/net/event.h"
#include "ra/net/session.h"
#include "ra/net/wsproto.h"
#include "ra/objects/house.h"  // IWYU pragma: keep - used by a DCHECK() below.
#include "ra/wol/wolapiob.h"   // IWYU pragma: keep

namespace ra {

namespace {

// A private packet from an unknown address may come from a player whose
// address changed (a dial-up reconnect, a NAT remapping): if it is that
// player's frame sync, the connection follows the player to the new address.
void FollowMovedPlayer(engine::net::IPXManagerClass& ipx,
                       std::span<const std::byte> payload,
                       const engine::net::IPXAddressClass& from) {
  if (payload.size() < offsetof(EventClass, Data)) {
    return;
  }
  EventClass event;
  base::CopyBytes(base::ObjectBytes(event), payload,
                  offsetof(EventClass, Data));
  if (event.Type != EventClass::FRAMESYNC) {
    return;
  }

  const int id = event.ID;
  DCHECK(id != ThePlayer()->ID);
  for (int k = 1; k < TheSession().Players.Count(); k++) {
    if (TheSession().Players.at(k)->ID == static_cast<HousesType>(id)) {
      // Null until Create_Connections() has run. Dropping this packet is
      // fine: the next frame sync arrives at the new address.
      if (engine::net::IPXAddressClass* address = ipx.Connection_Address(id)) {
        TheSession().Players.at(k)->Address = from;
        *address = from;
      }
      return;
    }
  }
}

}  // namespace

Network::Network() { ipx_.Set_Unknown_Sender_Handler(&FollowMovedPlayer); }
Network::~Network() = default;

bool Network::WinsockAvailable() {
  // A local probe: the interface the manager sends through is opened later,
  // by the protocol setup code.
  WinsockInterfaceClass probe;
  return probe.Init();
}

}  // namespace ra
