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
 * Functions: * Read_Interpolation_Palette -- reads an interpolation palette
 *table from disk               * Write_Interpolation_Palette -- writes an
 *interpolation palette to disk                     *
 *  Create_Palette_Interpolation_Table -- build the palette interpolation table
 ** Increase_Palette_Luminance -- increase the contrast of a palette *
 *  Interpolate_2X_Scale -- Stretch a 320x200 graphic buffer into 640x400 *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "td/interpal.h"

#include <algorithm>
#include <cstdint>

#include "sdllib/file_access.h"
#include "sdllib/gbuffer.h"
#include "td/ccfile.h"

bool InterpolationPaletteChanged = false;

unsigned char PaletteInterpolationTable[SIZE_OF_PALETTE][SIZE_OF_PALETTE];
unsigned char* InterpolationPalette;

/***********************************************************************************************
 * Read_Interpolatioin_Palette -- reads an interpolation palette table from disk
 **
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    name of palette file *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 12/12/95 12:15PM ST : Created *
 *=============================================================================================*/

void Read_Interpolation_Palette(const char* palette_file_name) {
  CCFileClass palette_file(palette_file_name);

  if (palette_file.Is_Available()) {
    palette_file.Open(FileAccess::kRead);
    palette_file.Read(&PaletteInterpolationTable[0][0],
                      static_cast<int32_t>(256) * 256);
    palette_file.Close();
    InterpolationPaletteChanged = false;
  }
}

/***********************************************************************************************
 * Write_Interpolatioin_Palette -- writes an interpolation palette table to disk
 **
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    name of palette file *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 12/12/95 12:15PM ST : Created *
 *=============================================================================================*/

void Write_Interpolation_Palette(const char* palette_file_name) {
  CCFileClass palette_file(palette_file_name);

  if (!palette_file.Is_Available()) {
    palette_file.Open(FileAccess::kWrite);
    palette_file.Write(&PaletteInterpolationTable[0][0],
                       static_cast<int32_t>(256) * 256);
    palette_file.Close();
  }
}

/***********************************************************************************************
 * Increase_Palette_Luminance -- increase contrast of colours in a palette *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    ptr to palette * percentage increase of red * percentage increase
 *of green                                                      * percentage
 *increase of blue                                                       * cap
 *value for colours *
 *                                                                                             *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 12/12/95 12:16PM ST : Created *
 *=============================================================================================*/

void Increase_Palette_Luminance(unsigned char* palette, int red_percentage,
                                int green_percentage, int blue_percentage,
                                int cap) {
  int red;
  int green;
  int blue;
  for (int i = 0; i < SIZE_OF_PALETTE * 3; i += 3) {
    red = *(palette + i);
    green = *(palette + i + 1);
    blue = *(palette + i + 2);

    red += red * red_percentage / 100;
    green += green * green_percentage / 100;
    blue += blue * blue_percentage / 100;

    red = std::min(cap, red);
    green = std::min(cap, green);
    blue = std::min(cap, blue);

    *(palette + i) = static_cast<unsigned char>(red);
    *(palette + i + 1) = static_cast<unsigned char>(green);
    *(palette + i + 2) = static_cast<unsigned char>(blue);
  }
}

/***************************************************************************
 * INTERPOLATE_2X_SCALE                                                    *
 *                                                                         *
 * Renders a 320x200 paletted frame to the screen using SDL texture        *
 * scaling with bilinear filtering.                                        *
 *                                                                         *
 * INPUT:    source - GraphicBufferClass containing 320x200 paletted data  *
 *           dest - unused (kept for API compatibility)                    *
 *           palette_file_name - unused (kept for API compatibility)       *
 *                                                                         *
 * OUTPUT:   Nothing                                                       *
 *                                                                         *
 * HISTORY:                                                                *
 *   12/06/1995  MG : Created.                                             *
 *   01/2026     : Replaced with SDL texture scaling.                      *
 *=========================================================================*/
void Interpolate_2X_Scale(GraphicBufferClass* source,
                          GraphicViewPortClass* /*unused*/,
                          const char* /*unused*/) {
  // Render using SDL scaling - palette already set via Update_Palette
  source->Lock();
  WindowBuffer->Render_Scaled_Frame(source->Get_Offset(), source->Get_Width(),
                                    source->Get_Height());
  source->Unlock();
}
