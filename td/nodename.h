#ifndef CNC_RED_ALERT_TD_NODENAME_H_
#define CNC_RED_ALERT_TD_NODENAME_H_

#include "td/defines.h"
#include "td/ipxaddr.h"

// Node for lists of available games & players in multiplayer.
// 'Game' structure is for games; 'Player' structure for players.
typedef struct NodeNameTag {
  char Name[MPLAYER_NAME_MAX];
  IPXAddressClass Address;
  union {
    struct {
      int Version;
      unsigned char IsOpen;
      unsigned long LastTime;
    } Game;
    struct {
      HousesType House;
      unsigned char Color;
    } Player;
  };
} NodeNameType;

#endif  // CNC_RED_ALERT_TD_NODENAME_H_
