#include "ra/palette.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>

#include "absl/strings/str_format.h"
#include "engine/gfx/palette.h"
#include "engine/gfx/palette_color.h"
#include "engine/platform/timer.h"
#include "engine/window/display.h"
#include "engine/window/hardware_cursor.h"
#include "ra/input.h"

PaletteClass PaletteClass::CurrentPalette;

// Shows `palette` on the display and recolors the mouse cursor, which SDL
// draws in the colors it had when the cursor was made.
static void ShowPalette(const engine::gfx::Palette& palette) {
  engine::window::TheDisplay().SetPalette(palette);
  TheMouse().UpdatePalette();
}

void PaletteClass::Set(int fade, void (*callback)()) {
  // A fade to the palette already on screen would spend its whole duration
  // redrawing an unchanged screen, e.g. the fade to black before a movie
  // that starts from black.
  if (fades_disabled_ || *this == CurrentPalette) {
    fade = 0;
  }

  if (fade) {
    const auto start_time = engine::platform::SystemTicks();

    engine::gfx::Palette fade_palette;

    while (true) {
      const int64_t now = engine::platform::SystemTicks();
      const int cur_time =
          static_cast<int>(std::min<int64_t>(now - start_time, fade));

      // Each gun steps linearly from the current palette's to this one's.
      const auto blend = [cur_time, fade](const int old_gun,
                                          const int new_gun) {
        const auto six_bits = [](const int gun) {
          return static_cast<int>(static_cast<unsigned>(gun) & 0x3FU);
        };
        const int old_val = six_bits(old_gun);
        const int new_val = six_bits(new_gun);
        return old_val + ((new_val - old_val) * cur_time / fade);
      };
      for (int i = 0; i < COLOR_COUNT; i++) {
        const engine::gfx::PaletteColor& from = CurrentPalette.at(i);
        const engine::gfx::PaletteColor& to = at(i);
        fade_palette.at(i) = engine::gfx::PaletteColor::FromGuns(
            blend(from.red_gun(), to.red_gun()),
            blend(from.green_gun(), to.green_gun()),
            blend(from.blue_gun(), to.blue_gun()));
      }

      ShowPalette(fade_palette);
      if (callback) {
        callback();
      } else {
        // make sure we actually display the fade
        engine::window::TheDisplay().EndFrame();
      }

      if (cur_time == fade) {
        break;
      }

      // The blend only changes when the tick does, so presenting again
      // before then redraws an identical frame. Unpaced, this loop presented
      // over a thousand times a second and kept a core busy for the fade.
      while (engine::platform::SystemTicks() == now) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
    }
  }

  CurrentPalette = *this;
  ShowPalette(*this);
}

// the only code that uses these two (Play_Movie and OptionsClass::Proccess)
// only use it to adjust the black palette then immediately adjust it back
// presumably this is to force a palette update
void PaletteClass::Adjust(int /*unused*/) {}

void PaletteClass::Adjust(int /*unused*/, PaletteClass& /*unused*/) {}

void PaletteClass::Partial_Adjust(int /*unused*/, char* /*unused*/) {
  absl::PrintF("PaletteClass::%s\n", __func__);
}

void PaletteClass::Partial_Adjust(int /*unused*/, PaletteClass& /*unused*/,
                                  char* /*unused*/) {
  absl::PrintF("PaletteClass::%s\n", __func__);
}

int PaletteClass::Closest_Color(const engine::gfx::PaletteColor& col) const {
  int index = -1;
  int diff = 256 * 3;

  for (int i = 0; i < COLOR_COUNT; i++) {
    const engine::gfx::PaletteColor& entry = at(i);
    const int new_diff = std::abs(col.red() - entry.red()) +
                         std::abs(col.green() - entry.green()) +
                         std::abs(col.blue() - entry.blue());

    if (new_diff == 0) {
      return i;
    }

    if (new_diff < diff) {
      index = i;
      diff = new_diff;
    }
  }

  return index;
}

void Set_Palette(const engine::gfx::Palette& palette) {
  PaletteClass::CurrentPalette = palette;
  ShowPalette(palette);
}
