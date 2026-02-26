#ifndef CNC_RED_ALERT_TD_MENUS_H_
#define CNC_RED_ALERT_TD_MENUS_H_

#include "sdllib/wwstd.h"

struct MenuConfig {
  int x = 1;
  int y = 3;
  int item_width = 12;
  int item_count = 3;
  int selected = 0;
  int normal_color = WHITE;
  int highlight_color = PINK;
};

void Setup_Menu(const MenuConfig& menu, const char* labels[],
                unsigned long visible_items, int bit_offset, int line_spacing);
int Check_Menu(MenuConfig& menu, const char* text[], long field, int index);
int Do_Menu(const char** strings, bool blue);
extern int UnknownKey;
int Main_Menu(unsigned long timeout);

#endif  // CNC_RED_ALERT_TD_MENUS_H_
