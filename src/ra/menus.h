#ifndef CNC_RED_ALERT_RA_MENUS_H_
#define CNC_RED_ALERT_RA_MENUS_H_

#include <cstdint>

void Setup_Menu(int menu, const char* text[], uint32_t field, int index,
                int skip);
int Check_Menu(int menu, const char* text[], char* selection, uint32_t field,
               int index);
int Do_Menu(const char** strings, bool blue);
extern int UnknownKey;
int Main_Menu(int32_t timeout);

#endif  // CNC_RED_ALERT_RA_MENUS_H_
