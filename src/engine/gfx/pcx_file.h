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

#include <array>
#include <cstdint>
#include <string_view>

#include "engine/gfx/palette.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/stream/byte_stream.h"

namespace engine::gfx {

// The 128 bytes a PCX file starts with, laid out as they are on disk: the
// 16-bit fields are little-endian and all fall on even offsets, so the struct
// has no padding and is read and written whole. Tiberian Dawn's PCX reader
// (td/startup/nondosstub.cc) uses it too.
struct PcxHeader {
  uint8_t id;         // 10 marks a PCX file.
  uint8_t version;    // 5 is the first that can end in a 256-color palette.
  uint8_t encoding;   // 1 means run-length encoded, the only encoding there is.
  uint8_t bits_per_pixel;  // Bits a pixel in each color plane.
  // The picture's left and top edge.
  int16_t left;
  int16_t top;
  // The picture's right and bottom edge, inclusive: for a picture at 0,0 they
  // are its width and height minus one.
  int16_t right;
  int16_t bottom;
  // The resolution of the device the picture was made for. Readers ignore it.
  int16_t horizontal_dpi;
  int16_t vertical_dpi;
  // Sixteen colors of three bytes each, unused by an 8-bit picture.
  std::array<uint8_t, 48> ega_palette;
  uint8_t reserved;                 // Reserved, zero.
  uint8_t color_planes;
  // Bytes in one decoded scanline of one plane, which may be more than the
  // picture is wide.
  int16_t bytes_per_line;
  int16_t palette_type;  // 1 is color, 2 grayscale.
  std::array<uint8_t, 58> padding;  // Pads the header to 128 bytes.
};
static_assert(sizeof(PcxHeader) == 128);

// Writes `view` as a PCX picture with `palette` to `out` at its current
// position, and leaves `out` open. Returns whether all of it was written;
// nothing is for a view without pixels (unattached, or of a surface that will
// not lock).
bool WritePcx(ByteStream& out, PixelView& view, const Palette& palette);

// Writes the same picture to the game file `file_name`, which OpenGameFile()
// creates as a loose file. A view without pixels creates no file.
bool WritePcxFile(std::string_view file_name, PixelView& view,
                  const Palette& palette);

}  // namespace engine::gfx

#endif  // CNC_RED_ALERT_ENGINE_GFX_PCX_FILE_H_
