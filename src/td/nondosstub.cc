// parts of winstub that didn't depend on windows
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

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "td/ccfile.h"
#include "td/compat.h"
#include "td/defines.h"
#include "td/externs.h"
// #include "ra/filepcx.h"
#include "sdllib/include/gbuffer.h"
#include "sdllib/include/iconcach.h"
#include "sdllib/include/ww_audio.h"
#include "sdllib/include/ww_mouse.h"
#include "td/conquer.h"
#include "td/interpal.h"
#include "td/mapedit.h"
#include "td/palette.h"
#include "td/theme.h"

typedef struct {
  char red;
  char green;
  char blue;
} RGB;

typedef struct {
  char id;
  char version;
  char encoding;
  char pixelsize;
  short x;
  short y;
  short width;
  short height;
  short xres;
  short yres;
  RGB ega_palette[16];
  char nothing;
  char color_planes;
  short byte_per_line;
  short palette_type;
  char filler[58];
} PCX_HEADER;

void output(short, short) {}

ThemeType OldTheme = THEME_NONE;
extern bool InMovie;

/***********************************************************************************************
 * Focus_Loss -- this function is called when a library function detects focus
 *loss            *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 2/1/96 2:10PM ST : Created *
 *=============================================================================================*/

void Focus_Loss() {
  if (SoundOn) {
    if (OldTheme == THEME_NONE) {
      OldTheme = Theme.What_Is_Playing();
    }
  }
  Theme.Stop();
  Stop_Primary_Sound_Buffer();
  if (WWMouse) {
    WWMouse->Clear_Cursor_Clip();
  }
}

void Focus_Restore() {
  Restore_Cached_Icons();
  Map.Flag_To_Redraw(true);
  Start_Primary_Sound_Buffer(true);

  if (!InMovie) {
    Theme.Queue_Song(OldTheme);
    OldTheme = THEME_NONE;
  }

  if (WWMouse) {
    WWMouse->Set_Cursor_Clip();
  }
#ifndef PORTABLE
  VisiblePage.Clear();
  HiddenPage.Clear();
#endif
}

unsigned char* VQPalette;
long VQNumBytes;
unsigned long VQSlowpal;
bool VQPaletteChange = false;

extern "C" {
void __cdecl SetPalette(unsigned char* palette, long numbytes,
                        unsigned long slowpal);
}

void Flag_To_Set_Palette(unsigned char* palette, long numbytes,
                         unsigned long slowpal) {
  VQPalette = palette;
  VQNumBytes = numbytes;
  VQSlowpal = slowpal;
  VQPaletteChange = true;
}

void Check_VQ_Palette_Set() {
  if (VQPaletteChange) {
    SetPalette(VQPalette, VQNumBytes, VQSlowpal);
    VQPaletteChange = false;
  }
}

void __cdecl SetPalette(unsigned char* palette, long, unsigned long) {
  for (int i = 0; i < 256 * 3; i++) {
    *(palette + i) &= 63;
  }
  Increase_Palette_Luminance(palette, 15, 15, 15, 63);

  if (PalettesRead) {
    memcpy(&PaletteInterpolationTable[0][0],
           InterpolatedPalettes[PaletteCounter++], 65536);
  }

  Set_Palette(palette);
}

GraphicBufferClass* Read_PCX_File(const char* name, char* Palette, void* Buff,
                                  long Size);

/***********************************************************************************************
 * Load_Title_Screen -- loads the title screen into the given video buffer *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    screen name * video buffer * ptr to buffer for palette *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 7/5/96 11:30AM ST : Created *
 *=============================================================================================*/

void Load_Title_Screen(const char* name, GraphicViewPortClass* video_page,
                       unsigned char* palette) {
  GraphicBufferClass* load_buffer;

  load_buffer = Read_PCX_File(name, (char*)palette, nullptr, 0);

  if (load_buffer) {
    load_buffer->Blit(*video_page);
    delete load_buffer;
  }
}

/***************************************************************************
 * READ_PCX_FILE -- read a pcx file into a Graphic Buffer                  *
 *                                                                         *
 *	GraphicBufferClass* Read_PCX_File (char* name, char* palette ,void
 **Buff, long size );	*
 *  																								*
 *                                                                         *
 * INPUT: name is a NULL terminated string of the format [xxxx.pcx]        *
 *        palette is optional, if palette != NULL the the color palette of *
 *					 the pcx file will be place in the
 *memory block pointed	   * by palette.
 ** Buff is optional, if Buff == NULL a new memory Buffer
 ** will be allocated, otherwise the file will be placed 		* at
 *location pointed by Buffer;
 ** Size is the size in bytes of the memory block pointed by Buff * is also
 *optional;
 **                                                                         *
 * OUTPUT: on success a pointer to a GraphicBufferClass containing the     *
 *         pcx file, NULL otherwise.                                       *
 *																									*
 * WARNINGS:                                                               *
 *         Appears to be a comment-free zone                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   05/03/1995 JRJ : Created.                                             *
 *   04/30/1996 ST : Tidied up and modified to use CCFileClass             *
 *=========================================================================*/

