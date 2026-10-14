#ifndef CNC_RED_ALERT_TD_NET_NODENAME_H_
#define CNC_RED_ALERT_TD_NET_NODENAME_H_

#include <cstdint>

#include "engine/net/ipxaddr.h"
#include "td/core/defines.h"

namespace td {

// A name and an address on the network: what every list node has.
struct NodeName {
  char Name[MPLAYER_NAME_MAX]{};
  engine::net::IPXAddressClass Address;
};

// A game announced on the network, in Network::games().
struct GameNode : NodeName {
  int Version = 0;
  unsigned char IsOpen = 0;  // is the game open?
  int64_t LastTime = 0;      // last time we heard from it
};

// A player in this game, in Network::players().
struct PlayerNode : NodeName {
  HousesType House{};
  unsigned char Color = 0;
};

}  // namespace td

#endif  // CNC_RED_ALERT_TD_NET_NODENAME_H_
