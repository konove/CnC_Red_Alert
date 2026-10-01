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

/***************************************************************************
;**   C O N F I D E N T I A L --- W E S T W O O D   A S S O C I A T E S   **
;***************************************************************************
;*                                                                         *
;*                 Project Name : iff                                      *
;*                                                                         *
;*                    File Name : FILEPCX.H                                *
;*                                                                         *
;*                   Programmer : Julio R. Jerez                           *
;*                                                                         *
;*                   Start Date : May 2, 1995                              *
;*                                                                         *
;*                  Last Update : May 2, 1995   [JRJ]                      *
;*                                                                         *
;*-------------------------------------------------------------------------*
;* Functions:                                                              *
;* PixelBuffer* Read_PCX_File (char* name, BYTE* palette,void *buff, long
size);
;* int Write_PCX_File (char* name, PixelView& pic, BYTE* palette );*
;*= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =*/
#ifndef CNC_RED_ALERT_RA_ASSETS_FILEPCX_H_
#define CNC_RED_ALERT_RA_ASSETS_FILEPCX_H_

#include <cstdint>
#include <memory>

#include "engine/gfx/palette.h"
#include "engine/gfx/pixel_buffer.h"

class ByteStream;

namespace ra {

struct RGB {
  unsigned char red;
  unsigned char green;
  unsigned char blue;
};

struct PCX_HEADER {
  char id;
  char version;
  char encoding;
  char pixelsize;
  int16_t x;
  int16_t y;
  int16_t width;
  int16_t height;
  int16_t xres;
  int16_t yres;
  RGB ega_palette[16];
  char nothing;
  char color_planes;
  int16_t byte_per_line;
  int16_t palette_type;
  char filler[58];
};

// Reads the PCX picture `name` into a new buffer, backed by `backing` when it
// is not empty (at most `size` bytes of it, 0 meaning all). Fills `*palette`
// with the file's palette when `palette` is not nullptr. Returns null if the
// file is missing, malformed or does not fit.
std::unique_ptr<PixelBuffer> Read_PCX_File(const char* name,
                                           engine::gfx::Palette* palette,
                                           std::span<uint8_t> backing,
                                           int32_t size);

// Writes pic as a PCX picture with palette to file, which the caller has
// opened for writing; the picture starts at the stream's current position and
// the stream is left open. Returns 0.
int Write_PCX_File(ByteStream& file, PixelBuffer& pic,
                   const engine::gfx::Palette& palette);

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_ASSETS_FILEPCX_H_
