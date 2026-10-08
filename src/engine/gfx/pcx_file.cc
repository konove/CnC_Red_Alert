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

#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

#include "base/numeric.h"
#include "engine/file/file_access.h"
#include "engine/file/game_file.h"
#include "engine/gfx/palette.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/stream/byte_stream.h"

namespace engine::gfx {

namespace {

// A byte of the pixel data from this value up is a run count, in the six bits
// under its two top ones, and the byte after it the color; any lower byte is
// one pixel of that color.
constexpr int kRunMarker = 0xC0;
constexpr int kMaxRunLength = 63;

// Appends `pixels`, one row of the picture, to `encoded` as a run-length
// encoded PCX scanline.
void AppendScanline(std::span<const uint8_t> pixels,
                    std::vector<uint8_t>& encoded) {
  if (pixels.empty()) {
    return;
  }
  uint8_t run_color = pixels.front();
  int run_length = 0;
  const auto append_run = [&] {
    // A lone pixel of a color that would read as a count goes out as a run of
    // one.
    if (run_length > 1 || run_color >= kRunMarker) {
      encoded.push_back(static_cast<uint8_t>(kRunMarker + run_length));
    }
    encoded.push_back(run_color);
  };
  for (const uint8_t color : pixels) {
    if (color != run_color || run_length == kMaxRunLength) {
      append_run();
      run_color = color;
      run_length = 0;
    }
    ++run_length;
  }
  append_run();
}

// Returns the scanlines of the picture in `view`, which the caller has locked.
std::vector<uint8_t> EncodeScanlines(PixelView& view) {
  std::vector<uint8_t> encoded;
  const int stride = view.stride();
  const auto pixels = view.pixels();
  for (int row = 0; row < view.height(); row++) {
    AppendScanline(
        pixels.subspan(base::ToSize(row * stride), base::ToSize(view.width())),
        encoded);
  }
  return encoded;
}

}  // namespace

bool WritePcxFile(std::string_view file_name, PixelView& view,
                  const Palette& palette) {
  // A surface's page has pixels only while it is locked, and a view that was
  // never attached has none at all.
  if (!view.Lock()) {
    return false;
  }
  const std::vector<uint8_t> scanlines = EncodeScanlines(view);
  view.Unlock();

  const auto file = OpenGameFile(file_name, FileAccess::kWrite);
  if (!file) {
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
  file->Write(std::span(scanlines));
  // The 256-color palette closes the file, 8 bits a component, behind a byte
  // of 12 that tells it from pixel data.
  const uint8_t palette_marker = 12;
  file->WriteObject(palette_marker);
  file->WriteObject(palette.ToRgb());
  return file->ok();
}

}  // namespace engine::gfx
