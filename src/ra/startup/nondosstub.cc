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
#include "base/numeric.h"
#include "engine/audio/audio_mixer.h"
#include "engine/file/game_file.h"
#include "engine/gfx/palette.h"
#include "engine/gfx/pcx_file.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/stream/byte_stream.h"
#include "engine/stream/seek_origin.h"
#include "engine/window/hardware_cursor.h"
#include "ra/assets/filepcx.h"
#include "ra/assets/graphics_loader.h"
#include "ra/assets/theme.h"
#include "ra/editor/mapedit.h"
#include "ra/map/world.h"
#include "ra/startup/winstub.h"

namespace ra {

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
  engine::audio::TheAudio().Pause();
  engine::window::HardwareCursor::ReleaseMouse();
}

void Focus_Restore() {
  TheMap().Flag_To_Redraw(true);
  engine::audio::TheAudio().Resume();
  engine::window::HardwareCursor::GrabMouse();
}

void Load_Title_Screen(std::string_view name, PixelView* video_page,
                       engine::gfx::Palette& palette) {
  const auto load_buffer = Read_PCX_File(std::string(name).c_str(), &palette);

  if (load_buffer) {
    load_buffer->view().BlitTo(*video_page);
  }
}

// Reads a stream a byte at a time through a buffer of its own, so that
// decoding a picture does not cost a file read a byte.
class BufferedFileReader {
 public:
  static constexpr size_t kBufferSize = 2048;

  explicit BufferedFileReader(ByteStream& file ABSL_ATTRIBUTE_LIFETIME_BOUND)
      : file_(file) {}

  // Neither copied nor moved: a copy would read on from the same place in the
  // stream with its own stale buffer.
  ~BufferedFileReader() = default;
  BufferedFileReader(const BufferedFileReader&) = delete;
  BufferedFileReader& operator=(const BufferedFileReader&) = delete;
  BufferedFileReader(BufferedFileReader&&) = delete;
  BufferedFileReader& operator=(BufferedFileReader&&) = delete;

  // Returns the next byte, or nullopt at the end of the stream.
  std::optional<uint8_t> ReadByte() {
    if ((cursor_ >= bytes_in_buffer_) && (!RefillBuffer())) {
      return std::nullopt;
    }

    return buffer_.at(cursor_++);
  }

 private:
  // Reads the next part of the stream into the buffer. Returns false at the
  // end of the stream.
  bool RefillBuffer() {
    cursor_ = 0;
    bytes_in_buffer_ =
        base::ToSize(file_.Read(std::as_writable_bytes(std::span(buffer_))));
    return bytes_in_buffer_ > 0;
  }

  ByteStream& file_;

  std::array<uint8_t, kBufferSize> buffer_{};

  // The next byte of buffer_ to hand out, and how many of its bytes came from
  // the stream; the stream is ahead of the reader by the difference.
  size_t cursor_ = 0;
  size_t bytes_in_buffer_ = 0;
};

std::unique_ptr<PixelBuffer> Read_PCX_File(const char* name,
                                           engine::gfx::Palette* palette) {
  const auto file_handle = OpenGameFile(name);
  if (!file_handle) {
    return nullptr;
  }
  // Only the kind of picture the games ship and write is read: version 5,
  // run-length encoded, 8 bits a pixel in one plane.
  engine::gfx::PcxHeader header{};
  if (!file_handle->ReadObject(header) || header.id != 10 ||
      header.version != 5 || header.bits_per_pixel != 8 ||
      header.color_planes != 1 || header.encoding != 1) {
    return nullptr;
  }
  const int width = header.right - header.left + 1;
  const int height = header.bottom - header.top + 1;
  if (width <= 0 || height <= 0 || header.bytes_per_line < width) {
    return nullptr;
  }
  // A PixelBuffer's size in bytes is an int32_t.
  if (base::ToSize(width) * base::ToSize(height) >
      static_cast<size_t>(std::numeric_limits<int32_t>::max())) {
    return nullptr;
  }
  auto pic = std::make_unique<PixelBuffer>(width, height);
  // Every exit returns `pic`, emptied on failure, so the return is elided.
  const bool decoded = [&] {
    const auto pixels = pic->bytes();
    // TODO: the pixels are not kept from running into the palette at the end
    // of the file: a picture that is cut short takes the palette's bytes for
    // pixels and loads as garbage instead of failing.
    BufferedFileReader reader(*file_handle);
    for (int row = 0; row < height; ++row) {
      int column = 0;
      // A scanline holds bytes_per_line pixels, of which the picture shows the
      // first `width`; the rest is padding to decode and drop.
      while (column < header.bytes_per_line) {
        const auto code = reader.ReadByte();
        if (!code) {
          return false;
        }
        // A byte with its two top bits set is a run: its low six bits count
        // the pixels and the next byte is their color. Any other byte is one
        // pixel.
        int count = 1;
        uint8_t color = *code;
        if ((color & 0xc0U) == 0xc0U) {
          count = color & 0x3fU;
          const auto value = reader.ReadByte();
          if (!value || count == 0) {
            return false;
          }
          color = *value;
        }
        // A run ends with its scanline; one that goes on into the next is
        // taken for a damaged file.
        if (count > header.bytes_per_line - column) {
          return false;
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
    if (palette != nullptr) {
      // The palette is the last 768 bytes of the file, 8 bits a component.
      // The reader's buffer has left the stream somewhere past the pixels, so
      // it is found from the end.
      std::array<uint8_t, engine::gfx::Palette::kFileSize> rgb{};
      file_handle->Seek(-engine::gfx::Palette::kFileSize, SeekOrigin::kEnd);
      if (file_handle->Read(std::span(rgb)) !=
          engine::gfx::Palette::kFileSize) {
        return false;
      }
      *palette = engine::gfx::Palette::FromRgb(rgb);
    }
    return true;
  }();
  if (!decoded) {
    pic.reset();
  }
  return pic;
}

}  // namespace ra
