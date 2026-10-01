#include "ra/assets/dib.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <optional>
#include <span>

#include "base/array.h"
#include "base/buffer.h"
#include "base/numeric.h"
#include "base/types.h"
#include "base/unaligned.h"

namespace ra::dib {
namespace {

// A BMP begins with a 14-byte file header and a 40-byte info header, both
// little-endian and neither of them aligned. They are read field by field
// with base::ReadLittleEndian() rather than mapped onto a packed struct, so the
// code does not depend on the host's alignment rules or byte order.
constexpr base::ssize kFileHeaderSize = 14;
constexpr base::ssize kInfoHeaderSize = 40;

constexpr std::uint16_t kBitmapSignature = 0x4D42;  // "BM", little-endian.
constexpr std::uint16_t kUncompressed = 0;
constexpr int kPalettisedBitCount = 8;
constexpr base::ssize kMaxColors = kPaletteSize;

}  // namespace

std::optional<Image> Image::FromBmp(std::span<const std::uint8_t> bmp) {
  if (std::ssize(bmp) < kFileHeaderSize + kInfoHeaderSize) {
    return std::nullopt;
  }
  const auto bytes = std::as_bytes(bmp);
  if (base::ReadLittleEndian<std::uint16_t>(bytes, 0) != kBitmapSignature) {
    return std::nullopt;
  }

  const auto info = bytes.subspan(static_cast<std::size_t>(kFileHeaderSize));
  const auto width =
      static_cast<std::int32_t>(base::ReadLittleEndian<std::uint32_t>(info, 4));
  const auto height =
      static_cast<std::int32_t>(base::ReadLittleEndian<std::uint32_t>(info, 8));
  const auto bit_count = base::ReadLittleEndian<std::uint16_t>(info, 14);
  const auto compression = base::ReadLittleEndian<std::uint32_t>(info, 16);

  // Only what the game actually receives. A negative height means a top-down
  // bitmap, which nothing here produces and which would silently flip the art.
  if (width <= 0 || height <= 0 || bit_count != kPalettisedBitCount ||
      compression != kUncompressed) {
    return std::nullopt;
  }

  // biClrUsed of 0 means the table is full for the bit depth.
  const auto declared_colors = base::ReadLittleEndian<std::uint32_t>(info, 32);
  const base::ssize color_count =
      declared_colors == 0 ? kMaxColors
                           : static_cast<base::ssize>(declared_colors);
  if (color_count > kMaxColors) {
    return std::nullopt;
  }

  const base::ssize color_table_start = kFileHeaderSize + kInfoHeaderSize;
  const base::ssize color_table_bytes =
      color_count * static_cast<base::ssize>(sizeof(Color));
  if (std::ssize(bmp) < color_table_start + color_table_bytes) {
    return std::nullopt;
  }

  Image image;
  image.width_ = width;
  image.height_ = height;

  image.colors_.resize(static_cast<std::size_t>(color_count));
  base::CopyBytes(std::as_writable_bytes(std::span(image.colors_)),
                  bytes.subspan(static_cast<size_t>(color_table_start)),
                  color_table_bytes);

  // bfOffBits says where the pixels start. Trust it only if it lands inside
  // the data and past the headers; some writers leave it at zero.
  const auto declared_offset = static_cast<base::ssize>(
      base::ReadLittleEndian<std::uint32_t>(bytes, 10));
  const base::ssize bits_start =
      declared_offset >= color_table_start + color_table_bytes &&
              declared_offset < std::ssize(bmp)
          ? declared_offset
          : color_table_start + color_table_bytes;

  const base::ssize needed = image.Stride() * height;
  if (std::ssize(bmp) - bits_start < needed) {
    return std::nullopt;
  }

  image.bits_.resize(static_cast<std::size_t>(needed));
  base::CopyBytes(std::as_writable_bytes(std::span(image.bits_)),
                  bytes.subspan(static_cast<size_t>(bits_start)), needed);

  return image;
}

void RemapToPalette(Image& image, std::span<const Color> target) {
  if (target.empty()) {
    return;
  }

  // The nearest entry in `target` to one of the image's own colours, by the
  // sum of the per-channel differences, the metric the game's own
  // PaletteClass::Closest_Color uses.
  const auto nearest = [target](const Color& color) {
    const int red = color.red;
    const int green = color.green;
    const int blue = color.blue;

    base::ssize best = 0;
    int best_difference = std::numeric_limits<int>::max();
    for (base::ssize i = 0; i < std::ssize(target); ++i) {
      const auto& entry = base::At(target, base::ToSize(i));
      const int difference = std::abs(red - entry.red) +
                             std::abs(green - entry.green) +
                             std::abs(blue - entry.blue);
      if (difference == 0) {
        return static_cast<std::uint8_t>(i);
      }
      if (difference < best_difference) {
        best = i;
        best_difference = difference;
      }
    }
    return static_cast<std::uint8_t>(best);
  };

  std::array<std::uint8_t, kMaxColors> mapping{};
  mapping.at(0) = nearest(Color{});
  for (base::ssize i = 1; i < std::ssize(image.Colors()); ++i) {
    mapping.at(base::ToSize(i)) =
        nearest(base::At(image.Colors(), base::ToSize(i)));
  }

  for (std::uint8_t& pixel : image.MutableBits()) {
    pixel = mapping.at(pixel);
  }
}

}  // namespace ra::dib
