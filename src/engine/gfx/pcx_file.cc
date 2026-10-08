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

namespace {

// How many encoded bytes are collected before they go to the stream, which is
// not buffered itself.
constexpr int kEncodeBufferBytes = 2048;

// The two top bits that make a byte of the pixel data a run count, and the
// longest run the six bits under them can count.
constexpr unsigned kRunMarker = 0xC0;
constexpr int kMaxRunLength = 63;

// Appends `pixels`, one row of the picture, to `file` as a run-length encoded
// PCX scanline. An empty row writes nothing.
void EncodeScanline(ByteStream& file, std::span<const uint8_t> pixels) {
  if (pixels.empty()) {
    return;
  }
  std::array<uint8_t, kEncodeBufferBytes> encoded{};
  int encoded_count = 0;
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
  // A byte with its top two bits set is a run count and the byte after it the
  // color; any other byte is one pixel of that color. A lone pixel whose color
  // has both bits set would read as a count, so it goes out as a run of one.
  const auto append_run = [&] {
    if (run_length == 0) {
      return;
    }
    if (run_length > 1 || (run_color & kRunMarker) == kRunMarker) {
      append(
          static_cast<uint8_t>(kRunMarker | static_cast<unsigned>(run_length)));
    }
    append(run_color);
  };

  for (const uint8_t color : pixels.subspan(1)) {
    if (color != run_color) {
      append_run();
      run_color = color;
      run_length = 1;
    } else if (++run_length == kMaxRunLength) {
      // A count cannot hold more: write the run and start counting afresh.
      append_run();
      run_length = 0;
    }
  }
  append_run();

  file.Write(std::span(encoded).first(base::ToSize(encoded_count)));
}

}  // namespace

bool WritePcxFile(std::string_view file_name, PixelView& view,
                  const engine::gfx::Palette& palette) {
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

  // An 8-bit, single-plane, run-length encoded version 5 picture. The device
  // resolution is 320x200 whatever the picture's size.
  const PcxHeader header = {
      .id = 10,
      .version = 5,
      .encoding = 1,
      .bits_per_pixel = 8,
      .left = 0,
      .top = 0,
      .right = static_cast<int16_t>(view.width() - 1),
      .bottom = static_cast<int16_t>(view.height() - 1),
      .horizontal_dpi = 320,
      .vertical_dpi = 200,
      .ega_palette = {},
      .reserved = 0,
      .color_planes = 1,
      .bytes_per_line = static_cast<int16_t>(view.width()),
      .palette_type = 1,
      .padding = {},
  };
  file->WriteObject(header);

  const int stride = view.stride();
  const auto pixels = view.pixels();
  for (int row = 0; row < view.height(); row++) {
    EncodeScanline(*file, pixels.subspan(base::ToSize(row * stride),
                                         base::ToSize(view.width())));
  }
  // The 256-color palette closes the file, 8 bits a component, behind a byte
  // of 12 that tells it from pixel data.
  const uint8_t palette_marker = 12;
  file->WriteObject(palette_marker);
  file->WriteObject(palette.ToRgb());
  view.Unlock();
  return file->ok();
}
