#ifndef CNC_RED_ALERT_TD_SAVELOAD_H_
#define CNC_RED_ALERT_TD_SAVELOAD_H_

class ArchiveReader;
class ArchiveWriter;

#include <cstdint>

#include "td/defines.h"
#include "td/type.h"

// Field-wise save body, including map, layers and globals.
inline constexpr int32_t kSaveGameVersion = 10;

bool Load_Misc_Values(ArchiveReader& file);
bool Save_Misc_Values(ArchiveWriter& file);
bool Get_Savefile_Info(int id, char* buf, unsigned* scenp, HousesType* housep);
bool Load_Game(int id);
bool Save_Game(int id, const char* descr);
void Dump();

#endif  // CNC_RED_ALERT_TD_SAVELOAD_H_
