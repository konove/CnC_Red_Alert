#include "ra/assets/palette.h"

#include <cstdlib>
#include <span>

#include "base/array.h"
#include "engine/gfx/palette.h"
#include "engine/gfx/palette_color.h"
#include "engine/window/display.h"
#include "engine/window/hardware_cursor.h"
#include "engine/window/palette_fade.h"
#include "ra/ui/input.h"

namespace ra {

PaletteClass PaletteClass::CurrentPalette;

// Shows `palette` on the display and recolors the mouse cursor, which SDL
// draws in the colors it had when the cursor was made.
static void ShowPalette(const engine::gfx::Palette& palette) {
  engine::window::TheDisplay().SetPalette(palette);
  TheMouse().UpdatePalette();
}

void PaletteClass::Set(int fade, void (*callback)()) {
  if (!fades_disabled_) {
    engine::window::FadePalette(CurrentPalette, *this, fade, ShowPalette,
                                callback);
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

}  // namespace ra
