#ifndef CNC_RED_ALERT_TD_SAVELOAD_H_
#define CNC_RED_ALERT_TD_SAVELOAD_H_

class ArchiveReader;
class ArchiveWriter;

#include <cstdint>

#include "td/defines.h"
#include "td/type.h"

// Field-wise object heaps; map and globals retain their staged legacy format.
inline constexpr int32_t kSaveGameVersion = 9;

bool Load_Misc_Values(ArchiveReader& file);
bool Save_Misc_Values(ArchiveWriter& file);
bool Get_Savefile_Info(int id, char* buf, unsigned* scenp, HousesType* housep);
bool Load_Game(int id);
bool Save_Game(int id, char* descr);
void Code_All_Pointers();
void Decode_All_Pointers();
void Dump();

// Legacy map/score helpers remain until those formats migrate.
bool Read_Object(void* ptr, int base_size, int class_size, ArchiveReader& file,
                 void* vtable);

bool Write_Object(void* ptr, int class_size, ArchiveWriter& file);

#endif  // CNC_RED_ALERT_TD_SAVELOAD_H_
