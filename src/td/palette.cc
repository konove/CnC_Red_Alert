#include "td/palette.h"

#include <algorithm>

#include "engine/gfx/palette.h"
#include "engine/gfx/palette_color.h"
#include "engine/platform/timer.h"
#include "engine/window/display.h"
#include "engine/window/hardware_cursor.h"
#include "td/input.h"

engine::gfx::Palette CurrentPalette;

// Shows `palette` on the display and recolors the mouse cursor, which SDL
// draws in the colors it had when the cursor was made.
static void ShowPalette(const engine::gfx::Palette& palette) {
  engine::window::TheDisplay().SetPalette(palette);
  TheMouse().UpdatePalette();
}

void Fade_Palette_To(const engine::gfx::Palette& palette, int fade,
                     void (*callback)()) {
  if (fade > 0) {
    // fade to new palette
    const auto start_time = engine::platform::SystemTicks();

    engine::gfx::Palette fade_palette;

    while (true) {
      const int cur_time = std::min<int>(
          static_cast<int>(engine::platform::SystemTicks() - start_time), fade);

      // Each gun steps linearly from the current palette's to the new one's.
      const auto blend = [cur_time, fade](const int old_gun,
                                          const int new_gun) {
        const auto six_bits = [](const int gun) {
          return static_cast<int>(static_cast<unsigned>(gun) & 0x3FU);
        };
        const int old_val = six_bits(old_gun);
        const int new_val = six_bits(new_gun);
        return old_val + ((new_val - old_val) * cur_time / fade);
      };
      for (int i = 0; i < engine::gfx::Palette::kColorCount; ++i) {
        const engine::gfx::PaletteColor& from = CurrentPalette.at(i);
        const engine::gfx::PaletteColor& to = palette.at(i);
        fade_palette.at(i) = engine::gfx::PaletteColor::FromGuns(
            blend(from.red_gun(), to.red_gun()),
            blend(from.green_gun(), to.green_gun()),
            blend(from.blue_gun(), to.blue_gun()));
      }

      ShowPalette(fade_palette);
      if (callback) {
        callback();
      }
      else {  // make sure we actually display the fade
        engine::window::TheDisplay().EndFrame();
      }

      if (cur_time == fade) {
        break;
      }
    }
  }

  Set_Palette(palette);
}

void Set_Palette(const engine::gfx::Palette& palette) {
  CurrentPalette = palette;
  ShowPalette(palette);
}
