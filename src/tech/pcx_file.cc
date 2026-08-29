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

#include "base/types.h"
#include "sdllib/file.h"
#include "sdllib/gbuffer.h"
#include "sdllib/memflag.h"

static void Write_Pcx_ScanLine(int file_handle, int scansize, char* ptr);

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
int Write_PCX_File(char* name, GraphicViewPortClass& pic,
                   unsigned char* palette) {
  unsigned char palcopy[256 * 3];
  unsigned i;
  int file_handle;
  int VP_Scan_Line;
  char* ptr;
  RGB* pal;
  GraphicBufferClass* Graphic_Buffer;
  PCX_HEADER header = {10,  5,   1,  8, 0, 0,   319, 199,
                       320, 200, {}, 0, 1, 320, 1,   {}};

  // Open file name
  file_handle = Open_File(name, FileAccess::kWrite);
  if (file_handle == -1) {
    return false;
  }

  header.width = static_cast<short>(pic.Get_Width() - 1);
  header.height = static_cast<short>(pic.Get_Height() - 1);
  header.byte_per_line = static_cast<short>(pic.Get_Width());
  Write_File(file_handle, &header, sizeof(PCX_HEADER));

  VP_Scan_Line = pic.Get_Width() + pic.Get_XAdd();
  Graphic_Buffer = pic.Get_Graphic_Buffer();
  ptr = (char*)Graphic_Buffer->Get_Buffer();
  ptr += ((pic.Get_YPos() * VP_Scan_Line) + pic.Get_XPos());

  for (i = 0; i < static_cast<unsigned>(header.height) + 1; i++) {
    Write_Pcx_ScanLine(file_handle, header.byte_per_line,
                       ptr + static_cast<base::ssize>(i) * VP_Scan_Line);
  }

  Mem_Copy(palette, palcopy, 256UL * 3);
  pal = (RGB*)palcopy;
  for (i = 0; i < 256; i++) {
    pal->red <<= 2;
    pal->green <<= 2;
    pal->blue <<= 2;
    pal++;
  }
  i = 0x0c;
  Write_File(file_handle, &i, 1);
  Write_File(file_handle, palcopy, 256 * sizeof(RGB));
  Close_File(file_handle);
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
void Write_Pcx_ScanLine(int file_handle, int scansize, char* ptr) {
  unsigned i;
  unsigned rle;
  unsigned color;
  unsigned last;
  char* file_ptr;
  char pool[kPoolSize];

  file_ptr = pool;

  const auto write_char = [&](unsigned char x) {
    *file_ptr++ = x;
    if (file_ptr >= &pool[kPoolSize]) {
      Write_File(file_handle, pool, kPoolSize);
      file_ptr = pool;
    }
  };
  last = static_cast<unsigned char>(*ptr);
  rle = 1;

  for (i = 1; i < static_cast<unsigned>(scansize); i++) {
    color = 0xff & *++ptr;
    if (color == last) {
      rle++;
      if (rle == 63) {
        write_char(255);
        write_char(color);
        rle = 0;
      }
    } else {
      if (rle) {
        if (rle == 1 && (192 != (192 & last))) {
          write_char(last);
        } else {
          write_char(rle | 192);
          write_char(last);
        }
      }
      last = color;
      rle = 1;
    }
  }
  if (rle) {
    if (rle == 1 && (192 != (192 & last))) {
      write_char(last);
    } else {
      write_char(rle | 192);
      write_char(last);
    }
  }

  Write_File(file_handle, pool, file_ptr - pool);
}
