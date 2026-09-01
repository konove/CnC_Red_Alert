#include "ra/dib.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <optional>
#include <span>

#include "base/types.h"

namespace dib {
namespace {

// A BMP begins with a 14-byte file header and a 40-byte info header, both
// little-endian and neither of them aligned. They are read field by field
// rather than mapped onto a packed struct, so the code does not depend on the
// host's alignment rules or byte order.
constexpr base::ssize kFileHeaderSize = 14;
constexpr base::ssize kInfoHeaderSize = 40;

constexpr std::uint16_t kBitmapSignature = 0x4D42;  // "BM", little-endian.
constexpr std::uint16_t kUncompressed = 0;
constexpr int kPalettisedBitCount = 8;
constexpr base::ssize kMaxColors = kPaletteSize;

std::uint16_t ReadU16(std::span<const std::uint8_t> data, base::ssize offset) {
  return static_cast<std::uint16_t>(
      static_cast<unsigned>(data[static_cast<std::size_t>(offset)]) |
      (static_cast<unsigned>(data[static_cast<std::size_t>(offset) + 1])
       << 8U));
}

std::uint32_t ReadU32(std::span<const std::uint8_t> data, base::ssize offset) {
  std::uint32_t value = 0;
  for (base::ssize i = 3; i >= 0; --i) {
    value <<= 8U;
    value |= data[static_cast<std::size_t>(offset + i)];
  }
  return value;
}

}  // namespace

std::optional<Image> Image::FromBmp(std::span<const std::uint8_t> bmp) {
  if (std::ssize(bmp) < kFileHeaderSize + kInfoHeaderSize) {
    return std::nullopt;
  }
  if (ReadU16(bmp, 0) != kBitmapSignature) {
    return std::nullopt;
  }

  const auto info = bmp.subspan(static_cast<std::size_t>(kFileHeaderSize));
  const std::int32_t width = static_cast<std::int32_t>(ReadU32(info, 4));
  const std::int32_t height = static_cast<std::int32_t>(ReadU32(info, 8));
  const std::uint16_t bit_count = ReadU16(info, 14);
  const std::uint32_t compression = ReadU32(info, 16);

  // Only what the game actually receives. A negative height means a top-down
  // bitmap, which nothing here produces and which would silently flip the art.
  if (width <= 0 || height <= 0 || bit_count != kPalettisedBitCount ||
      compression != kUncompressed) {
    return std::nullopt;
  }

  // biClrUsed of 0 means the table is full for the bit depth.
  const std::uint32_t declared_colors = ReadU32(info, 32);
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
  std::memcpy(image.colors_.data(),
              bmp.data() + static_cast<std::size_t>(color_table_start),
              static_cast<std::size_t>(color_table_bytes));

  // bfOffBits says where the pixels start. Trust it only if it lands inside
  // the data and past the headers; some writers leave it at zero.
  const auto declared_offset = static_cast<base::ssize>(ReadU32(bmp, 10));
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
  std::memcpy(image.bits_.data(),
              bmp.data() + static_cast<std::size_t>(bits_start),
              static_cast<std::size_t>(needed));

  return image;
}

void RemapToPalette(Image& image, std::span<const Color> target) {
  if (target.empty()) {
    return;
  }

  // The nearest entry in `target` to one of the image's own colours, by the
  // sum of the per-channel differences. That is the metric the game's own
  // PaletteClass::Closest_Color uses, and it runs in the palette's 6-bit
  // space, so the image's 8-bit channels are shifted down to meet it.
  const auto nearest = [target](const Color& color) {
    const int red = color.red >> 2U;
    const int green = color.green >> 2U;
    const int blue = color.blue >> 2U;

    base::ssize best = 0;
    int best_difference = std::numeric_limits<int>::max();
    for (base::ssize i = 0; i < std::ssize(target); ++i) {
      const int difference = std::abs(red - target[i].red) +
                             std::abs(green - target[i].green) +
                             std::abs(blue - target[i].blue);
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
  mapping[0] = nearest(Color{});
  for (base::ssize i = 1; i < std::ssize(image.Colors()); ++i) {
    mapping[static_cast<std::size_t>(i)] = nearest(image.Colors()[i]);
  }

  for (std::uint8_t& pixel : image.MutableBits()) {
    pixel = mapping[pixel];
  }
}

}  // namespace dib
