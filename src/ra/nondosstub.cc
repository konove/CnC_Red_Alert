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
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "absl/base/attributes.h"
#include "engine/base/numeric.h"
#include "engine/file/game_file.h"
#include "engine/stream/byte_stream.h"
#include "engine/stream/seek_origin.h"
#include "ra/filepcx.h"
#include "ra/graphics_loader.h"
#include "ra/input.h"
#include "ra/interpal.h"
#include "ra/mapedit.h"
#include "ra/theme.h"
#include "ra/winstub.h"
#include "ra/world.h"
#include "sdllib/pixel_buffer.h"
#include "sdllib/ww_mouse.h"
#include "tech/audio_mixer.h"

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
  TheTheme().Suspend();
  TheAudio().Pause();
  if (TheMouse()) {
    WWMouseClass::Clear_Cursor_Clip();
  }
}

void Focus_Restore() {
  TheMap().Flag_To_Redraw(true);
  TheAudio().Resume();
  if (TheMouse()) {
    WWMouseClass::Set_Cursor_Clip();
  }
}

void Load_Title_Screen(std::string_view name, PixelView* video_page,
                       std::span<uint8_t> palette) {
  PixelBuffer* load_buffer =
      Read_PCX_File(std::string(name).c_str(), palette, {}, 0);

  if (load_buffer) {
    load_buffer->view().BlitTo(*video_page);
    delete load_buffer;
  }
}

/***************************************************************************
 * READ_PCX_FILE -- read a pcx file into a Graphic Buffer                  *
 *                                                                         *
 *	PixelBuffer* Read_PCX_File (char* name, char* palette ,void
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
 * OUTPUT: on success a pointer to a PixelBuffer containing the     *
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

  explicit BufferedFileReader(ByteStream& file ABSL_ATTRIBUTE_LIFETIME_BOUND)
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

  ByteStream& file_;

  // Use std::array for standard compliance and bounds awareness.
  std::array<uint8_t, kBufferSize> buffer_{};

  // cursor_ tracks current position; bytes_in_buffer_ tracks valid data range.
  size_t cursor_ = 0;
  size_t bytes_in_buffer_ = 0;
};

PixelBuffer* Read_PCX_File(const char* name, std::span<uint8_t> palette,
                           std::span<uint8_t> backing, int32_t size) {
  const auto file_handle = OpenGameFile(name);
  if (!file_handle) {
    return nullptr;
  }
  PCX_HEADER header{};
  if (!file_handle->ReadObject(header) || header.id != 10 ||
      header.version != 5 || header.pixelsize != 8 ||
      header.color_planes != 1 || header.encoding != 1) {
    return nullptr;
  }
  const int width = header.width - header.x + 1;
  int height = header.height - header.y + 1;
  if (width <= 0 || height <= 0 || header.byte_per_line < width || size < 0) {
    return nullptr;
  }
  if (base::ToSize(width) * base::ToSize(height) >
      static_cast<size_t>(std::numeric_limits<int32_t>::max())) {
    return nullptr;
  }
  if (!backing.empty()) {
    const auto available = size == 0
                               ? backing.size()
                               : std::min(backing.size(), base::ToSize(size));
    height =
        std::min(height, static_cast<int>(available / base::ToSize(width)));
    if (height <= 0) {
      return nullptr;
    }
  }
  auto pic = std::make_unique<PixelBuffer>(width, height, backing);
  const auto pixels = pic->bytes();
  BufferedFileReader reader(*file_handle);
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
    file_handle->Seek(-768, SeekOrigin::kEnd);
    if (file_handle->Read(std::as_writable_bytes(palette.first(768))) != 768) {
      return nullptr;
    }
    for (auto& color : palette.first(768)) {
      color >>= 2;
    }
  }
  return pic.release();
}
