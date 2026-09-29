/*
**	Command & Conquer(tm)
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

/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : INTERPAL.CPP *
 *                                                                                             *
 *                   Programmer : Steve Tall *
 *                                                                                             *
 *                   Start Date : December 7th 1995 *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Overview: * This module contains functions to allow use of old 320x200
 *animations on a 640x400 screen  *
 *                                                                                             *
 * Functions:
 *  Increase_Palette_Luminance -- increase the contrast of a palette *
 *  Interpolate_2X_Scale -- Stretch a 320x200 graphic buffer into 640x400 *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "td/interpal.h"

#include <algorithm>
#include <cstdint>
#include <span>

#include "engine/gfx/palette.h"
#include "engine/gfx/palette_color.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/window/display.h"

/***********************************************************************************************
 * Increase_Palette_Luminance -- increase contrast of colours in a palette *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    palette, percentage increase of red, green and blue *
 *                                                                                             *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 12/12/95 12:16PM ST : Created *
 *=============================================================================================*/

void Increase_Palette_Luminance(engine::gfx::Palette& palette,
                                int red_percentage, int green_percentage,
                                int blue_percentage) {
  const auto brighten = [](const int component, const int percentage) {
    return static_cast<uint8_t>(
        std::min<int>(component + (component * percentage / 100),
                      engine::gfx::PaletteColor::kMaxComponent));
  };
  for (engine::gfx::PaletteColor& color : palette.colors()) {
    color = engine::gfx::PaletteColor(brighten(color.red(), red_percentage),
                                      brighten(color.green(), green_percentage),
                                      brighten(color.blue(), blue_percentage));
  }
}

/***************************************************************************
 * INTERPOLATE_2X_SCALE                                                    *
 *                                                                         *
 * Renders a 320x200 paletted frame to the screen using SDL texture        *
 * scaling with bilinear filtering.                                        *
 *                                                                         *
 * INPUT:    source - PixelBuffer containing 320x200 paletted data  *
 *           dest - unused (kept for API compatibility)                    *
 *           palette_file_name - unused (kept for API compatibility)       *
 *                                                                         *
 * OUTPUT:   Nothing                                                       *
 *                                                                         *
 * HISTORY:                                                                *
 *   12/06/1995  MG : Created.                                             *
 *   01/2026     : Replaced with SDL texture scaling.                      *
 *=========================================================================*/
void Interpolate_2X_Scale(PixelBuffer* source, PixelView* /*unused*/,
                          const char* /*unused*/) {
  // SDL stretches the frame to the window, in the palette already set.
  source->LockSurface();
  engine::window::TheDisplay().PresentScaledFrame(
      source->bytes(), source->width(), source->height());
  source->UnlockSurface();
}
