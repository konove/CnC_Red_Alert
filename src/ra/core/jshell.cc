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

#include "ra/core/jshell.h"

#include <algorithm>
#include <cstdint>
#include <numeric>
#include <ranges>

#include "base/array.h"
#include "engine/gfx/fading_table.h"
#include "engine/gfx/palette.h"
#include "engine/gfx/palette_color.h"
#include "ra/assets/palette.h"

namespace ra {

FadingTable BuildClosestColorFadingTable(const PaletteClass& palette,
                                         const int target_color,
                                         const int ratio) {
  const engine::gfx::PaletteColor& target = palette.at(target_color);
  FadingTable table{};
  for (int index = 0; index < engine::gfx::Palette::kColorCount; index++) {
    const engine::gfx::PaletteColor faded =
        palette.at(index).FadedToward(ratio, target);
    // Closest_Color() always finds an entry: its starting distance is larger
    // than any two colors can be apart.
    base::At(table, index) = static_cast<uint8_t>(palette.Closest_Color(faded));
  }
  return table;
}

FadingTable BuildShadowFadingTable(const engine::gfx::Palette& palette,
                                   const int target_color, const int ratio) {
  constexpr int kShadowStart = engine::gfx::Palette::kColorCount - 16;
  // Color 255 is not a candidate.
  constexpr auto kCandidates =
      std::views::iota(kShadowStart, engine::gfx::Palette::kColorCount - 1);

  const engine::gfx::PaletteColor& target = palette.at(target_color);
  // Transparent black and the shadow colors above the first one keep their
  // identity entry; that is what stops a shadow over a shadow from darkening.
  FadingTable table{};
  std::ranges::iota(table, uint8_t{0});
  for (int index = 1; index <= kShadowStart; index++) {
    const engine::gfx::PaletteColor faded =
        palette.at(index).FadedToward(ratio, target);
    // min_element() returns the first of equals, so ties go to the lower
    // color.
    const int closest =
        *std::ranges::min_element(kCandidates, {}, [&](const int candidate) {
          return palette.at(candidate).SquaredDistance(faded);
        });
    base::At(table, index) = static_cast<uint8_t>(closest);
  }
  return table;
}

}  // namespace ra
