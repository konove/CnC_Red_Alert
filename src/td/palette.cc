#include "td/palette.h"

#include <algorithm>

#include "engine/gfx/palette.h"
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

    while (true) {
      const int cur_time = std::min<int>(
          static_cast<int>(engine::platform::SystemTicks() - start_time), fade);

      const engine::gfx::Palette fade_palette =
          engine::gfx::BlendedPalette(CurrentPalette, palette, cur_time, fade);

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
