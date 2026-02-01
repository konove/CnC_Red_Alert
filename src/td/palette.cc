#include "td/palette.h"

#include <algorithm>
#include <cstring>

#include "sdllib/gbuffer.h"
#include "sdllib/timer.h"
#include "sdllib/ww_win.h"

unsigned char CurrentPalette[3 * 256];

void Fade_Palette_To(unsigned char* palette, int fade, void (*callback)()) {
  if (fade) {
    // fade to new palette
    auto start_time = TickCount.Time();

    unsigned char fade_palette[256 * 3];

    while (true) {
      int cur_time = std::min<int>(TickCount.Time() - start_time, fade);

      const unsigned char* old_ptr = CurrentPalette;
      const unsigned char* new_ptr = palette;
      unsigned char* out_ptr = fade_palette;

      for (int c = 0; c < 256 * 3; c++) {
        int new_val = *new_ptr++ & 0x3F;
        int old_val = *old_ptr++ & 0x3F;
        *out_ptr++ = old_val + (new_val - old_val) * cur_time / fade;
      }

      Do_Set_Palette(fade_palette);
      if (callback) {
        callback();
      }
#ifdef PORTABLE
      else {  // make sure we actually display the fade
        Video_End_Frame();
      }
#endif

      if (cur_time == fade) {
        break;
      }
    }
  }

  Set_Palette(palette);
}

void Set_Palette(void* palette) {
  memcpy(CurrentPalette, palette, 256 * 3);
  Do_Set_Palette(palette);
}
