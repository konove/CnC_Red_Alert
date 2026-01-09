#ifndef CNC_RED_ALERT_TD_SAVELOAD_H_
#define CNC_RED_ALERT_TD_SAVELOAD_H_

#include "td/defines.h"
#include "tech/wwfile.h"
#include "td/type.h"

bool Load_Misc_Values(FileClass &file);
bool Save_Misc_Values(FileClass &file);
bool Get_Savefile_Info(int id, char *buf, unsigned *scenp, HousesType *housep);
bool Load_Game(int id);
bool Read_Object(void *ptr, int base_size, int class_size, FileClass &file,
                 void *vtable);
bool Save_Game(int id, char *descr);
bool Write_Object(void *ptr, int class_size, FileClass &file);
TARGET TechnoType_To_Target(TechnoTypeClass const *ptr);
TechnoTypeClass const *Target_To_TechnoType(TARGET target);
void *Get_VTable(void *ptr, int base_size);
void Code_All_Pointers(void);
void Decode_All_Pointers(void);
void Dump(void);
void Set_VTable(void *ptr, int base_size, void *vtable);

#endif  // CNC_RED_ALERT_TD_SAVELOAD_H_
