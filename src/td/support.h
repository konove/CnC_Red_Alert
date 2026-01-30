#ifndef CNC_RED_ALERT_TD_SUPPORT_H_
#define CNC_RED_ALERT_TD_SUPPORT_H_

#include <cstdint>
#include <cstdlib>
#include <source_location>

#include "sdllib/include/gbuffer.h"
#include "sdllib/include/misc.h"
#include "td/config.h"
#include "td/monoc.h"

void* Conquer_Build_Fading_Table(void const* palette, void* dest, int color,
                                 int frac);
void Fat_Put_Pixel(int x, int y, std::uint8_t color, int size,
                   GraphicViewPortClass&);
void strtrim(char* buffer);

// Null pointer check that fires only in cheat-key builds.
inline void Check_Ptr(const void* ptr, std::source_location loc =
                                           std::source_location::current()) {
  if constexpr (config::kCheatKeysEnabled) {
    if (!ptr) {
      Mono_Clear_Screen();
      Mono_Printf("NULL Pointer, Module:%s, line:%d!\n", loc.file_name(),
                  static_cast<int>(loc.line()));
      Prog_End();
      exit(EXIT_SUCCESS);
    }
  }
}

#endif  // CNC_RED_ALERT_TD_SUPPORT_H_
