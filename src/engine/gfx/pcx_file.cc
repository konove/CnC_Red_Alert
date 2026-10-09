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
#include <limits>
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

// The 128 bytes a PCX file starts with, laid out as they are on disk: the
// 16-bit fields are little-endian and all fall on even offsets, so the struct
// has no padding and is read and written whole.
struct PcxHeader {
  uint8_t id;        // 10 marks a PCX file.
  uint8_t version;   // 5 is the first that can end in a 256-color palette.
  uint8_t encoding;  // 1 means run-length encoded, the only encoding there is.
  uint8_t bits_per_pixel;  // Bits a pixel in each color plane.
  // The picture's left and top edge.
  int16_t left;
  int16_t top;
  // The picture's right and bottom edge, inclusive: for a picture at 0,0 they
  // are its width and height minus one.
  int16_t right;
  int16_t bottom;
  // The resolution of the device the picture was made for. Readers ignore it.
  int16_t horizontal_dpi;
  int16_t vertical_dpi;
  // Sixteen colors of three bytes each, unused by an 8-bit picture.
  std::array<uint8_t, 48> ega_palette;
  uint8_t reserved;  // Reserved, zero.
  uint8_t color_planes;
  // Bytes in one decoded scanline of one plane, which may be more than the
  // picture is wide.
  int16_t bytes_per_line;
  int16_t palette_type;             // 1 is color, 2 grayscale.
  std::array<uint8_t, 58> padding;  // Pads the header to 128 bytes.
};
static_assert(sizeof(PcxHeader) == 128);

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

// Returns the bytes a scanline of a picture `width` pixels wide decodes to.
// The format wants an even number, so a row of odd width is one byte longer
// than the picture is wide.
int BytesPerLine(const int width) { return width + (width % 2); }

// Appends `pixels`, one row of the picture, to `encoded` as a run-length
// encoded PCX scanline, padded to BytesPerLine().
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
  const auto append_pixel = [&](const uint8_t color) {
    if (color != run_color || run_length == kMaxRunLength) {
      append_run();
      run_color = color;
      run_length = 0;
    }
    ++run_length;
  };
  for (const uint8_t color : pixels) {
    append_pixel(color);
  }
  // The padding repeats the row's last pixel, which as a rule only makes the
  // last run one longer.
  if (std::ssize(pixels) % 2 != 0) {
    append_pixel(pixels.back());
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

// Returns whether the header's 16-bit fields hold the size of `view`: its
// width padded to an even number, in bytes_per_line, and its height less one,
// in bottom.
bool FitsHeader(const PixelView& view) {
  constexpr int kMaxField = std::numeric_limits<int16_t>::max();
  return BytesPerLine(view.width()) <= kMaxField &&
         view.height() - 1 <= kMaxField;
}

// Returns the scanlines of the picture in `view`, or nothing for a view too
// large for a header or without pixels: a surface's page has them only while
// it is locked, and a view that was never attached has none at all.
std::optional<std::vector<uint8_t>> LockedScanlines(PixelView& view) {
  if (!FitsHeader(view) || !view.Lock()) {
    return std::nullopt;
  }
  std::vector<uint8_t> scanlines = EncodeScanlines(view);
  view.Unlock();
  return scanlines;
}

// Writes the picture in `view`, whose encoded rows are `scanlines`, to `out`.
// Returns whether `out` took all of it. The caller has checked FitsHeader().
bool WriteEncoded(ByteStream& out, const PixelView& view,
                  const std::span<const uint8_t> scanlines,
                  const Palette& palette) {
  // The resolution fields state 320x200 whatever the picture's size. No reader
  // uses them, and the screenshot hashes of tools/ui_check.py cover them, so
  // the value stays the one the files have always had.
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
      .bytes_per_line = static_cast<int16_t>(BytesPerLine(view.width())),
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
  // Encoding comes first, so that a view that cannot be written creates no
  // file.
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

bool DrawPcxFile(const std::string_view file_name, PixelView& view,
                 Palette& palette) {
  const std::optional<PcxPicture> picture = ReadPcxFile(file_name);
  if (!picture) {
    return false;
  }
  view.CopyFromBuffer(0, 0, picture->width, picture->height, picture->pixels);
  palette = picture->palette;
  return true;
}

}  // namespace engine::gfx
