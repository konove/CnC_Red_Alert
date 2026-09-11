#ifndef CNC_RED_ALERT_TD_SAVELOAD_H_
#define CNC_RED_ALERT_TD_SAVELOAD_H_

class ArchiveReader;
class ArchiveWriter;

#include <cstdint>

#include "td/defines.h"
#include "td/type.h"

// Raw-object archive checkpoint; older layout-derived versions are rejected.
inline constexpr int32_t kSaveGameVersion = 6;

bool Load_Misc_Values(ArchiveReader& file);
bool Save_Misc_Values(ArchiveWriter& file);
bool Get_Savefile_Info(int id, char* buf, unsigned* scenp, HousesType* housep);
bool Load_Game(int id);
bool Read_Object(void* ptr, int base_size, int class_size, ArchiveReader& file,
                 void* vtable);
bool Save_Game(int id, char* descr);
bool Write_Object(void* ptr, int class_size, ArchiveWriter& file);
void* Get_VTable(void* ptr, int base_size);
void Code_All_Pointers();
void Decode_All_Pointers();
void Dump();
void Set_VTable(void* ptr, int base_size, void* vtable);

#endif  // CNC_RED_ALERT_TD_SAVELOAD_H_
