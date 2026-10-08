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

#include <algorithm>
#include <array>
#include <cstdint>
#include <iterator>
#include <optional>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include "base/numeric.h"
#include "base/types.h"
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

// The header of the one kind of picture written and read here: version 5, the
// first that can end in a 256-color palette, run-length encoded, 8 bits a
// pixel in one plane.
constexpr uint8_t kPcxId = 10;
constexpr uint8_t kVersion = 5;
constexpr uint8_t kRunLengthEncoding = 1;
constexpr uint8_t kBitsPerPixel = 8;
constexpr uint8_t kColorPlanes = 1;

// The 256-color palette closes the picture, 8 bits a component, behind a byte
// of 12 that tells it from pixel data.
constexpr uint8_t kPaletteMarker = 12;
constexpr base::ssize kTrailerSize = 1 + Palette::kFileSize;

// Appends `pixels`, one row of the picture, to `encoded` as a run-length
// encoded PCX scanline.
void AppendScanline(const std::span<const uint8_t> pixels,
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

// Returns the scanlines of the picture in `view`, or nothing for a view without
// pixels: a surface's page has them only while it is locked, and a view that
// was never attached has none at all.
std::optional<std::vector<uint8_t>> LockedScanlines(PixelView& view) {
  if (!view.Lock()) {
    return std::nullopt;
  }
  std::vector<uint8_t> scanlines = EncodeScanlines(view);
  view.Unlock();
  return scanlines;
}

// Writes the picture in `view`, whose encoded rows are `scanlines`, to `out`.
// Returns whether `out` took all of it.
bool WriteEncoded(ByteStream& out, const PixelView& view,
                  const std::span<const uint8_t> scanlines,
                  const Palette& palette) {
  // The device resolution is 320x200 whatever the picture's size.
  const PcxHeader header = {
      .id = kPcxId,
      .version = kVersion,
      .encoding = kRunLengthEncoding,
      .bits_per_pixel = kBitsPerPixel,
      .left = 0,
      .top = 0,
      .right = static_cast<int16_t>(view.width() - 1),
      .bottom = static_cast<int16_t>(view.height() - 1),
      .horizontal_dpi = 320,
      .vertical_dpi = 200,
      .ega_palette = {},
      .reserved = 0,
      .color_planes = kColorPlanes,
      .bytes_per_line = static_cast<int16_t>(view.width()),
      .palette_type = 1,
      .padding = {},
  };
  return out.WriteObject(header) &&
         out.Write(scanlines) == std::ssize(scanlines) &&
         out.WriteObject(kPaletteMarker) && out.WriteObject(palette.ToRgb());
}

// Returns the pixels of a `width` by `height` picture whose scanlines are
// `encoded`, each decoding to `bytes_per_line` pixels, or nothing if `encoded`
// ends before the picture does or holds a run no writer makes.
std::optional<std::vector<uint8_t>> DecodeScanlines(
    const std::span<const uint8_t> encoded, const int width, const int height,
    const int bytes_per_line) {
  // The pixels are appended as they are decoded, so that a header claiming a
  // huge picture costs no more memory than its scanlines decode to.
  std::vector<uint8_t> pixels;
  auto next = encoded.begin();
  for (int row = 0; row < height; ++row) {
    // A scanline holds bytes_per_line pixels, of which the picture shows the
    // first `width`; the rest is padding to decode and drop.
    int column = 0;
    while (column < bytes_per_line) {
      if (next == encoded.end()) {
        return std::nullopt;
      }
      uint8_t color = *next++;
      int run_length = 1;
      if (color >= kRunMarker) {
        run_length = color - kRunMarker;
        if (run_length == 0 || next == encoded.end()) {
          return std::nullopt;
        }
        color = *next++;
      }
      // A run ends with its scanline; one that goes on into the next is
      // taken for a damaged file.
      if (run_length > bytes_per_line - column) {
        return std::nullopt;
      }
      const int visible = std::min(run_length, std::max(0, width - column));
      pixels.insert(pixels.end(), base::ToSize(visible), color);
      column += run_length;
    }
  }
  return pixels;
}

}  // namespace

bool WritePcx(ByteStream& out, PixelView& view, const Palette& palette) {
  const std::optional<std::vector<uint8_t>> scanlines = LockedScanlines(view);
  return scanlines && WriteEncoded(out, view, *scanlines, palette);
}

bool WritePcxFile(const std::string_view file_name, PixelView& view,
                  const Palette& palette) {
  // Encoding comes first, so that a view without pixels creates no file.
  const std::optional<std::vector<uint8_t>> scanlines = LockedScanlines(view);
  if (!scanlines) {
    return false;
  }
  const auto file = OpenGameFile(file_name, FileAccess::kWrite);
  return file && WriteEncoded(*file, view, *scanlines, palette) && file->ok();
}

std::optional<PcxPicture> ReadPcx(ByteStream& in) {
  PcxHeader header{};
  if (!in.ReadObject(header) || header.id != kPcxId ||
      header.version != kVersion || header.encoding != kRunLengthEncoding ||
      header.bits_per_pixel != kBitsPerPixel ||
      header.color_planes != kColorPlanes) {
    return std::nullopt;
  }
  const int width = header.right - header.left + 1;
  const int height = header.bottom - header.top + 1;
  if (width <= 0 || height <= 0 || header.bytes_per_line < width) {
    return std::nullopt;
  }
  // Everything between the header and the palette with its marker is
  // scanlines. They are read whole, which keeps the decoder out of the
  // palette when the picture is cut short.
  const base::ssize encoded_size = in.Size() - in.Tell() - kTrailerSize;
  if (encoded_size < 0) {
    return std::nullopt;
  }
  std::vector<uint8_t> encoded(base::ToSize(encoded_size));
  uint8_t marker = 0;
  std::array<uint8_t, Palette::kFileSize> rgb{};
  if (in.Read(std::span(encoded)) != encoded_size || !in.ReadObject(marker) ||
      marker != kPaletteMarker ||
      in.Read(std::span(rgb)) != Palette::kFileSize) {
    return std::nullopt;
  }
  std::optional<std::vector<uint8_t>> pixels =
      DecodeScanlines(encoded, width, height, header.bytes_per_line);
  if (!pixels) {
    return std::nullopt;
  }
  return PcxPicture{
      .width = width,
      .height = height,
      .pixels = std::move(*pixels),
      .palette = Palette::FromRgb(rgb),
  };
}

std::optional<PcxPicture> ReadPcxFile(const std::string_view file_name) {
  const auto file = OpenGameFile(file_name);
  if (!file) {
    return std::nullopt;
  }
  return ReadPcx(*file);
}

}  // namespace engine::gfx
