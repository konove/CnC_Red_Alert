#ifndef CNC_RED_ALERT_RA_SCREENS_MENUS_H_
#define CNC_RED_ALERT_RA_SCREENS_MENUS_H_

#include <cstdint>
#include <span>

class PixelView;

namespace ra {

void Setup_Menu(PixelView& view, int menu, std::span<const char* const> text,
                uint32_t field, int index, int skip);
int Check_Menu(PixelView& view, int menu, std::span<const char* const> text,
               char* selection, uint32_t field, int index);
int Do_Menu(std::span<const char* const> strings, bool blue);
int Main_Menu(int32_t timeout);

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_SCREENS_MENUS_H_
