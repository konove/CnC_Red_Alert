#include "ra/palette.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <span>
#include <thread>

#include "base/array.h"
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

  if (fade > 0) {
    const auto start_time = engine::platform::SystemTicks();

    while (true) {
      const int64_t now = engine::platform::SystemTicks();
      const int cur_time =
          static_cast<int>(std::min<int64_t>(now - start_time, fade));

      const engine::gfx::Palette fade_palette =
          engine::gfx::BlendedPalette(CurrentPalette, *this, cur_time, fade);

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

// Every adjustment is RGBClass::Adjust applied per entry: a blend of
// `ratio` steps out of 256.
void PaletteClass::Adjust(const int ratio) {
  Adjust(ratio, engine::gfx::Palette(engine::gfx::kBlackColor));
}

void PaletteClass::Adjust(const int ratio,
                          const engine::gfx::Palette& palette) {
  static_cast<engine::gfx::Palette&>(*this) =
      engine::gfx::BlendedPalette(*this, palette, ratio, 256);
}

void PaletteClass::Partial_Adjust(
    const int ratio, const std::span<const char, kColorCount> lookup) {
  Partial_Adjust(ratio, engine::gfx::Palette(engine::gfx::kBlackColor), lookup);
}

void PaletteClass::Partial_Adjust(
    const int ratio, const engine::gfx::Palette& palette,
    const std::span<const char, kColorCount> lookup) {
  const engine::gfx::Palette adjusted =
      engine::gfx::BlendedPalette(*this, palette, ratio, 256);
  for (int i = 0; i < kColorCount; ++i) {
    if (base::At(lookup, i) != 0) {
      at(i) = adjusted.at(i);
    }
  }
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
