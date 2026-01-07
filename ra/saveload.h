#ifndef CNC_RED_ALERT_RA_SAVELOAD_H_
#define CNC_RED_ALERT_RA_SAVELOAD_H_

#include <cstddef>

#include "ra/defines.h"
#include "tech/pipe.h"
#include "tech/straw.h"
#include "tech/wwfile.h"

bool Load_Misc_Values(Straw &file);
bool Save_Misc_Values(Pipe &file);
bool Load_MPlayer_Values(Straw &file);
bool Save_MPlayer_Values(Pipe &file);
bool Get_Savefile_Info(int id, char *buf, size_t buf_size, unsigned *scenp,
                       HousesType *housep);
bool Load_Game(int id);
bool Read_Object(void *ptr, int base_size, int class_size, FileClass &file,
                 void *vtable);
bool Save_Game(int id, char const *descr, bool bargraph = false);
bool Write_Object(void *ptr, int class_size, FileClass &file);
void Code_All_Pointers(void);
void Decode_All_Pointers(void);
void Dump(void);

#endif  // CNC_RED_ALERT_RA_SAVELOAD_H_
