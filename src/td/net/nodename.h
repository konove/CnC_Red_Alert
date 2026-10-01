#ifndef CNC_RED_ALERT_TD_NET_NODENAME_H_
#define CNC_RED_ALERT_TD_NET_NODENAME_H_

#include <cstdint>

#include "td/core/defines.h"
#include "td/net/ipxaddr.h"

namespace td {

// Node for lists of available games & players in multiplayer.
// 'Game' structure is for games; 'Player' structure for players.
struct NodeNameTag {
  char Name[MPLAYER_NAME_MAX]{};
  IPXAddressClass Address;
  union {
    struct {
      int Version;
      unsigned char IsOpen;
      int64_t LastTime;
    } Game{};
    struct {
      HousesType House;
      unsigned char Color;
    } Player;
  };
};
using NodeNameType = NodeNameTag;

}  // namespace td

#endif  // CNC_RED_ALERT_TD_NET_NODENAME_H_
