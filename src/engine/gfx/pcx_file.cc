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
static void EncodeScanline(ByteStream& file, std::span<const uint8_t> pixels);

bool WritePcxFile(std::string_view file_name, PixelView& view,
                  const engine::gfx::Palette& palette) {
  // An 8-bit, single-plane, run-length encoded version 5 picture. The sizes
  // are set below; the device resolution stays 320x200 whatever the picture.
  PcxHeader header = {10,  5,   1,  8, 0, 0,   319, 199,
                      320, 200, {}, 0, 1, 320, 1,   {}};

  // A surface's page has pixels only while it is locked, and a view that was
  // never attached has none at all.
  if (!view.Lock()) {
    return false;
  }
  const auto file = OpenGameFile(file_name, FileAccess::kWrite);
  if (!file) {
    view.Unlock();
    return false;
  }

  header.right = static_cast<int16_t>(view.width() - 1);
  header.bottom = static_cast<int16_t>(view.height() - 1);
  header.bytes_per_line = static_cast<int16_t>(view.width());
  file->WriteObject(header);

  const int stride = view.stride();
  const auto pixels = view.pixels();
  for (int row = 0; row < view.height(); row++) {
    EncodeScanline(*file, pixels.subspan(base::ToSize(row * stride),
                                         base::ToSize(view.width())));
  }
  // The 256-color palette closes the file, 8 bits a component, behind a byte
  // of 12 that tells it from pixel data.
  const uint8_t palette_marker = 0x0c;
  file->WriteObject(palette_marker);
  const std::array<uint8_t, engine::gfx::Palette::kFileSize> rgb =
      palette.ToRgb();
  file->WriteObject(rgb);
  view.Unlock();
  return file->ok();
}

// How many encoded bytes are collected before they go to the stream, which is
// not buffered itself.
constexpr int kEncodeBufferBytes = 2048;
void EncodeScanline(ByteStream& file, std::span<const uint8_t> pixels) {
  std::array<uint8_t, kEncodeBufferBytes> encoded{};

  int encoded_count = 0;
  if (pixels.empty()) {
    return;
  }

  // A byte with its top two bits set is a run count (the low six bits, so at
  // most 63) and the byte after it the color; any other byte is one pixel of
  // that color. A lone pixel whose color has both bits set would read as a
  // count, so it goes out as a run of one.
  const auto append = [&](uint8_t byte) {
    encoded.at(base::ToSize(encoded_count++)) = byte;
    if (encoded_count >= kEncodeBufferBytes) {
      file.Write(std::span(encoded));
      encoded_count = 0;
    }
  };
  uint8_t run_color = pixels.front();
  // How many pixels of `run_color` have been seen and not written yet; 0 just
  // after a full run went out.
  int run_length = 1;

  for (const uint8_t color : pixels.subspan(1)) {
    if (color == run_color) {
      run_length++;
      // A count cannot hold more: write the full run (255 is 192 | 63) and
      // start counting afresh.
      if (run_length == 63) {
        append(255);
        append(color);
        run_length = 0;
      }
    } else {
      if (run_length != 0) {
        if (run_length == 1 && (run_color & 0xC0U) != 0xC0U) {
          append(run_color);
        } else {
          append(
              static_cast<uint8_t>(0xC0U | static_cast<unsigned>(run_length)));
          append(run_color);
        }
      }
      run_color = color;
      run_length = 1;
    }
  }
  if (run_length != 0) {
    if (run_length == 1 && (run_color & 0xC0U) != 0xC0U) {
      append(run_color);
    } else {
      append(static_cast<uint8_t>(0xC0U | static_cast<unsigned>(run_length)));
      append(run_color);
    }
  }

  file.Write(std::span(encoded).first(base::ToSize(encoded_count)));
}
