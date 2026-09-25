#include "td/palette.h"

#include <algorithm>
#include <cstddef>
#include <span>

#include "absl/log/check.h"
#include "engine/base/array.h"
#include "engine/platform/timer.h"
#include "engine/window/display.h"

unsigned char CurrentPalette[3 * 256];

void Fade_Palette_To(std::span<const unsigned char> palette, int fade,
                     void (*callback)()) {
  CHECK_GE(palette.size(), sizeof(CurrentPalette));
  if (fade > 0) {
    // fade to new palette
    const auto start_time = SystemTicks();

    unsigned char fade_palette[256 * 3];

    while (true) {
      const int cur_time =
          std::min<int>(static_cast<int>(SystemTicks() - start_time), fade);

      for (std::size_t c = 0; c < palette.size() && c < sizeof(CurrentPalette);
           ++c) {
        const int new_val = base::At(palette, c) & 0x3F;
        const int old_val = base::At(std::span(CurrentPalette), c) & 0x3F;
        base::At((std::span(fade_palette)), c) = static_cast<unsigned char>(
            old_val + ((new_val - old_val) * cur_time / fade));
      }

      TheDisplay().SetPalette(fade_palette);
      if (callback) {
        callback();
      }
      else {  // make sure we actually display the fade
        TheDisplay().EndFrame();
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
  TheDisplay().SetPalette(palette);
}
