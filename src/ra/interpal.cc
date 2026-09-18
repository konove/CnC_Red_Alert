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
#include <cstddef>
#include <span>
#include <vector>

#include "absl/log/check.h"
#include "base/array.h"
#include "base/buffer.h"
#include "base/numeric.h"
#include "base/types.h"
#include "ra/externs.h"
#include "sdllib/file_access.h"
#include "sdllib/gbuffer.h"
#include "tech/game_file.h"

bool InterpolationPaletteChanged = false;

unsigned char PaletteInterpolationTable[SIZE_OF_PALETTE][SIZE_OF_PALETTE];
std::span<unsigned char> InterpolationPalette;

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

void Read_Interpolation_Palette(const char* palette_file_name) {
  GameFile palette_file(palette_file_name);

  if (palette_file.IsAvailable()) {
    palette_file.Open(FileAccess::kRead);
    palette_file.ReadObject(PaletteInterpolationTable);
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

void Write_Interpolation_Palette(const char* palette_file_name) {
  GameFile palette_file(palette_file_name);

  if (!palette_file.IsAvailable()) {
    palette_file.Open(FileAccess::kWrite);
    palette_file.WriteObject(PaletteInterpolationTable);
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
static void Create_Palette_Interpolation_Table() {
  //	Asm_Create_Palette_Interpolation_Table();


  //
  // Create an interpolation table for the current palette.
  //
  CHECK_GE(InterpolationPalette.size(),
           static_cast<std::size_t>(SIZE_OF_PALETTE) * 3);
  std::size_t first_palette_ptr = 0;
  for (auto& i : PaletteInterpolationTable) {
    //
    // Get the first palette entry's RGB.
    //
    const int first_r = base::At(InterpolationPalette, first_palette_ptr);
    first_palette_ptr++;
    const int first_g = base::At(InterpolationPalette, first_palette_ptr);
    first_palette_ptr++;
    const int first_b = base::At(InterpolationPalette, first_palette_ptr);
    first_palette_ptr++;

    std::size_t second_palette_ptr = 0;
    for (unsigned char& j : i) {
      //
      // Get the second palette entry's RGB.
      //
      const int second_r = base::At(InterpolationPalette, second_palette_ptr);
      second_palette_ptr++;
      const int second_g = base::At(InterpolationPalette, second_palette_ptr);
      second_palette_ptr++;
      const int second_b = base::At(InterpolationPalette, second_palette_ptr);
      second_palette_ptr++;

      //
      // Now calculate the RGB halfway between the first and second colors.
      //
      const int dest_r = (first_r + second_r) / 2;
      const int dest_g = (first_g + second_g) / 2;
      const int dest_b = (first_b + second_b) / 2;

      //
      // Now find the color in the palette that most closely matches the
      // interpolated color.
      //
      int index_of_closest_color = 0;
      //			closest_distance = (256 * 256) * 3;
      int closest_distance = 500000;
      std::size_t match_pal_ptr = 0;
      for (int p = 0; p < SIZE_OF_PALETTE; p++) {
        const int diff_r =
            static_cast<int>(base::At(InterpolationPalette, match_pal_ptr)) -
            dest_r;
        match_pal_ptr++;
        const int diff_g =
            static_cast<int>(base::At(InterpolationPalette, match_pal_ptr)) -
            dest_g;
        match_pal_ptr++;
        const int diff_b =
            static_cast<int>(base::At(InterpolationPalette, match_pal_ptr)) -
            dest_b;
        match_pal_ptr++;

        const int distance =
            (diff_r * diff_r) + (diff_g * diff_g) + (diff_b * diff_b);
        if (distance < closest_distance) {
          closest_distance = distance;
          index_of_closest_color = p;
        }
      }

      j = static_cast<unsigned char>(index_of_closest_color);
    }
  }
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

void Increase_Palette_Luminance(std::span<unsigned char> palette,
                                int red_percentage, int green_percentage,
                                int blue_percentage, int cap) {
  CHECK_GE(palette.size(), static_cast<std::size_t>(SIZE_OF_PALETTE) * 3);
  for (std::size_t i = 0; i < static_cast<std::size_t>(SIZE_OF_PALETTE) * 3;
       i += 3) {
    int red = base::At(palette, i);
    int green = base::At(palette, i + 1);
    int blue = base::At(palette, i + 2);

    red += red * red_percentage / 100;
    green += green * green_percentage / 100;
    blue += blue * blue_percentage / 100;

    red = std::min(cap, red);
    green = std::min(cap, green);
    blue = std::min(cap, blue);

    base::At(palette, i) = static_cast<unsigned char>(red);
    base::At(palette, i + 1) = static_cast<unsigned char>(green);
    base::At(palette, i + 2) = static_cast<unsigned char>(blue);
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
  // Keep palette interpolation table updated for other code that may use it
  if (InterpolationPaletteChanged) {
    Create_Palette_Interpolation_Table();
  }

  // Render using SDL scaling - palette already set via Update_Palette
  source->Lock();
  WindowBuffer->Render_Scaled_Frame(source->Get_Bytes(), source->Get_Width(),
                                    source->Get_Height());
  source->Unlock();
}

void Rebuild_Interpolated_Palette(const std::span<unsigned char> interpal) {
  if (interpal.size() < 65536) {
    return;
  }
  for (int y = 0; y < 255; y++) {
    for (int x = y + 1; x < 256; x++) {
      base::At(interpal, base::ToSize((y * 256) + x)) =
          base::At(interpal, base::ToSize((x * 256) + y));
    }
  }
}

int Load_Interpolated_Palettes(const char* filename, const bool add) {
  int num_palettes = 0;
  int start_palette = 0;

  PalettesRead = false;
  GameFile file(filename);

  if (!add) {
    // Clearing an inner vector does not invalidate the outer array's iterator.
    // LLVM 23 incorrectly propagates the element invalidation to the array.
    // NOLINTNEXTLINE(clang-diagnostic-lifetime-safety-invalidation)
    for (auto& InterpolatedPalette : InterpolatedPalettes) {
      InterpolatedPalette.clear();
    }
    start_palette = 0;
  } else {
    for (start_palette = 0; start_palette < std::ssize(InterpolatedPalettes);
         start_palette++) {
      if (base::At(InterpolatedPalettes, start_palette).empty()) {
        break;
      }
    }
  }

  // Hack another interpolated palette if the requested one is
  // not present.
  if (!file.IsAvailable()) {
    file.SetName("AAGUN.VQP");
  }

  if (file.IsAvailable()) {
    file.Open(FileAccess::kRead);
    file.ReadObject(num_palettes);

    if (num_palettes < 0 ||
        num_palettes > std::ssize(InterpolatedPalettes) - start_palette) {
      file.Close();
      return 0;
    }
    // Only the lower triangle is stored, row y holding y + 1 entries, so a
    // palette occupies 1 + 2 + ... + 256 bytes on disk. Read all of it at once
    // and lay the rows out here: a read per row was 256 trips down the stream
    // chain, and for a movie packed inside a mixfile each of those was a seek
    // and a read on the archive underneath.
    constexpr int kStoredBytes = 256 * 257 / 2;
    std::vector<unsigned char> stored(kStoredBytes);
    for (int i = 0; i < num_palettes; i++) {
      const int index = i + start_palette;
      // 256 x 256: the blended result for every pair of palette indices.
      base::At(InterpolatedPalettes, index).assign(65536, 0);
      if (file.Read(std::span(stored)) != kStoredBytes) {
        break;
      }
      base::ssize read_offset = 0;
      for (int y = 0; y < 256; y++) {
        const base::ssize row_size = y + 1;
        base::CopyBytes(
            std::as_writable_bytes(
                std::span(base::At(InterpolatedPalettes, index))
                    .subspan(base::ToSize(y) * 256, base::ToSize(row_size))),
            std::as_bytes(std::span(stored).subspan(base::ToSize(read_offset))),
            row_size);
        read_offset += row_size;
      }

      Rebuild_Interpolated_Palette(base::At(InterpolatedPalettes, index));
    }

    PalettesRead = true;
    file.Close();
  }
  PaletteCounter = 0;
  return num_palettes;
}

void Free_Interpolated_Palettes() {
  // Clearing an inner vector does not invalidate the outer array's iterator.
  // LLVM 23 incorrectly propagates the element invalidation to the array.
  // NOLINTNEXTLINE(clang-diagnostic-lifetime-safety-invalidation)
  for (auto& InterpolatedPalette : InterpolatedPalettes) {
    InterpolatedPalette.clear();
  }
}
