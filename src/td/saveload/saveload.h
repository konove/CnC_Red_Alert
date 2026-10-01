#ifndef CNC_RED_ALERT_TD_SAVELOAD_SAVELOAD_H_
#define CNC_RED_ALERT_TD_SAVELOAD_SAVELOAD_H_

#include <cstdint>

#include "td/core/defines.h"
#include "td/objects/type.h"

class ArchiveReader;
class ArchiveWriter;

namespace td {

// Field-wise save body, including map, layers and globals.
inline constexpr int32_t kSaveGameVersion = 10;

bool Load_Misc_Values(ArchiveReader& file);
bool Save_Misc_Values(ArchiveWriter& file);
bool Get_Savefile_Info(int id, std::span<char> buf, unsigned* scenp,
                       HousesType* housep);
bool Load_Game(int id);
bool Save_Game(int id, const char* descr);
void Dump();

}  // namespace td

#endif  // CNC_RED_ALERT_TD_SAVELOAD_SAVELOAD_H_
