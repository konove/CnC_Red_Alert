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

#include "engine/gfx/pcx_file.h"

#include <array>
#include <cstdint>
#include <span>
#include <string_view>

#include "base/numeric.h"
#include "engine/file/file_access.h"
#include "engine/file/game_file.h"
#include "engine/gfx/palette.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/stream/byte_stream.h"

// Appends `pixels`, one row of the picture, to `file` as a run-length encoded
// PCX scanline. An empty row writes nothing.
static void Write_Pcx_ScanLine(ByteStream& file,
                               std::span<const uint8_t> pixels);

bool Write_PCX_File(std::string_view name, PixelView& pic,
                    const engine::gfx::Palette& palette) {
  // An 8-bit, single-plane, run-length encoded version 5 picture. The sizes
  // are set below; the device resolution stays 320x200 whatever the picture.
  PCX_HEADER header = {10,  5,   1,  8, 0, 0,   319, 199,
                       320, 200, {}, 0, 1, 320, 1,   {}};

  // A surface's page has pixels only while it is locked, and a view that was
  // never attached has none at all.
  if (!pic.Lock()) {
    return false;
  }
  const auto file = OpenGameFile(name, FileAccess::kWrite);
  if (!file) {
    pic.Unlock();
    return false;
  }

  header.width = static_cast<int16_t>(pic.width() - 1);
  header.height = static_cast<int16_t>(pic.height() - 1);
  header.byte_per_line = static_cast<int16_t>(pic.width());
  file->WriteObject(header);

  const int VP_Scan_Line = pic.stride();
  const auto pixels = pic.pixels();
  for (int i = 0; i < pic.height(); i++) {
    Write_Pcx_ScanLine(*file, pixels.subspan(base::ToSize(i * VP_Scan_Line),
                                             base::ToSize(pic.width())));
  }
  // The 256-color palette closes the file, 8 bits a component, behind a byte
  // of 12 that tells it from pixel data.
  const uint8_t palette_marker = 0x0c;
  file->WriteObject(palette_marker);
  const std::array<uint8_t, engine::gfx::Palette::kFileSize> palcopy =
      palette.ToRgb();
  file->WriteObject(palcopy);
  pic.Unlock();
  return file->ok();
}

// How many encoded bytes are collected before they go to the stream, which is
// not buffered itself.
constexpr int kPoolSize = 2048;
void Write_Pcx_ScanLine(ByteStream& file, std::span<const uint8_t> pixels) {
  std::array<uint8_t, kPoolSize> pool{};

  int used = 0;
  if (pixels.empty()) {
    return;
  }

  // A byte with its top two bits set is a run count (the low six bits, so at
  // most 63) and the byte after it the color; any other byte is one pixel of
  // that color. A lone pixel whose color has both bits set would read as a
  // count, so it goes out as a run of one.
  const auto write_char = [&](uint8_t x) {
    pool.at(base::ToSize(used++)) = x;
    if (used >= kPoolSize) {
      file.Write(std::span(pool));
      used = 0;
    }
  };
  uint8_t last = pixels.front();
  // How many pixels of `last` have been seen and not written yet; 0 just after
  // a full run went out.
  int rle = 1;

  for (const uint8_t color : pixels.subspan(1)) {
    if (color == last) {
      rle++;
      // A count cannot hold more: write the full run (255 is 192 | 63) and
      // start counting afresh.
      if (rle == 63) {
        write_char(255);
        write_char(color);
        rle = 0;
      }
    } else {
      if (rle != 0) {
        if (rle == 1 && (last & 0xC0U) != 0xC0U) {
          write_char(last);
        } else {
          write_char(static_cast<uint8_t>(0xC0U | static_cast<unsigned>(rle)));
          write_char(last);
        }
      }
      last = color;
      rle = 1;
    }
  }
  if (rle != 0) {
    if (rle == 1 && (last & 0xC0U) != 0xC0U) {
      write_char(last);
    } else {
      write_char(static_cast<uint8_t>(0xC0U | static_cast<unsigned>(rle)));
      write_char(last);
    }
  }

  file.Write(std::span(pool).first(base::ToSize(used)));
}
