// Tests Load_Uncompress()'s handling of the palette a picture reserves.

#include "td/core/jshell.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "engine/gfx/palette.h"
#include "engine/gfx/palette_color.h"
#include "engine/stream/memory_stream.h"
#include "gtest/gtest.h"

namespace {

// An uncompressed four-pixel picture reserving `reserved` ahead of its data.
std::vector<std::byte> Picture(const std::vector<uint8_t>& reserved) {
  const std::vector<uint8_t> pixels = {9, 8, 7, 6};
  const auto skip = static_cast<uint16_t>(reserved.size());
  // The stored size counts the 8-byte header, the reserved bytes and the data.
  const auto stored = static_cast<uint16_t>(8 + skip + pixels.size());
  std::vector<uint8_t> bytes = {
      static_cast<uint8_t>(stored & 0xFFU), static_cast<uint8_t>(stored >> 8U),
      0,  // NOCOMPRESS
      0,  // pad
      4, 0, 0, 0,  // uncompressed_bytes
      static_cast<uint8_t>(skip & 0xFFU), static_cast<uint8_t>(skip >> 8U)};
  bytes.insert(bytes.end(), reserved.begin(), reserved.end());
  bytes.insert(bytes.end(), pixels.begin(), pixels.end());
  std::vector<std::byte> out(bytes.size());
  for (std::size_t i = 0; i < bytes.size(); ++i) {
    out.at(i) = static_cast<std::byte>(bytes.at(i));
  }
  return out;
}

TEST(LoadUncompressTest, AShortPaletteLeavesTheOtherColorsAlone) {
  const std::vector<std::byte> file = Picture({63, 0, 0, 0, 63, 0});
  MemoryStream stream(file);
  std::vector<uint8_t> scratch(64);
  std::vector<uint8_t> pixels(64);
  // Off the 6-bit grid, so a round trip through the file format shows.
  engine::gfx::Palette palette(engine::gfx::PaletteColor(1, 2, 3));

  EXPECT_EQ(Load_Uncompress(stream, scratch, pixels, &palette), 4);

  EXPECT_EQ(palette.at(0), engine::gfx::PaletteColor(255, 0, 0));
  EXPECT_EQ(palette.at(1), engine::gfx::PaletteColor(0, 255, 0));
  EXPECT_EQ(palette.at(2), engine::gfx::PaletteColor(1, 2, 3));
  EXPECT_EQ(pixels.at(0), 9);
}

}  // namespace
