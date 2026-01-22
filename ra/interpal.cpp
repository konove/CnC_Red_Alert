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

/* $Header: /CounterStrike/INTERPAL.CPP 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
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

#include "ra/interpal.h"

#include <algorithm>

#include "ra/ccfile.h"
#include "ra/compat.h"
#include "sdllib/include/gbuffer.h"

bool InterpolationPaletteChanged = false;

unsigned char PaletteInterpolationTable[SIZE_OF_PALETTE][SIZE_OF_PALETTE];
unsigned char* InterpolationPalette;

/***********************************************************************************************
 * Read_Interpolation_Palette -- reads an interpolation palette table from disk
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

void Read_Interpolation_Palette(char const* palette_file_name) {
  CCFileClass palette_file(palette_file_name);

  if (palette_file.Is_Available()) {
    palette_file.Open(READ);
    palette_file.Read(&PaletteInterpolationTable[0][0], 256 * 256);
    palette_file.Close();
    InterpolationPaletteChanged = false;
  }
}

/***********************************************************************************************
 * Write_Interpolation_Palette -- writes an interpolation palette table to disk
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

void Write_Interpolation_Palette(char const* palette_file_name) {
  CCFileClass palette_file(palette_file_name);

  if (!palette_file.Is_Available()) {
    palette_file.Open(WRITE);
    palette_file.Write(&PaletteInterpolationTable[0][0], 256 * 256);
    palette_file.Close();
  }
}

/***************************************************************************
 * CREATE_PALETTE_INTERPOLATION_TABLE                                      *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   12/06/1995  MG : Created.                                             *
 *=========================================================================*/
void Create_Palette_Interpolation_Table() {
  //	Asm_Create_Palette_Interpolation_Table();

#if (1)

  int i;
  int j;
  int p;
  unsigned char* first_palette_ptr;
  unsigned char* second_palette_ptr;
  unsigned char* match_pal_ptr;
  int first_r;
  int first_g;
  int first_b;
  int second_r;
  int second_g;
  int second_b;
  int diff_r;
  int diff_g;
  int diff_b;
  int dest_r;
  int dest_g;
  int dest_b;
  int distance;
  int closest_distance;
  int index_of_closest_color;

  //
  // Create an interpolation table for the current palette.
  //
  first_palette_ptr = InterpolationPalette;
  for (i = 0; i < SIZE_OF_PALETTE; i++) {
    //
    // Get the first palette entry's RGB.
    //
    first_r = *first_palette_ptr;
    first_palette_ptr++;
    first_g = *first_palette_ptr;
    first_palette_ptr++;
    first_b = *first_palette_ptr;
    first_palette_ptr++;

    second_palette_ptr = InterpolationPalette;
    for (j = 0; j < SIZE_OF_PALETTE; j++) {
      //
      // Get the second palette entry's RGB.
      //
      second_r = *second_palette_ptr;
      second_palette_ptr++;
      second_g = *second_palette_ptr;
      second_palette_ptr++;
      second_b = *second_palette_ptr;
      second_palette_ptr++;

      //
      // Now calculate the RGB halfway between the first and second colors.
      //
      dest_r = (first_r + second_r) >> 1;
      dest_g = (first_g + second_g) >> 1;
      dest_b = (first_b + second_b) >> 1;

      //
      // Now find the color in the palette that most closely matches the
      // interpolated color.
      //
      index_of_closest_color = 0;
      //			closest_distance = (256 * 256) * 3;
      closest_distance = 500000;
      match_pal_ptr = (unsigned char*)InterpolationPalette;
      for (p = 0; p < SIZE_OF_PALETTE; p++) {
        diff_r = (int)*match_pal_ptr - dest_r;
        match_pal_ptr++;
        diff_g = (int)*match_pal_ptr - dest_g;
        match_pal_ptr++;
        diff_b = (int)*match_pal_ptr - dest_b;
        match_pal_ptr++;

        distance = diff_r * diff_r + diff_g * diff_g + diff_b * diff_b;
        if (distance < closest_distance) {
          closest_distance = distance;
          index_of_closest_color = p;
        }
      }

      PaletteInterpolationTable[i][j] = (unsigned char)index_of_closest_color;
    }
  }

#endif
  InterpolationPaletteChanged = false;
}

/***********************************************************************************************
 * Increase_Palette_Luminance -- increase contrast of colours in a palette *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    ptr to palette
 * percentage increase of red
 * percentage increase of green
 * percentage increase of blue
 * cap value for colours
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
                                unsigned cap) {
  unsigned int red;
  unsigned int green;
  unsigned int blue;
  for (int i = 0; i < SIZE_OF_PALETTE * 3; i += 3) {
    red = (unsigned)*(palette + i);
    green = (unsigned)*(palette + i + 1);
    blue = (unsigned)*(palette + i + 2);

    red += red * red_percentage / 100;
    green += green * green_percentage / 100;
    blue += blue * blue_percentage / 100;

    red = std::min(cap, red);
    green = std::min(cap, green);
    blue = std::min(cap, blue);

    *(palette + i) = (unsigned char)red;
    *(palette + i + 1) = (unsigned char)green;
    *(palette + i + 2) = (unsigned char)blue;
  }
}

#if defined(WIN32)
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
void Interpolate_2X_Scale(GraphicBufferClass* source, GraphicViewPortClass*,
                          char const*) {
  // Keep palette interpolation table updated for other code that may use it
  if (InterpolationPaletteChanged) {
    Create_Palette_Interpolation_Table();
  }

  // Render using SDL scaling - palette already set via Update_Palette
  source->Lock();
  WindowBuffer->Render_Scaled_Frame(source->Get_Offset(), source->Get_Width(),
                                    source->Get_Height());
  source->Unlock();
}

#endif
