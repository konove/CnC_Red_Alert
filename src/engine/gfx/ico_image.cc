#include "engine/gfx/ico_image.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "base/array.h"
#include "base/buffer.h"
#include "base/numeric.h"
#include "base/types.h"
#include "base/unaligned.h"

namespace engine::gfx {
namespace {

// The icon directory: three little-endian 16-bit words (reserved, 0; type,
// 1 for an icon; the entry count), then one kEntrySize-byte entry per image,
// whose 32-bit word at kEntryOffsetField is the image's offset in the file.
// The entry's own size and depth fields are not trusted: the image's bitmap
// header says the same, and some writers leave them 0.
constexpr base::ssize kDirectorySize = 6;
constexpr base::ssize kEntrySize = 16;
constexpr base::ssize kEntryOffsetField = 12;
constexpr uint16_t kTypeIcon = 1;

// An image is a BITMAPINFOHEADER, its palette of 4-byte B, G, R, unused
// entries, the color bitmap and a 1-bit mask, both bottom-up with rows padded
// to 4 bytes. The header's height counts both bitmaps, so it is twice the
// image's.
constexpr base::ssize kBitmapHeaderSize = 40;
constexpr base::ssize kPaletteEntrySize = 4;
constexpr uint32_t kCompressionNone = 0;

// Icons larger than this are not icons any program draws; it also keeps a
// corrupt header from asking for gigabytes.
constexpr int kMaxSide = 256;

// The fields of an image's bitmap header that decoding needs.
struct BitmapHeader {
  int width = 0;
  int height = 0;
  int bit_count = 0;
  // The bytes from the image's start to its palette.
  base::ssize header_size = 0;
  int palette_size = 0;
};

// Reads the bitmap header of the image at the start of `image`. Returns
// nullopt for an image DecodeImage() cannot read: PNG, compressed or
// true-color data, or a size out of range.
std::optional<BitmapHeader> ReadBitmapHeader(
    const std::span<const std::byte> image) {
  const auto header_size = base::ReadLittleEndian<uint32_t>(image, 0);
  if (header_size < kBitmapHeaderSize) {
    return std::nullopt;  // Also a PNG, whose signature starts 0x89 'P'.
  }
  BitmapHeader header;
  header.header_size = header_size;
  header.width = base::ReadLittleEndian<int32_t>(image, 4);
  header.height = base::ReadLittleEndian<int32_t>(image, 8) / 2;
  header.bit_count = base::ReadLittleEndian<uint16_t>(image, 14);
  const auto compression = base::ReadLittleEndian<uint32_t>(image, 16);
  const auto colors_used = base::ReadLittleEndian<uint32_t>(image, 32);
  if (header.width <= 0 || header.width > kMaxSide || header.height <= 0 ||
      header.height > kMaxSide || compression != kCompressionNone) {
    return std::nullopt;
  }
  if (header.bit_count != 1 && header.bit_count != 4 && header.bit_count != 8) {
    return std::nullopt;
  }
  const auto max_colors = base::Bit<uint32_t>(header.bit_count);
  header.palette_size = static_cast<int>(
      colors_used == 0 || colors_used > max_colors ? max_colors : colors_used);
  return header;
}

// The bytes in one row of a bottom-up bitmap `width` pixels wide at
// `bit_count` bits per pixel: rows are padded to a 32-bit boundary.
base::ssize RowBytes(const int width, const int bit_count) {
  return (((base::ssize{width} * bit_count) + 31) / 32) * 4;
}

// Returns bit-packed pixel `x` of `row`, most significant bits first.
int PixelAt(const std::span<const std::byte> row, const int x,
            const int bit_count) {
  const int pixels_per_byte = 8 / bit_count;
  const auto byte =
      std::to_integer<uint32_t>(base::At(row, x / pixels_per_byte));
  const auto shift =
      static_cast<uint32_t>(8 - (bit_count * (1 + (x % pixels_per_byte))));
  return static_cast<int>((byte >> shift) &
                          (base::Bit<uint32_t>(bit_count) - 1U));
}

// Decodes the image described by `header` at the start of `image`, or returns
// nullopt if its palette or bitmaps run past the end of `image`.
std::optional<IcoImage> DecodeImage(const std::span<const std::byte> image,
                                    const BitmapHeader& header) {
  const auto palette = base::SliceOrEmpty(
      image, header.header_size, header.palette_size * kPaletteEntrySize);
  const base::ssize color_offset =
      header.header_size +
      (base::ssize{header.palette_size} * kPaletteEntrySize);
  const base::ssize color_row_bytes = RowBytes(header.width, header.bit_count);
  const base::ssize mask_row_bytes = RowBytes(header.width, 1);
  const auto color_bits =
      base::SliceOrEmpty(image, color_offset, color_row_bytes * header.height);
  const auto mask_bits = base::SliceOrEmpty(
      image, color_offset + (color_row_bytes * header.height),
      mask_row_bytes * header.height);
  if (palette.empty() || color_bits.empty() || mask_bits.empty()) {
    return std::nullopt;
  }

  IcoImage result{.width = header.width, .height = header.height, .rgba = {}};
  result.rgba.reserve(base::ToSize(header.width * header.height * 4));
  for (int y = 0; y < header.height; ++y) {
    // The bitmaps store the bottom row first.
    const base::ssize row = header.height - 1 - y;
    const auto colors = color_bits.subspan(base::ToSize(row * color_row_bytes),
                                           base::ToSize(color_row_bytes));
    const auto mask = mask_bits.subspan(base::ToSize(row * mask_row_bytes),
                                        base::ToSize(mask_row_bytes));
    for (int x = 0; x < header.width; ++x) {
      const int index = PixelAt(colors, x, header.bit_count);
      if (index >= header.palette_size) {
        return std::nullopt;  // A color past the end of a short palette.
      }
      // A palette entry is B, G, R, unused: 0x00RRGGBB, read little-endian.
      const auto color =
          base::ReadLittleEndian<uint32_t>(palette, index * kPaletteEntrySize);
      const bool transparent = PixelAt(mask, x, 1) != 0;
      result.rgba.push_back(static_cast<uint8_t>(color >> 16U));
      result.rgba.push_back(static_cast<uint8_t>(color >> 8U));
      result.rgba.push_back(static_cast<uint8_t>(color));
      result.rgba.push_back(transparent ? 0 : 255);
    }
  }
  return result;
}

}  // namespace

std::optional<IcoImage> DecodeIco(const std::span<const std::byte> ico_file) {
  if (base::ReadLittleEndian<uint16_t>(ico_file, 0) != 0 ||
      base::ReadLittleEndian<uint16_t>(ico_file, 2) != kTypeIcon) {
    return std::nullopt;
  }
  const int count = base::ReadLittleEndian<uint16_t>(ico_file, 4);
  if (base::ToSigned(ico_file.size()) < kDirectorySize + (count * kEntrySize)) {
    return std::nullopt;
  }

  std::span<const std::byte> best_image;
  std::optional<BitmapHeader> best;
  for (int i = 0; i < count; ++i) {
    const base::ssize entry = kDirectorySize + (i * kEntrySize);
    const auto offset =
        base::ReadLittleEndian<uint32_t>(ico_file, entry + kEntryOffsetField);
    const auto image = base::SliceOrEmpty(ico_file, offset);
    const auto header = ReadBitmapHeader(image);
    if (!header.has_value()) {
      continue;
    }
    if (best.has_value()) {
      const int area = header->width * header->height;
      const int best_area = best->width * best->height;
      if (area < best_area ||
          (area == best_area && header->bit_count <= best->bit_count)) {
        continue;
      }
    }
    best = header;
    best_image = image;
  }
  if (!best.has_value()) {
    return std::nullopt;
  }
  return DecodeImage(best_image, *best);
}

}  // namespace engine::gfx
