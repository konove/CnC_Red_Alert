#ifndef CNC_RED_ALERT_TD_MPLAYER_H_
#define CNC_RED_ALERT_TD_MPLAYER_H_

#include <cstdint>

#include "td/defines.h"

inline PlayerColorType MPlayerID_To_ColorIndex(uint16_t id) {
  return static_cast<PlayerColorType>(id >> 4);
}
inline HousesType MPlayerID_To_HousesType(uint16_t id) {
  return static_cast<HousesType>(id & 0x000f);
}
inline uint16_t Build_MPlayerID(int c_idx, HousesType htype) {
  return static_cast<uint16_t>(static_cast<unsigned>(c_idx) << 4 |
                               static_cast<unsigned>(htype));
}

GameType Select_MPlayer_Game();
void Read_MultiPlayer_Settings();
void Write_MultiPlayer_Settings();
void Read_Scenario_Descriptions();
void Free_Scenario_Descriptions();
void Computer_Message();
int Surrender_Dialog();

#endif  // CNC_RED_ALERT_TD_MPLAYER_H_
