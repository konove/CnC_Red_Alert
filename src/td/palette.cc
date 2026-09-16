#include "td/palette.h"

#include <algorithm>
#include <cstddef>
#include <span>

#include "absl/log/check.h"
#include "sdllib/gbuffer.h"
#include "sdllib/timer.h"
#include "sdllib/ww_win.h"
#include "td/externs.h"

unsigned char CurrentPalette[3 * 256];

void Fade_Palette_To(std::span<const unsigned char> palette, int fade,
                     void (*callback)()) {
  CHECK_GE(palette.size(), sizeof(CurrentPalette));
  if (fade > 0) {
    // fade to new palette
    const auto start_time = TickCount.Time();

    unsigned char fade_palette[256 * 3];

    while (true) {
      const int cur_time =
          std::min<int>(static_cast<int>(TickCount.Time() - start_time), fade);

      for (std::size_t c = 0; c < palette.size() && c < sizeof(CurrentPalette);
           ++c) {
        const int new_val = palette[c] & 0x3F;
        const int old_val = std::span(CurrentPalette)[c] & 0x3F;
        (std::span(fade_palette))[c] = static_cast<unsigned char>(
            old_val + ((new_val - old_val) * cur_time / fade));
      }

      Do_Set_Palette(fade_palette);
      if (callback) {
        callback();
      }
      else {  // make sure we actually display the fade
        Video_End_Frame();
      }

      if (cur_time == fade) {
        break;
      }
    }
  }

  Set_Palette(palette);
}

void Set_Palette(std::span<const unsigned char> palette) {
  CHECK_GE(palette.size(), sizeof(CurrentPalette));
  std::ranges::copy(palette.first(sizeof(CurrentPalette)),
                    std::span(CurrentPalette).begin());
  Do_Set_Palette(palette);
}
