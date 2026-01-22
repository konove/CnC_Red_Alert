#ifndef CNC_RED_ALERT_TD_MENUS_H_
#define CNC_RED_ALERT_TD_MENUS_H_

void Setup_Menu(int menu, char const* text[], unsigned long field, int index,
                int skip);
int Check_Menu(int menu, char const* text[], char* selection, long field,
               int index);
int Do_Menu(char const** strings, bool blue);
extern int UnknownKey;
int Main_Menu(unsigned long timeout);

#endif  // CNC_RED_ALERT_TD_MENUS_H_
