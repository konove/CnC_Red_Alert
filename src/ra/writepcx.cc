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

/* $Header: /CounterStrike/WRITEPCX.CPP 1     3/03/97 10:26a Joe_bostic $ */
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

#include <cstdint>
#include <cstring>
#include <span>

#include "base/buffer.h"
#include "ra/filepcx.h"
#include "ra/palette.h"
#include "sdllib/file_access.h"
#include "sdllib/gbuffer.h"
#include "tech/file.h"

static void Write_Pcx_ScanLine(File& file, int scansize,
                               std::span<const uint8_t> pixels);

/***************************************************************************
 * WRITE_PCX_FILE -- Write the data in ViewPort to a pcx file              *
 *                                                                         *
 *                                                                         *
 *                                                                         *
 * INPUT:  name is a NULL terminated string of the format [xxxx.pcx]
 ** pic	 is a pointer to a GraphicViewPortClass or to a
 ** GraphicBufferClass holding the picture.
 ** palette is a pointer the the memory block holding the color * palette of the
 *picture.                                    *
 *                                                                         *
 * OUTPUT: FALSE  if the function fails zero otherwise *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   05/04/1995 JRJ : Created.                                             *
 *   08/01/1995 SKB : Copy the palette so it is not modified.              *
 *   06/03/1996 JLB : Converted to C++ and file class I/O.                 *
 *=========================================================================*/

static const unsigned char rle_code = 0xC0;     // Run code.
static const unsigned char rle_max_run = 0x2F;  // Maximum run allowed.
static const unsigned char rle_full_run =
    rle_max_run | rle_code;  // Full character run.

/***********************************************************************************************
 * Write_PCX_File -- Write a PCX file from specified buffer. *
 *                                                                                             *
 *    This routine will take the specified buffer and write out the data as a
 *PCX file to the  * the file object specified. *
 *                                                                                             *
 * INPUT:   file     -- Reference to the file object to write the buffer as a
 *PCX file.        *
 *                                                                                             *
 *          pic      -- Reference to a graphic buffer that contains the data to
 *be written.    *
 *                                                                                             *
 *          palette  -- Reference to the palette to be attached to the PCX file
 *as well.       *
 *                                                                                             *
 * OUTPUT:  bool; Was there an error? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/03/1996 JLB : Created. *
 *=============================================================================================*/
int Write_PCX_File(File& file, GraphicBufferClass& pic,
                   const PaletteClass* palette) {
  unsigned char palcopy[256 * sizeof(RGB)];
  const PCX_HEADER header = {10,
                             5,
                             1,
                             8,
                             0,
                             0,
                             static_cast<int16_t>(pic.Get_Width() - 1),
                             static_cast<int16_t>(pic.Get_Height() - 1),
                             static_cast<int16_t>(pic.Get_Width()),
                             static_cast<int16_t>(pic.Get_Height()),
                             {},
                             0,
                             1,
                             static_cast<int16_t>(pic.Get_Width()),
                             1,
                             {0}};

  /*
  **	Open the output file and write out the header information. If the file
  **	is already open, then just presume that it is positioned correctly and
  *is *	open for write.
  */
  bool open = false;
  if (!file.IsOpen()) {
    file.Open(FileAccess::kWrite);
    open = true;
  }
  file.WriteObject(header);

  /*
  **	Write out the picture, line by line.
  */
  const int VP_Scan_Line = pic.Get_Width() + pic.Get_XAdd();
  const auto pixels = pic.Get_Pixels();
  for (int line = 0; line < header.height + 1; line++) {
    Write_Pcx_ScanLine(file, header.byte_per_line,
                       pixels.subspan(static_cast<size_t>(line) *
                                      static_cast<size_t>(VP_Scan_Line)));
  }

  /*
  **	Special marker for end of RLE data.
  */
  const unsigned char ender = 0x0C;
  file.WriteObject(ender);

  /*
  **	Convert the palette from 6 bit to 8 bit format.
  */
  base::CopyBytes(base::ObjectBytes(palcopy), std::as_bytes(palette->bytes()),
                  sizeof(palcopy));
  for (unsigned char& component : palcopy) {
    component = static_cast<unsigned char>(component << 2);
  }

  /*
  **	Write the palette out.
  */
  file.WriteObject(palcopy);

  /*
  **	Close the file (if necessary) and exit with no error flag.
  */
  if (open) {
    file.Close();
  }
  return 0;
}

/***********************************************************************************************
 * Write_Pcx_ScanLine -- Writes a PCX scanline. *
 *                                                                                             *
 *    Writes out a PCX scanline using RLE compression. *
 *                                                                                             *
 * INPUT:   file     -- Reference to the file to write the scan line to. *
 *                                                                                             *
 *          scansize -- The number of bytes to compress (write). *
 *                                                                                             *
 *          ptr      -- Pointer to the data to compress (write). *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/04/1995 JRJ : Created. * 06/03/1996 JLB : Converted to C++ and
 *file class I/O.                                     *
 *=============================================================================================*/
static void Write_Pcx_ScanLine(File& file, int scansize,
                               std::span<const uint8_t> pixels) {
  if (scansize <= 0 || static_cast<size_t>(scansize) > pixels.size()) {
    return;
  }
  auto last = pixels.front();
  unsigned char rle = 1;
  unsigned char c = 0;
  for (int i = 1; i < scansize; i++) {
    const auto color = pixels[static_cast<size_t>(i)];
    if (color == last) {
      rle++;
      if (rle == rle_max_run) {
        file.WriteObject(rle_full_run);
        file.WriteObject(color);
        rle = 0;
      }
    } else {
      if (rle) {
        if (rle == 1 && rle_code != (rle_code & last)) {
          file.WriteObject(last);
        } else {
          c = static_cast<unsigned char>(rle | rle_code);
          file.WriteObject(c);
          file.WriteObject(last);
        }
      }
      last = color;
      rle = 1;
    }
  }
  if (rle) {
    if (rle == 1 && rle_code != (rle_code & last)) {
      file.WriteObject(last);
    } else {
      c = static_cast<unsigned char>(rle | rle_code);
      file.WriteObject(c);
      file.WriteObject(last);
    }
  }
}
