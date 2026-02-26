#ifndef CNC_RED_ALERT_TD_MENUS_H_
#define CNC_RED_ALERT_TD_MENUS_H_

void Setup_Menu(int menu_index, const char* labels[], unsigned long visible_items,
                int bit_offset, int line_spacing);
int Check_Menu(int menu, const char* text[], char* selection, long field,
               int index);
int Do_Menu(const char** strings, bool blue);
extern int UnknownKey;
int Main_Menu(unsigned long timeout);

#endif  // CNC_RED_ALERT_TD_MENUS_H_
