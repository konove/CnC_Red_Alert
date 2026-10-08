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

// Saving a picture as a .PCX file: 8 bits a pixel, run-length encoded, with a
// 256-color palette after the pixels. The games write their screenshots this
// way. Originally part of the Westwood IFF library (FILEPCX.H and WRITEPCX.CPP,
// Julio R. Jerez, May 1995).

#ifndef CNC_RED_ALERT_ENGINE_GFX_PCX_FILE_H_
#define CNC_RED_ALERT_ENGINE_GFX_PCX_FILE_H_

#include <cstdint>

#include "engine/gfx/palette.h"
#include "engine/gfx/pixel_buffer.h"

// One color of the header's 16-color EGA palette.
struct RGB {
  char red;
  char green;
  char blue;
};

// The 128 bytes a PCX file starts with, laid out as they are on disk: the
// 16-bit fields are little-endian and all fall on even offsets, so the struct
// has no padding and is read and written whole. Tiberian Dawn's PCX reader
// (td/startup/nondosstub.cc) uses it too.
struct PCX_HEADER {
  char id;         // 10 marks a PCX file.
  char version;    // 5 is the first that can end in a 256-color palette.
  char encoding;   // 1 means run-length encoded, the only encoding there is.
  char pixelsize;  // Bits a pixel in each color plane.
  // The picture's left and top edge.
  int16_t x;
  int16_t y;
  // The picture's right and bottom edge, inclusive: for a picture at 0,0 they
  // are its width and height minus one.
  int16_t width;
  int16_t height;
  // The resolution of the device the picture was made for. Readers ignore it.
  int16_t xres;
  int16_t yres;
  RGB ega_palette[16];  // Unused by an 8-bit picture.
  char nothing;         // Reserved, zero.
  char color_planes;
  // Bytes in one decoded scanline of one plane, which may be more than the
  // picture is wide.
  int16_t byte_per_line;
  int16_t palette_type;  // 1 is color, 2 grayscale.
  char filler[58];       // Pads the header to 128 bytes.
};

// Writes `pic` as a PCX picture with `palette` to the game file `name`, which
// OpenGameFile() creates as a loose file. Returns 0 whether or not that
// worked: a file that cannot be created is skipped silently, and a failed
// write is not noticed.
int Write_PCX_File(const char* name, PixelView& pic,
                   const engine::gfx::Palette& palette);

#endif  // CNC_RED_ALERT_ENGINE_GFX_PCX_FILE_H_