class BufferedFileReader {
 public:
  static constexpr size_t kBufferSize = 2048;

  explicit BufferedFileReader(CCFileClass& file) : file_(file) {}

  // Delete copy/move to prevent accidental state duplication.
  BufferedFileReader(const BufferedFileReader&) = delete;
  BufferedFileReader& operator=(const BufferedFileReader&) = delete;

  // Returns the next byte, or an OutOfRange error on EOF.
  absl::StatusOr<uint8_t> ReadByte() {
    if (cursor_ >= bytes_in_buffer_) {
      if (!RefillBuffer()) {
        return absl::OutOfRangeError("End of file reached.");
      }
    }
    return buffer_[cursor_++];
  }

 private:
  // Returns true if data was successfully read, false on EOF.
  bool RefillBuffer() {
    cursor_ = 0;
    // Track exactly how many bytes were read.
    bytes_in_buffer_ = file_.Read(buffer_.data(), kBufferSize);
    return bytes_in_buffer_ > 0;
  }

  CCFileClass& file_;

  // Use std::array for standard compliance and bounds awareness.
  std::array<uint8_t, kBufferSize> buffer_{};

  // cursor_ tracks current position; bytes_in_buffer_ tracks valid data range.
  size_t cursor_ = 0;
  size_t bytes_in_buffer_ = 0;
};

GraphicBufferClass* Read_PCX_File(const char* name, char* palette, void* Buff,
                                  long Size) {
  CCFileClass file_handle(name);

  if (!file_handle.Is_Available()) {
    return nullptr;
  }

  file_handle.Open(READ);

  PCX_HEADER header;
  file_handle.Read(&header, sizeof(PCX_HEADER));

  if (header.id != 10 && header.version != 5 && header.pixelsize != 8) {
    return nullptr;
  }

  int width = header.width - header.x + 1;
  int height = header.height - header.y + 1;

  GraphicBufferClass* pic;
  char* buffer;

  if (Buff) {
    buffer = static_cast<char*>(Buff);
    int max_lines = Size / width;
    height = std::min(max_lines - 1, height);
    pic = new GraphicBufferClass(width, height, buffer, Size);
    if (!pic->Get_Buffer()) {
      delete pic;
      return nullptr;
    }
  } else {
    pic = new GraphicBufferClass(width, height, nullptr, width * (height + 4));
    if (!pic->Get_Buffer()) {
      delete pic;
      return nullptr;
    }
  }

  buffer = static_cast<char*>(pic->Get_Buffer());
  BufferedFileReader reader(file_handle);

  if (header.byte_per_line != width) {
    for (unsigned scan_pos = 0, j = 0; j < static_cast<unsigned>(height);
         j++, scan_pos += width) {
      for (int i = 0; i < width;) {
        unsigned rle = *reader.ReadByte();
        if (rle > 192) {
          rle -= 192;
          unsigned color = *reader.ReadByte();
          memset(buffer + scan_pos + i, color, rle);
          i += rle;
        } else {
          buffer[scan_pos + i++] = static_cast<char>(rle);
        }
      }
    }

    // Consume any trailing RLE data for the scanline
    unsigned rle = *reader.ReadByte();
    if (rle > 192) {
      (void)reader.ReadByte();
    }

  } else {
    for (int i = 0; i < width * height;) {
      unsigned rle = *reader.ReadByte() & 0xff;
      if (rle > 192) {
        rle -= 192;
        unsigned color = *reader.ReadByte();
        memset(buffer + i, color, rle);
        i += rle;
      } else {
        buffer[i++] = static_cast<char>(rle);
      }
    }
  }

  if (palette) {
    file_handle.Seek(-static_cast<int>(256 * sizeof(RGB)), SEEK_END);
    file_handle.Read(palette, 256L * sizeof(RGB));

    auto* pal = reinterpret_cast<RGB*>(palette);
    for (int i = 0; i < 256; i++) {
      pal->red >>= 2;
      pal->green >>= 2;
      pal->blue >>= 2;
      pal++;
    }
  }

  file_handle.Close();
  return pic;
}
