#ifndef CNC_RED_ALERT_TD_MENUS_H_
#define CNC_RED_ALERT_TD_MENUS_H_

#include <cstdint>
#include <span>

#include "sdllib/wwstd.h"

struct MenuConfig {
  int x = 1;
  int y = 3;
  int item_width = 12;
  int item_count = 3;
  int selected = 0;
  int normal_color = kWhite;
  int highlight_color = kPink;
};

void Setup_Menu(const MenuConfig& menu, std::span<const char* const> labels,
                uint32_t visible_items, int bit_offset, int line_spacing);
int Check_Menu(MenuConfig& menu, std::span<const char* const> text,
               uint32_t field, int index);
int Do_Menu(std::span<const char* const> strings, bool blue);
extern int UnknownKey;
int Main_Menu(int timeout);

#endif  // CNC_RED_ALERT_TD_MENUS_H_
