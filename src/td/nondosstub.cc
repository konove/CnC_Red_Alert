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
#include <memory>
#include <optional>
#include <span>

#include "absl/base/attributes.h"
#include "base/array.h"
#include "base/buffer.h"
#include "base/numeric.h"
#include "base/seek_origin.h"
#include "sdllib/file_access.h"
#include "td/defines.h"
#include "td/externs.h"
#include "tech/game_file.h"
// #include "ra/filepcx.h"
#include "sdllib/gbuffer.h"
#include "sdllib/iconcach.h"
#include "sdllib/ww_audio.h"
#include "sdllib/ww_mouse.h"
#include "td/conquer.h"
#include "td/globals.h"
#include "td/interpal.h"
#include "td/mapedit.h"
#include "td/palette.h"
#include "td/theme.h"
#include "tech/pcx_file.h"
#include "winvq/vqa32/vqaplay.h"

static ThemeType OldTheme = THEME_NONE;

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
  if (SoundOn && (OldTheme == THEME_NONE)) {
    OldTheme = Theme.What_Is_Playing();
  }

  Theme.Stop();
  Audio.Pause();
  if (WWMouse) {
    WWMouseClass::Clear_Cursor_Clip();
  }
}

void Focus_Restore() {
  Restore_Cached_Icons();
  Map.Flag_To_Redraw(true);
  Audio.Resume();

  if (!InMovie) {
    Theme.Queue_Song(OldTheme);
    OldTheme = THEME_NONE;
  }

  if (WWMouse) {
    WWMouseClass::Set_Cursor_Clip();
  }
}

static std::span<unsigned char> VQPalette;
static int32_t VQNumBytes;
static uint32_t VQSlowpal;
bool VQPaletteChange = false;

extern "C" {
void __cdecl SetPalette(std::span<unsigned char> palette, int32_t numbytes,
                        uint32_t slowpal);
}

void Flag_To_Set_Palette(std::span<unsigned char> palette, int32_t numbytes,
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

void __cdecl SetPalette(std::span<unsigned char> palette, int32_t /*unused*/,
                        uint32_t /*unused*/) {
  for (int i = 0; i < 256 * 3; i++) {
    base::At(palette, base::ToSize(i)) &= 63;
  }
  Increase_Palette_Luminance(palette, 15, 15, 15, 63);

  if (PalettesRead) {
    base::CopyBytes(base::ObjectBytes(PaletteInterpolationTable),
                    std::as_bytes(std::span(
                        base::At(InterpolatedPalettes, PaletteCounter++))),
                    sizeof(PaletteInterpolationTable));
  }

  Set_Palette(palette);
}

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
                       std::span<unsigned char> palette) {
  GraphicBufferClass* load_buffer = Read_PCX_File(name, palette, {}, 0);

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

    return buffer_.at(cursor_++);
  }

 private:
  // Returns true if data was successfully read, false on EOF.
  bool RefillBuffer() {
    cursor_ = 0;
    // Track exactly how many bytes were read.
    bytes_in_buffer_ =
        base::ToSize(file_.Read(std::as_writable_bytes(std::span(buffer_))));
    return bytes_in_buffer_ > 0;
  }

  GameFile& file_;

  // Use std::array for standard compliance and bounds awareness.
  std::array<uint8_t, kBufferSize> buffer_{};

  // cursor_ tracks current position; bytes_in_buffer_ tracks valid data range.
  size_t cursor_ = 0;
  size_t bytes_in_buffer_ = 0;
};

GraphicBufferClass* Read_PCX_File(const char* name, std::span<uint8_t> palette,
                                  std::span<uint8_t> buff, int32_t size) {
  GameFile file_handle(name);
  if (!file_handle.IsAvailable() || !file_handle.Open(FileAccess::kRead)) {
    return nullptr;
  }
  PCX_HEADER header{};
  if (!file_handle.ReadObject(header) || header.id != 10 ||
      header.version != 5 || header.pixelsize != 8 ||
      header.color_planes != 1 || header.encoding != 1) {
    return nullptr;
  }
  const int width = header.width - header.x + 1;
  int height = header.height - header.y + 1;
  if (width <= 0 || height <= 0 || header.byte_per_line < width || size < 0 ||
      static_cast<int64_t>(width) * height > INT32_MAX) {
    return nullptr;
  }
  if (!buff.empty()) {
    const auto available =
        size == 0 ? buff.size() : std::min(buff.size(), base::ToSize(size));
    height =
        std::min(height, static_cast<int>(available / base::ToSize(width)));
    if (height <= 0) {
      return nullptr;
    }
  }
  auto pic = std::make_unique<GraphicBufferClass>(width, height, buff);
  const auto pixels = pic->Get_Bytes();
  BufferedFileReader reader(file_handle);
  for (int row = 0; row < height; ++row) {
    int column = 0;
    while (column < header.byte_per_line) {
      const auto code = reader.ReadByte();
      if (!code) {
        return nullptr;
      }
      int count = 1;
      uint8_t color = *code;
      if ((color & 0xc0U) == 0xc0U) {
        count = color & 0x3fU;
        const auto value = reader.ReadByte();
        if (!value || count == 0) {
          return nullptr;
        }
        color = *value;
      }
      if (count > header.byte_per_line - column) {
        return nullptr;
      }
      const int visible = std::min(count, std::max(0, width - column));
      if (visible > 0) {
        std::ranges::fill(pixels.subspan(base::ToSize((row * width) + column),
                                         base::ToSize(visible)),
                          color);
      }
      column += count;
    }
  }
  if (!palette.empty()) {
    if (palette.size() < 768) {
      return nullptr;
    }
    file_handle.Seek(-768, SeekOrigin::kEnd);
    if (file_handle.Read(std::as_writable_bytes(palette.first(768))) != 768) {
      return nullptr;
    }
    for (auto& color : palette.first(768)) {
      color >>= 2;
    }
  }
  return pic.release();
}
