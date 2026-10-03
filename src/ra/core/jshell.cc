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

/* $Header: /CounterStrike/JSHELL.CPP 2     3/13/97 2:05p Steve_tall $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : JSHELL.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : April 2, 1994 *
 *                                                                                             *
 *                  Last Update : May 11, 1995 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:
 *   Set_Window -- Sets the window dimensions to that specified. *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "ra/core/jshell.h"

#include <cstdint>
#include <cstring>

#include "base/array.h"
#include "engine/gfx/fading_table.h"
#include "engine/gfx/palette_color.h"
#include "engine/gfx/text_window.h"
#include "ra/assets/palette.h"

namespace ra {

/***********************************************************************************************
 * Set_Window -- Sets the window dimensions to that specified. *
 *                                                                                             *
 *    Use this routine to set the windows dimensions to the coordinates and
 *dimensions         * specified. *
 *                                                                                             *
 * INPUT:   x     -- Window X pixel position. *
 *                                                                                             *
 *          y     -- Window Y pixel position. *
 *                                                                                             *
 *          w     -- Window width in pixels. *
 *                                                                                             *
 *          h     -- Window height in pixels. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   The X and width values are truncated to an even 8 pixel boundary.
 *This is       * the same as stripping off the lower 3 bits. *
 *                                                                                             *
 * HISTORY: * 01/15/1995 JLB : Created. *
 *=============================================================================================*/
void Set_Window(int window, int x, int y, int w, int h) {
  base::At(base::At(WindowList, window), kWindowWidth) = w;
  base::At(base::At(WindowList, window), kWindowHeight) = h;
  base::At(base::At(WindowList, window), kWindowX) = x;
  base::At(base::At(WindowList, window), kWindowY) = y;
}

FadingTable MakeFadingTable(const PaletteClass& palette, int color, int frac) {
  FadingTable table{};
  for (int index = 0; index < PaletteClass::COLOR_COUNT; index++) {
    const engine::gfx::PaletteColor faded =
        palette.at(index).FadedToward(frac, palette.at(color));
    base::At(table, index) = static_cast<uint8_t>(palette.Closest_Color(faded));
  }
  return table;
}

FadingTable ConquerBuildFadingTable(const PaletteClass& palette, int color,
                                    int frac) {
  constexpr int kShadowStart = PaletteClass::COLOR_COUNT - 16;
  FadingTable table{};
  for (int index = 0; index < PaletteClass::COLOR_COUNT; index++) {
    if (index == 0 || index > kShadowStart) {
      base::At(table, index) = static_cast<uint8_t>(index);
      continue;
    }
    const engine::gfx::PaletteColor faded =
        palette.at(index).FadedToward(frac, palette.at(color));
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
