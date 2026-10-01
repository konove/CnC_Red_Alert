#include "engine/gfx/ico_image.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <ranges>
#include <span>
#include <vector>

#include "base/numeric.h"
#include "base/types.h"
#include "gtest/gtest.h"

namespace engine::gfx {
namespace {

// Appends the low `size` bytes of `value`, least significant first.
// LLVM 23 mistakes element invalidation for invalidating the vector reference;
// no element reference or iterator is retained across these appends.
// NOLINTNEXTLINE(clang-diagnostic-lifetime-safety-invalidation)
void Put(std::vector<std::byte>& out, uint32_t value, const int size) {
  for (int i = 0; i < size; ++i) {
    out.push_back(static_cast<std::byte>(value & 0xffU));
    value >>= 8U;
  }
}

// Appends `bits` packed most significant first, padded to a 32-bit boundary.
void PutRow(std::vector<std::byte>& out, const std::vector<int>& values,
            const int bits) {
  std::vector<std::byte> row;
  uint32_t byte = 0;
  int used = 0;
  for (const int value : values) {
    byte = (byte << static_cast<uint32_t>(bits)) | static_cast<uint32_t>(value);
    used += bits;
    if (used == 8) {
      row.push_back(static_cast<std::byte>(byte));
      byte = 0;
      used = 0;
    }
  }
  if (used != 0) {
    row.push_back(
        static_cast<std::byte>(byte << static_cast<uint32_t>(8 - used)));
  }
  while (row.size() % 4 != 0) {
    row.push_back(std::byte{0});
  }
  out.insert(out.end(), row.begin(), row.end());
}

// An image to store in an icon file. Rows are listed top row first; the
// builder stores them bottom-up, as the format does.
struct TestImage {
  int width = 0;
  int height = 0;
  int bit_count = 4;
  std::vector<uint32_t> palette;  // 0xRRGGBB
  std::vector<std::vector<int>> pixels;
  std::vector<std::vector<int>> mask;  // 1 is transparent
};

std::vector<std::byte> ImageBytes(const TestImage& image) {
  std::vector<std::byte> out;
  Put(out, 40, 4);
  Put(out, static_cast<uint32_t>(image.width), 4);
  Put(out, static_cast<uint32_t>(image.height * 2), 4);
  Put(out, 1, 2);  // planes
  Put(out, static_cast<uint32_t>(image.bit_count), 2);
  Put(out, 0, 4);  // compression
  Put(out, 0, 4);  // image size
  Put(out, 0, 4);  // x pixels per meter
  Put(out, 0, 4);  // y pixels per meter
  Put(out, static_cast<uint32_t>(image.palette.size()), 4);
  Put(out, 0, 4);  // colors important
  for (const uint32_t color : image.palette) {
    Put(out, color, 4);  // B, G, R, 0 in little-endian order
  }
  for (const auto& row : std::views::reverse(image.pixels)) {
    PutRow(out, row, image.bit_count);
  }
  for (const auto& row : std::views::reverse(image.mask)) {
    PutRow(out, row, 1);
  }
  return out;
}

// Builds an icon file of `images`, each stored as given.
std::vector<std::byte> IcoFile(
    const std::vector<std::vector<std::byte>>& images) {
  std::vector<std::byte> out;
  Put(out, 0, 2);
  Put(out, 1, 2);
  Put(out, static_cast<uint32_t>(images.size()), 2);
  auto offset = static_cast<uint32_t>(6 + (16 * images.size()));
  for (const auto& image : images) {
    Put(out, 0, 4);  // size and color count, which the decoder ignores
    Put(out, 0, 4);  // planes and bit count, likewise
    Put(out, static_cast<uint32_t>(image.size()), 4);
    Put(out, offset, 4);
    offset += static_cast<uint32_t>(image.size());
  }
  for (const auto& image : images) {
    out.insert(out.end(), image.begin(), image.end());
  }
  return out;
}

// A width x height image of one palette color, 0xRRGGBB, with no mask.
TestImage SolidImage(const int width, const int height, const int bit_count,
                     const uint32_t color) {
  return {.width = width,
          .height = height,
          .bit_count = bit_count,
          .palette = {color},
          .pixels = std::vector(base::ToSize(height),
                                std::vector(base::ToSize(width), 0)),
          .mask = std::vector(base::ToSize(height),
                              std::vector(base::ToSize(width), 0))};
}

// The R, G, B, A bytes of pixel (x, y).
std::vector<uint8_t> PixelAt(const IcoImage& image, const int x, const int y) {
  const auto start =
      image.rgba.begin() + (((base::ssize{y} * image.width) + x) * 4);
  return {start, start + 4};
}

TEST(DecodeIcoTest, DecodesPalettedPixelsTopRowFirst) {
  const TestImage image{.width = 3,
                        .height = 2,
                        .bit_count = 4,
                        .palette = {0xff0000, 0x00ff00, 0x0000ff},
                        .pixels = {{0, 1, 2}, {2, 2, 1}},
                        .mask = {{0, 0, 1}, {0, 0, 0}}};
  const auto decoded = DecodeIco(IcoFile({ImageBytes(image)}));
  ASSERT_TRUE(decoded.has_value());
  EXPECT_EQ(decoded->width, 3);
  EXPECT_EQ(decoded->height, 2);
  ASSERT_EQ(decoded->rgba.size(), 3U * 2U * 4U);
  EXPECT_EQ(PixelAt(*decoded, 0, 0), (std::vector<uint8_t>{255, 0, 0, 255}));
  EXPECT_EQ(PixelAt(*decoded, 1, 0), (std::vector<uint8_t>{0, 255, 0, 255}));
  EXPECT_EQ(PixelAt(*decoded, 2, 0), (std::vector<uint8_t>{0, 0, 255, 0}));
  EXPECT_EQ(PixelAt(*decoded, 0, 1), (std::vector<uint8_t>{0, 0, 255, 255}));
  EXPECT_EQ(PixelAt(*decoded, 2, 1), (std::vector<uint8_t>{0, 255, 0, 255}));
}

TEST(DecodeIcoTest, DecodesOneAndEightBitImages) {
  const TestImage one_bit{.width = 9,
                          .height = 1,
                          .bit_count = 1,
                          .palette = {0x000000, 0xffffff},
                          .pixels = {{1, 0, 0, 0, 0, 0, 0, 0, 1}},
                          .mask = {{0, 0, 0, 0, 0, 0, 0, 0, 1}}};
  const auto one = DecodeIco(IcoFile({ImageBytes(one_bit)}));
  ASSERT_TRUE(one.has_value());
  EXPECT_EQ(PixelAt(*one, 0, 0), (std::vector<uint8_t>{255, 255, 255, 255}));
  EXPECT_EQ(PixelAt(*one, 1, 0), (std::vector<uint8_t>{0, 0, 0, 255}));
  EXPECT_EQ(PixelAt(*one, 8, 0), (std::vector<uint8_t>{255, 255, 255, 0}));

  TestImage eight_bit = SolidImage(2, 1, 8, 0);
  eight_bit.palette.resize(256);
  eight_bit.palette.at(200) = 0x123456;
  eight_bit.pixels = {{200, 0}};
  const auto eight = DecodeIco(IcoFile({ImageBytes(eight_bit)}));
  ASSERT_TRUE(eight.has_value());
  EXPECT_EQ(PixelAt(*eight, 0, 0),
            (std::vector<uint8_t>{0x12, 0x34, 0x56, 255}));
}

TEST(DecodeIcoTest, PicksTheLargestThenTheDeepestImage) {
  const auto decoded = DecodeIco(IcoFile({
      ImageBytes(SolidImage(16, 16, 8, 0x000001)),
      ImageBytes(SolidImage(32, 32, 4, 0x000002)),
      ImageBytes(SolidImage(32, 32, 8, 0x000003)),
      ImageBytes(SolidImage(32, 32, 1, 0x000004)),
  }));
  ASSERT_TRUE(decoded.has_value());
  EXPECT_EQ(decoded->width, 32);
  EXPECT_EQ(PixelAt(*decoded, 0, 0), (std::vector<uint8_t>{0, 0, 3, 255}));
}

TEST(DecodeIcoTest, SkipsImagesItCannotRead) {
  std::vector<std::byte> png;
  // The PNG signature: 0x89, "PNG", CR, LF, 0x1a, LF.
  for (const int byte : {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a}) {
    png.push_back(static_cast<std::byte>(byte));
  }
  auto true_color = ImageBytes(SolidImage(64, 64, 4, 0));
  true_color.at(14) = std::byte{32};  // bit count
  const auto readable = ImageBytes(SolidImage(16, 16, 4, 0x000005));

  const auto decoded = DecodeIco(IcoFile({png, true_color, readable}));
  ASSERT_TRUE(decoded.has_value());
  EXPECT_EQ(decoded->width, 16);
  EXPECT_FALSE(DecodeIco(IcoFile({png, true_color})).has_value());
}

TEST(DecodeIcoTest, RejectsMalformedFiles) {
  EXPECT_FALSE(DecodeIco({}).has_value());

  const auto valid = IcoFile({ImageBytes(SolidImage(4, 4, 4, 0))});
  ASSERT_TRUE(DecodeIco(valid).has_value());

  auto cursor = valid;
  cursor.at(2) = std::byte{2};  // type 2 is a cursor file
  EXPECT_FALSE(DecodeIco(cursor).has_value());

  auto more_entries = valid;
  more_entries.at(5) = std::byte{1};  // 257 entries, past the file's end
  EXPECT_FALSE(DecodeIco(more_entries).has_value());

  EXPECT_FALSE(DecodeIco(std::span(valid).first(valid.size() - 1)).has_value());

  TestImage past_palette = SolidImage(4, 4, 4, 0);
  past_palette.pixels.at(0).at(0) = 3;  // the palette has one color
  EXPECT_FALSE(DecodeIco(IcoFile({ImageBytes(past_palette)})).has_value());
}

}  // namespace
}  // namespace engine::gfx
