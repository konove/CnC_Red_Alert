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

/* $Header: /CounterStrike/JSHELL.H 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : JSHELL.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 03/13/95 *
 *                                                                                             *
 *                  Last Update : March 13, 1995 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

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

struct TLucentType {
  unsigned char SourceColor;
  unsigned char DestColor;
  unsigned char Fading;
  unsigned char reserved;
};

void Set_Window(int window, int x, int y, int w, int h);

// Returns the table that fades every palette color toward `color` by `frac`
// (0..255), each entry the palette's closest color to the faded one.
FadingTable MakeFadingTable(const PaletteClass& palette, int color, int frac);

// Returns a fading table toward `color` by `frac` that stays inside the shadow
// range, the last 16 palette colors: colors 1..240 fade to the closest of
// colors 240..254, and color 0 and colors 241..255 keep their own color. A
// shadow drawn over a shadow therefore does not darken further, however many
// shapes overlap.
FadingTable ConquerBuildFadingTable(const PaletteClass& palette, int color,
                                    int frac);

// Returns the translucency table in which each `control` entry's SourceColor
// is drawn through `fade(entry)` and every other color is opaque.
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
