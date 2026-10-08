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

// Loading a .PCX picture, which is how the title screens and the pictures
// behind the score and the credits are stored. Originally part of the Westwood
// IFF library (FILEPCX.H, Julio R. Jerez, May 1995).

#ifndef CNC_RED_ALERT_RA_ASSETS_FILEPCX_H_
#define CNC_RED_ALERT_RA_ASSETS_FILEPCX_H_

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

#include "engine/gfx/palette.h"

namespace ra {

// A picture read from a PCX file.
struct PcxPicture {
  int width = 0;
  int height = 0;
  // The palette index of every pixel, row after row with nothing between
  // them: width * height bytes.
  std::vector<uint8_t> pixels;
  engine::gfx::Palette palette;
};

// Reads the PCX picture in the game file `name`. Returns nothing if the file
// is missing, is not a run-length encoded 8-bit picture with a palette, or is
// damaged.
std::optional<PcxPicture> Read_PCX_File(std::string_view name);

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_ASSETS_FILEPCX_H_
