#ifndef CNC_RED_ALERT_TD_MPLAYER_H_
#define CNC_RED_ALERT_TD_MPLAYER_H_

#include "td/defines.h"

inline PlayerColorType MPlayerID_To_ColorIndex(unsigned short id) {
  return (PlayerColorType)(id >> 4);
}
inline HousesType MPlayerID_To_HousesType(unsigned short id) {
  return ((HousesType)(id & 0x000f));
}
inline unsigned short Build_MPlayerID(int c_idx, HousesType htype) {
  return ((c_idx << 4) | htype);
}

GameType Select_MPlayer_Game(void);
void Read_MultiPlayer_Settings(void);
void Write_MultiPlayer_Settings(void);
void Read_Scenario_Descriptions(void);
void Free_Scenario_Descriptions(void);
void Computer_Message(void);
int Surrender_Dialog(void);

#endif  // CNC_RED_ALERT_TD_MPLAYER_H_
