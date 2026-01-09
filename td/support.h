#ifndef CNC_RED_ALERT_TD_SUPPORT_H_
#define CNC_RED_ALERT_TD_SUPPORT_H_

#include <cstdint>

#include "sdllib/include/gbuffer.h"

void *Conquer_Build_Fading_Table(void const *palette, void *dest, int color,
                                 int frac);
void Fat_Put_Pixel(int x, int y, std::uint8_t color, int size,
                   GraphicViewPortClass &);
void strtrim(char *buffer);

#ifdef CHEAT_KEYS
#define Check_Ptr(ptr, file, line)                                    \
  {                                                                   \
    if (!ptr) {                                                       \
      Mono_Clear_Screen();                                            \
      Mono_Printf("NULL Pointer, Module:%s, line:%d!\n", file, line); \
      Prog_End();                                                     \
      exit(EXIT_SUCCESS);                                             \
    }                                                                 \
  }
#else
#define Check_Ptr(ptr, file, line)
#endif

#endif  // CNC_RED_ALERT_TD_SUPPORT_H_
