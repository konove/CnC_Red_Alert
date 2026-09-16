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
#include <optional>
#include <string>
#include <string_view>

#include "absl/base/attributes.h"
#include "base/array.h"
#include "base/numeric.h"
#include "base/seek_origin.h"
#include "ra/externs.h"
#include "ra/filepcx.h"
#include "ra/graphics_loader.h"
#include "ra/interpal.h"
#include "ra/mapedit.h"
#include "ra/palette.h"
#include "ra/theme.h"
#include "sdllib/file_access.h"
#include "sdllib/gbuffer.h"
#include "sdllib/iconcach.h"
#include "sdllib/ww_audio.h"
#include "sdllib/ww_mouse.h"
#include "tech/game_file.h"

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
  Theme.Suspend();
  Stop_Primary_Sound_Buffer();
  if (WWMouse) {
    WWMouseClass::Clear_Cursor_Clip();
  }
}

void Focus_Restore() {
  Restore_Cached_Icons();
  Map.Flag_To_Redraw(true);
  Start_Primary_Sound_Buffer(true);
  if (WWMouse) {
    WWMouseClass::Set_Cursor_Clip();
  }
}

static unsigned char* VQPalette;
static int32_t VQNumBytes;
static uint32_t VQSlowpal;
static bool VQPaletteChange = false;

extern "C" {
void __cdecl SetPalette(unsigned char* palette, int32_t numbytes,
                        uint32_t slowpal);
}

void Flag_To_Set_Palette(unsigned char* palette, int32_t numbytes,
                         uint32_t slowpal) {
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

void __cdecl SetPalette(unsigned char* palette, int32_t /*unused*/,
                        uint32_t /*unused*/) {
  for (int i = 0; i < 256 * 3; i++) {
    *(palette + i) &= 63;
  }
  Increase_Palette_Luminance(palette, 15, 15, 15, 63);
  if (PalettesRead) {
    memcpy(&PaletteInterpolationTable[0][0],
           base::At(InterpolatedPalettes, PaletteCounter++), 65536);
  }
  Set_Palette(palette);
}

void Load_Title_Screen(std::string_view name, GraphicViewPortClass* video_page,
                       unsigned char* palette) {
  GraphicBufferClass* load_buffer =
      Read_PCX_File(std::string(name).c_str(), palette, nullptr, 0);

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
 *   04/30/1996 ST : Tidied up and modified to use GameFile             *
 *=========================================================================*/

class BufferedFileReader {
 public:
  static constexpr size_t kBufferSize = 2048;

  explicit BufferedFileReader(GameFile& file ABSL_ATTRIBUTE_LIFETIME_BOUND)
      : file_(file) {}

  // Delete copy/move to prevent accidental state duplication.
  ~BufferedFileReader() = default;
  BufferedFileReader(const BufferedFileReader&) = delete;
  BufferedFileReader& operator=(const BufferedFileReader&) = delete;
  BufferedFileReader(BufferedFileReader&&) = delete;
  BufferedFileReader& operator=(BufferedFileReader&&) = delete;

  // Returns the next byte, or nullopt at end of file.
  std::optional<uint8_t> ReadByte() {
    if ((cursor_ >= bytes_in_buffer_) && (!RefillBuffer())) {
      return std::nullopt;
    }

    return buffer_[cursor_++];
  }

 private:
  // Returns true if data was successfully read, false on EOF.
  bool RefillBuffer() {
    cursor_ = 0;
    // Track exactly how many bytes were read.
    bytes_in_buffer_ = base::ToSize(file_.Read(buffer_.data(), kBufferSize));
    return bytes_in_buffer_ > 0;
  }

  GameFile& file_;

  // Use std::array for standard compliance and bounds awareness.
  std::array<uint8_t, kBufferSize> buffer_{};

  // cursor_ tracks current position; bytes_in_buffer_ tracks valid data range.
  size_t cursor_ = 0;
  size_t bytes_in_buffer_ = 0;
};

GraphicBufferClass* Read_PCX_File(const char* name, unsigned char* palette,
                                  void* Buff, int32_t Size) {
  GameFile file_handle(name);

  if (!file_handle.IsAvailable()) {
    return nullptr;
  }

  file_handle.Open(FileAccess::kRead);

  PCX_HEADER header;
  file_handle.ReadObject(header);

  if (header.id != 10 && header.version != 5 && header.pixelsize != 8) {
    return nullptr;
  }

  const int width = header.width - header.x + 1;
  int32_t height = header.height - header.y + 1;

  GraphicBufferClass* pic = nullptr;
  char* buffer = nullptr;

  if (Buff) {
    buffer = static_cast<char*>(Buff);
    const auto max_lines = static_cast<int32_t>(Size / width);
    height = std::min(max_lines - 1, height);
    pic = new GraphicBufferClass(width, height, buffer, Size);
    if (!pic->Get_Buffer()) {
      delete pic;
      return nullptr;
    }
  } else {
    pic = new GraphicBufferClass(width, height, nullptr,
                                 static_cast<int32_t>(width) * (height + 4));
    if (!pic->Get_Buffer()) {
      delete pic;
      return nullptr;
    }
  }

  buffer = static_cast<char*>(pic->Get_Buffer());
  BufferedFileReader reader(file_handle);

  if (header.byte_per_line != width) {
    for (int scan_pos = 0, j = 0; j < height; j++, scan_pos += width) {
      for (int i = 0; i < width;) {
        const auto rle_result = reader.ReadByte();
        if (!rle_result.has_value()) {
          delete pic;
          return nullptr;
        }
        int rle = *rle_result;
        if (rle > 192) {
          rle -= 192;
          const auto color_result = reader.ReadByte();
          if (!color_result.has_value()) {
            delete pic;
            return nullptr;
          }
          const int color = *color_result;
          memset(buffer + scan_pos + i, color, base::ToSize(rle));
          i += rle;
        } else {
          buffer[scan_pos + i++] = static_cast<char>(rle);
        }
      }
    }

    // Consume any trailing RLE data for the scanline
    const auto rle_result = reader.ReadByte();
    if (!rle_result.has_value()) {
      delete pic;
      return nullptr;
    }
    const int rle = *rle_result;
    if ((rle > 192) && (!reader.ReadByte().has_value())) {
      delete pic;
      return nullptr;
    }

  } else {
    for (int i = 0; i < width * height;) {
      const auto rle_result = reader.ReadByte();
      if (!rle_result.has_value()) {
        delete pic;
        return nullptr;
      }
      int rle = *rle_result;
      if (rle > 192) {
        rle -= 192;
        const auto color_result = reader.ReadByte();
        if (!color_result.has_value()) {
          delete pic;
          return nullptr;
        }
        const int color = *color_result;
        memset(buffer + i, color, base::ToSize(rle));
        i += rle;
      } else {
        buffer[i++] = static_cast<char>(rle);
      }
    }
  }

  if (palette) {
    file_handle.Seek(-static_cast<int>(256 * sizeof(RGB)), SeekOrigin::kEnd);
    file_handle.Read(palette, 256L * sizeof(RGB));

    for (int i = 0; i < 256 * 3; i++) {
      palette[i] >>= 2;
    }
  }

  file_handle.Close();
  return pic;
}
