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

// Saving and loading a picture as a .PCX file: 8 bits a pixel, run-length
// encoded, with a 256-color palette after the pixels. The games write their
// screenshots this way, and their title screens and the pictures behind the
// score and the credits are stored so. Originally part of the Westwood IFF
// library (FILEPCX.H and WRITEPCX.CPP, Julio R. Jerez, May 1995).

#ifndef CNC_RED_ALERT_ENGINE_GFX_PCX_FILE_H_
#define CNC_RED_ALERT_ENGINE_GFX_PCX_FILE_H_

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

#include "engine/gfx/palette.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/stream/byte_stream.h"

namespace engine::gfx {

// A picture read from a PCX file.
struct PcxPicture {
  int width = 0;
  int height = 0;
  // The palette index of every pixel, row after row with nothing between
  // them: width * height bytes.
  std::vector<uint8_t> pixels;
  Palette palette;
};

// Writes `view` as a PCX picture with `palette` to `out` at its current
// position, and leaves `out` open. Returns whether all of it was written;
// nothing is for a view without pixels (unattached, or of a surface that will
// not lock) or one larger than a PCX header can say: wider than 32766 pixels
// or taller than 32768. A row of odd width is written one byte longer, as the
// format wants an even number of bytes a scanline; ReadPcx() drops that byte.
bool WritePcx(ByteStream& out, PixelView& view, const Palette& palette);

// Writes the same picture to the game file `file_name`, which OpenGameFile()
// creates as a loose file. A view that WritePcx() writes nothing for creates
// no file.
[[nodiscard]] bool WritePcxFile(std::string_view file_name, PixelView& view,
                                const Palette& palette);

// Reads the PCX picture that `in` holds from its current position to its end.
// Returns nothing if that is not a run-length encoded 8-bit picture ending in
// a palette, or if its scanlines end before the picture does.
std::optional<PcxPicture> ReadPcx(ByteStream& in);

// Reads the PCX picture in the game file `file_name`. Returns nothing if the
// file is missing or ReadPcx() makes nothing of it.
std::optional<PcxPicture> ReadPcxFile(std::string_view file_name);

}  // namespace engine::gfx

#endif  // CNC_RED_ALERT_ENGINE_GFX_PCX_FILE_H_
