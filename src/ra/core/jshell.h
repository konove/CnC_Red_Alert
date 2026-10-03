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

// File: Red Alert's text-window setter and the builders of the fading and
// translucency tables its theaters draw through.

#ifndef CNC_RED_ALERT_RA_CORE_JSHELL_H_
#define CNC_RED_ALERT_RA_CORE_JSHELL_H_

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>

#include "base/array.h"
#include "engine/gfx/fading_table.h"
#include "ra/assets/palette.h"

namespace ra {

// One translucent color of a translucency table: shape pixels of SourceColor
// are not drawn as that color but fade what is under them toward DestColor.
struct TLucentType {
  unsigned char SourceColor;  // Palette index that draws translucent.
  unsigned char DestColor;    // Palette index the background fades toward.
  unsigned char Fading;       // How far it fades: 0 not at all, 255 fully.
  unsigned char reserved;     // Unused; 0 in every table.
};

// Sets row `window` of WindowList (a WindowNumberType) to the rectangle at
// (`x`, `y`), `w` by `h`. All four are in pixels: unlike Tiberian Dawn, Red
// Alert does not keep the X position and width in 8-pixel bytes.
void Set_Window(int window, int x, int y, int w, int h);

// Returns the table that fades every palette color toward `color` by `frac`
// (0..255; values outside are clamped), each entry the palette's closest color
// to the faded one. Unlike the engine's BuildFadingTable(), color 0 is faded
// too, a color may map to itself, and closeness is the sum of the component
// differences, not of their squares.
FadingTable MakeFadingTable(const PaletteClass& palette, int color, int frac);

// Returns a fading table toward `color` by `frac` (0..255; values outside are
// clamped) that stays inside the shadow range, the last 16 palette colors:
// colors 1..240 fade to the closest of colors 240..254, and color 0 and colors
// 241..255 keep their own color. A shadow drawn over a shadow therefore does
// not darken further, however many shapes overlap.
FadingTable ConquerBuildFadingTable(const PaletteClass& palette, int color,
                                    int frac);

// Returns the translucency table in which each `control` entry's SourceColor
// is drawn through `fade(entry)`, a FadingTable, and every other color is
// opaque. When two entries share a SourceColor the later one wins. `N` is at
// most 255, the lookup value that marks an opaque color.
template <std::size_t N, std::invocable<const TLucentType&> MakeFade>
TranslucencyTable<N> MakeTranslucencyTable(
    const std::array<TLucentType, N>& control, MakeFade fade) {
  TranslucencyTable<N> table{};
  const std::span<uint8_t> lookup = std::span(table).first(256);
  std::ranges::fill(lookup, uint8_t{255});
  for (std::size_t index = 0; index < N; ++index) {
    const TLucentType& entry = base::At(control, index);
    base::At(lookup, entry.SourceColor) = static_cast<uint8_t>(index);
    const FadingTable faded = fade(entry);
    std::ranges::copy(faded,
                      std::span(table).subspan((index + 1) * 256).begin());
  }
  return table;
}

// Returns the translucency table that draws each `control` entry's
// SourceColor faded toward its DestColor by its Fading, matched against the
// whole of `palette`.
template <std::size_t N>
TranslucencyTable<N> BuildTranslucentTable(
    const PaletteClass& palette, const std::array<TLucentType, N>& control) {
  return MakeTranslucencyTable(control, [&palette](const TLucentType& entry) {
    return BuildFadingTable(palette, entry.DestColor, entry.Fading);
  });
}

// Like BuildTranslucentTable(), but the fades stay inside the shadow range
// (see ConquerBuildFadingTable()), so overlapping translucent shapes do not
// compound.
template <std::size_t N>
TranslucencyTable<N> ConquerBuildTranslucentTable(
    const PaletteClass& palette, const std::array<TLucentType, N>& control) {
  return MakeTranslucencyTable(control, [&palette](const TLucentType& entry) {
    return ConquerBuildFadingTable(palette, entry.DestColor, entry.Fading);
  });
}

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_CORE_JSHELL_H_
