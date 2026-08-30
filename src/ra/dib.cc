#include "ra/dib.h"

#include <cstdint>
#include <cstring>
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
constexpr base::ssize kMaxColors = 256;

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

}  // namespace dib
