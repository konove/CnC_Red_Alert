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

#ifndef CNC_RED_ALERT_RA_CORE_FADING_TABLE_BUILDERS_H_
#define CNC_RED_ALERT_RA_CORE_FADING_TABLE_BUILDERS_H_

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>

#include "base/array.h"
#include "engine/gfx/fading_table.h"
#include "engine/gfx/palette.h"
#include "ra/assets/palette.h"

namespace ra {

// One translucent color of a translucency table: shape pixels of source_color
// are not drawn as that color but fade what is under them toward target_color.
struct TranslucentColor {
  uint8_t source_color;  // Palette index that draws translucent.
  uint8_t target_color;  // Palette index the background fades toward.
  uint8_t fade_ratio;    // How far it fades: 0 not at all, 255 fully.
};

// Returns the table that fades every palette color toward `target_color` by
// `ratio` (0..255; values outside are clamped), each entry the palette's
// closest color to the faded one. Unlike the engine's BuildFadingTable(), color
// 0 is faded too, a color may map to itself, and closeness is the sum of the
// component differences, not of their squares.
FadingTable BuildClosestColorFadingTable(const PaletteClass& palette,
                                         int target_color, int ratio);

// Returns a fading table toward `target_color` by `ratio` (0..255; values
// outside are clamped) that stays inside the shadow range, the last 16 palette
// colors: colors 1..240 fade to the closest of colors 240..254, and color 0 and
// colors 241..255 keep their own color. A shadow drawn over a shadow therefore
// does not darken further, however many shapes overlap.
FadingTable BuildShadowFadingTable(const engine::gfx::Palette& palette,
                                   int target_color, int ratio);

// Returns the translucency table in which each `colors` entry's source_color
// is drawn through `make_fade(entry)`, a FadingTable, and every other color is
// opaque. When two entries share a source_color the later one wins. `N` is at
// most 255, the lookup value that marks an opaque color.
template <std::size_t N, std::invocable<const TranslucentColor&> MakeFade>
TranslucencyTable<N> BuildTranslucencyTableWith(
    const std::array<TranslucentColor, N>& colors, MakeFade make_fade) {
  TranslucencyTable<N> table{};
  const std::span<uint8_t> lookup = std::span(table).first(256);
  std::ranges::fill(lookup, uint8_t{255});
  for (std::size_t index = 0; index < N; ++index) {
    const TranslucentColor& entry = base::At(colors, index);
    base::At(lookup, entry.source_color) = static_cast<uint8_t>(index);
    const FadingTable fade_table = make_fade(entry);
    std::ranges::copy(fade_table,
                      std::span(table).subspan((index + 1) * 256).begin());
  }
  return table;
}

// Returns the translucency table that draws each `colors` entry's
// source_color faded toward its target_color by its fade_ratio, matched against
// the whole of `palette`.
template <std::size_t N>
TranslucencyTable<N> BuildTranslucencyTable(
    const engine::gfx::Palette& palette,
    const std::array<TranslucentColor, N>& colors) {
  return BuildTranslucencyTableWith(
      colors, [&palette](const TranslucentColor& entry) {
        return BuildFadingTable(palette, entry.target_color, entry.fade_ratio);
      });
}

// Like BuildTranslucencyTable(), but the fades stay inside the shadow range
// (see BuildShadowFadingTable()), so overlapping translucent shapes do not
// compound.
template <std::size_t N>
TranslucencyTable<N> BuildShadowTranslucencyTable(
    const engine::gfx::Palette& palette,
    const std::array<TranslucentColor, N>& colors) {
  return BuildTranslucencyTableWith(
      colors, [&palette](const TranslucentColor& entry) {
        return BuildShadowFadingTable(palette, entry.target_color,
                                      entry.fade_ratio);
      });
}

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_CORE_FADING_TABLE_BUILDERS_H_
