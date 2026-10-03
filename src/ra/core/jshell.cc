/*
**	Command & Conquer Red Alert(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// File: The builders of the fading and translucency tables Red Alert's
// theaters draw through.

#include "ra/core/jshell.h"

#include <cstdint>

#include "base/array.h"
#include "engine/gfx/fading_table.h"
#include "engine/gfx/palette_color.h"
#include "ra/assets/palette.h"

namespace ra {

FadingTable BuildClosestColorFadingTable(const PaletteClass& palette,
                                         int target_color, int ratio) {
  FadingTable table{};
  for (int index = 0; index < PaletteClass::COLOR_COUNT; index++) {
    const engine::gfx::PaletteColor faded =
        palette.at(index).FadedToward(ratio, palette.at(target_color));
    // Closest_Color() always finds an entry: its starting distance is larger
    // than any two colors can be apart.
    base::At(table, index) = static_cast<uint8_t>(palette.Closest_Color(faded));
  }
  return table;
}

FadingTable BuildShadowFadingTable(const PaletteClass& palette,
                                   int target_color, int ratio) {
  constexpr int kShadowStart = PaletteClass::COLOR_COUNT - 16;
  FadingTable table{};
  for (int index = 0; index < PaletteClass::COLOR_COUNT; index++) {
    // Transparent black and the shadow colors above the first one are drawn
    // as they are; that is what stops a shadow over a shadow from darkening.
    if (index == 0 || index > kShadowStart) {
      base::At(table, index) = static_cast<uint8_t>(index);
      continue;
    }
    const engine::gfx::PaletteColor faded =
        palette.at(index).FadedToward(ratio, palette.at(target_color));
    // Color 255 is not a candidate; ties go to the lower color.
    int best = -1;
    int best_distance = 0;
    for (int candidate = kShadowStart;
         candidate < PaletteClass::COLOR_COUNT - 1; candidate++) {
      const int distance = palette.at(candidate).SquaredDistance(faded);
      if (best == -1 || distance < best_distance) {
        best = candidate;
        best_distance = distance;
      }
    }
    base::At(table, index) = static_cast<uint8_t>(best);
  }
  return table;
}

}  // namespace ra
