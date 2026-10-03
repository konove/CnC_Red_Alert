#ifndef CNC_RED_ALERT_TD_NET_MPLAYER_H_
#define CNC_RED_ALERT_TD_NET_MPLAYER_H_

#include <cstdint>

#include "td/core/defines.h"

namespace engine::file {
class IniFile;
}  // namespace engine::file

namespace td {

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
// Sets the player's handle, colour and side, the serial defaults, the init
// strings, the phone book and the sync-bug trap from `ini`. The two lists
// are emptied first. Read_MultiPlayer_Settings() reads them from CONQUER.INI.
void ReadMultiPlayerSettings(const engine::file::IniFile& ini);
// Stores those settings in `ini`, which keeps what else it holds, and replaces
// its [SerialDefaults], [InitStrings] and [PhoneBook] sections.
// Write_MultiPlayer_Settings() saves them to CONQUER.INI.
void WriteMultiPlayerSettings(engine::file::IniFile& ini);
void Read_Scenario_Descriptions();
void Free_Scenario_Descriptions();
void Computer_Message();
int Surrender_Dialog();

}  // namespace td

#endif  // CNC_RED_ALERT_TD_NET_MPLAYER_H_
