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

#include <memory>

#include "engine/gfx/palette.h"
#include "engine/gfx/pixel_buffer.h"

namespace ra {

// Reads the PCX picture `name` into a new buffer. Fills `*palette` with the
// file's palette when `palette` is not nullptr. Returns null if the file is
// missing or malformed.
std::unique_ptr<PixelBuffer> Read_PCX_File(const char* name,
                                           engine::gfx::Palette* palette);

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_ASSETS_FILEPCX_H_
