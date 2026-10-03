#include "engine/gfx/cps_image.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "engine/gfx/palette_color.h"
#include "engine/stream/memory_stream.h"
#include "gtest/gtest.h"

namespace engine::gfx {

namespace {

// An uncompressed picture of `pixels`, whose header claims `claimed` of them,
// with `palette` in its skipped area.
std::vector<std::byte> Picture(const std::vector<uint8_t>& pixels,
                               uint8_t claimed,
                               const std::vector<uint8_t>& palette) {
  const auto skip = static_cast<uint16_t>(palette.size());
  // The stored size counts the 8-byte header, the palette and the data.
  const auto stored = static_cast<uint16_t>(8 + skip + pixels.size());
  std::vector<uint8_t> bytes = {static_cast<uint8_t>(stored & 0xFFU),
                                static_cast<uint8_t>(stored >> 8U),
                                0,  // NOCOMPRESS
                                0,  // pad
                                claimed,
                                0,
                                0,
                                0,  // uncompressed_bytes
                                static_cast<uint8_t>(skip & 0xFFU),
                                static_cast<uint8_t>(skip >> 8U)};
  bytes.insert(bytes.end(), palette.begin(), palette.end());
  bytes.insert(bytes.end(), pixels.begin(), pixels.end());
  std::vector<std::byte> out(bytes.size());
  std::ranges::transform(bytes, out.begin(),
                         [](uint8_t byte) { return std::byte{byte}; });
  return out;
}

TEST(ReadCpsImageTest, SkipsThePalette) {
  const std::vector<std::byte> file = Picture({9, 8, 7}, 3, {63, 63, 63});
  MemoryStream stream(file);

  EXPECT_EQ(ReadCpsImage(stream).pixels, (std::vector<uint8_t>{9, 8, 7}));
}

TEST(ReadCpsImageTest, KeepsOnlyTheColorsAShortPaletteHoldsWhole) {
  const std::vector<std::byte> file =
      Picture({9, 8, 7, 6}, 4, {63, 0, 0, 0, 63, 0, 5});
  MemoryStream stream(file);

  const CpsImage image = ReadCpsImage(stream);

  EXPECT_EQ(image.colors, (std::vector<PaletteColor>{PaletteColor(255, 0, 0),
                                                     PaletteColor(0, 255, 0)}));
  EXPECT_EQ(image.pixels.at(0), 9);
}

TEST(ReadCpsImageTest, APictureWithoutAPaletteHasNoColors) {
  const std::vector<std::byte> file = Picture({9}, 1, {});
  MemoryStream stream(file);

  EXPECT_TRUE(ReadCpsImage(stream).colors.empty());
}

TEST(ReadCpsImageTest, RejectsATruncatedFile) {
  std::vector<std::byte> file = Picture({9, 8, 7}, 3, {});
  file.pop_back();
  MemoryStream stream(file);

  EXPECT_TRUE(ReadCpsImage(stream).pixels.empty());
}

TEST(ReadCpsImageTest, RejectsMoreDataThanTheBlockHolds) {
  const std::vector<std::byte> file = Picture({9, 8, 7}, 4, {});
  MemoryStream stream(file);

  EXPECT_TRUE(ReadCpsImage(stream).pixels.empty());
}

}  // namespace

}  // namespace engine::gfx
