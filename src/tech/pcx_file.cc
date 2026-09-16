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
 **   C O N F I D E N T I A L --- W E S T W O O D   A S S O C I A T E S   **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : iff                                      *
 *                                                                         *
 *                    File Name : WRITEPCX.CPP                             *
 *                                                                         *
 *                   Programmer : Julio R. Jerez                           *
 *                                                                         *
 *                   Start Date : May 2, 1995                              *
 *                                                                         *
 *                  Last Update : May 2, 1995   [JRJ]                      *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 * int Save_PCX_File (char* name, GraphicViewPortClass& pic, char* palette)*
 *= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =*/

#include "tech/pcx_file.h"

#include <cstddef>
#include <cstdint>
#include <span>

#include "base/array.h"
#include "base/buffer.h"
#include "base/numeric.h"
#include "sdllib/file.h"
#include "sdllib/file_access.h"
#include "sdllib/gbuffer.h"

static void Write_Pcx_ScanLine(int file_handle,
                               std::span<const uint8_t> pixels);

/***************************************************************************
 * WRITE_PCX_FILE -- Write the data in ViewPort to a pcx file              *
 *                                                                         *
 *                                                                         *
 *                                                                         *
 * INPUT:  name is a NULL terminated string of the fromat [xxxx.pcx]
 ** pic	 is a pointer to a GraphicViewPortClass or to a
 ** GraphicBufferClass holding the picture.
 ** palette is a pointer the the memry block holding the color 		*
 ** palette of the picture.                                    *
 *                                                                         *
 * OUTPUT: FALSE  if the function fails zero otherwise *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   05/04/1995 JRJ : Created.                                             *
 *   08/01/1995 SKB : Copy the palette so it is not modified.              *
 *=========================================================================*/
int Write_PCX_File(const char* name, GraphicViewPortClass& pic,
                   std::span<const unsigned char> palette) {
  unsigned char palcopy[256 * 3];
  unsigned i = 0;
  PCX_HEADER header = {10,  5,   1,  8, 0, 0,   319, 199,
                       320, 200, {}, 0, 1, 320, 1,   {}};

  // Open file name
  const int file_handle = OpenFileHandle(name, FileAccess::kWrite);
  if (file_handle == -1) {
    return 0;
  }

  header.width = static_cast<int16_t>(pic.Get_Width() - 1);
  header.height = static_cast<int16_t>(pic.Get_Height() - 1);
  header.byte_per_line = static_cast<int16_t>(pic.Get_Width());
  WriteFileHandle(file_handle, base::ObjectBytes(header));

  const int VP_Scan_Line = pic.Get_Width() + pic.Get_XAdd();
  GraphicBufferClass* Graphic_Buffer = pic.Get_Graphic_Buffer();
  const auto pixels = Graphic_Buffer->Get_Bytes().subspan(
      base::ToSize((pic.Get_YPos() * VP_Scan_Line) + pic.Get_XPos()));
  for (i = 0; i < static_cast<unsigned>(header.height) + 1; i++) {
    Write_Pcx_ScanLine(
        file_handle,
        pixels.subspan(i * static_cast<std::size_t>(VP_Scan_Line),
                       static_cast<std::size_t>(header.byte_per_line)));
  }
  base::CopyBytes(base::ObjectBytes(palcopy), std::as_bytes(palette),
                  sizeof(palcopy));
  // Scale the 6-bit palette components to 8 bits.
  for (unsigned char& component : palcopy) {
    component = static_cast<unsigned char>(component << 2);
  }
  i = 0x0c;
  WriteFileHandle(file_handle, base::ObjectBytes(i).first(1));
  WriteFileHandle(file_handle, base::ObjectBytes(palcopy));
  CloseFileHandle(file_handle);
  return 0;
}

/***************************************************************************
 * WRITE_PCX_SCANLINE -- function to write a single pcx scanline to a file *
 *                                                                         *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   05/04/1995 JRJ : Created.                                             *
 *=========================================================================*/

constexpr int kPoolSize = 2048;
void Write_Pcx_ScanLine(int file_handle, std::span<const uint8_t> pixels) {
  unsigned char pool[kPoolSize];

  std::size_t used = 0;
  if (pixels.empty()) {
    return;
  }

  const auto write_char = [&](unsigned char x) {
    base::At(pool, used++) = x;
    if (used >= kPoolSize) {
      WriteFileHandle(file_handle, base::ObjectBytes(pool));
      used = 0;
    }
  };
  unsigned last = pixels.front();
  unsigned rle = 1;

  for (unsigned i = 1; i < pixels.size(); i++) {
    const unsigned color = pixels[i];
    if (color == last) {
      rle++;
      if (rle == 63) {
        write_char(255);
        write_char(static_cast<unsigned char>(color));
        rle = 0;
      }
    } else {
      if (rle) {
        if (rle == 1 && (192 != (192 & last))) {
          write_char(static_cast<unsigned char>(last));
        } else {
          write_char(static_cast<unsigned char>(rle | 192));
          write_char(static_cast<unsigned char>(last));
        }
      }
      last = color;
      rle = 1;
    }
  }
  if (rle) {
    if (rle == 1 && (192 != (192 & last))) {
      write_char(static_cast<unsigned char>(last));
    } else {
      write_char(static_cast<unsigned char>(rle | 192));
      write_char(static_cast<unsigned char>(last));
    }
  }

  WriteFileHandle(file_handle, base::ObjectBytes(pool).first(used));
}
