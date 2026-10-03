// Tests jshell's picture reader and the fading and translucency tables it
// builds.

#include "ra/core/jshell.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "base/array.h"
#include "engine/gfx/fading_table.h"
#include "engine/gfx/palette_color.h"
#include "engine/stream/memory_stream.h"
#include "gtest/gtest.h"
#include "ra/assets/palette.h"

namespace ra {

namespace {

using engine::gfx::PaletteColor;

// An uncompressed picture of `pixels` whose header claims `claimed` of them,
// with a two-byte palette ahead of the data.
std::vector<std::byte> Picture(const std::vector<uint8_t>& pixels,
                               uint8_t claimed) {
  // The stored size counts the 8-byte header, the palette and the data.
  const auto stored = static_cast<uint8_t>(8 + 2 + pixels.size());
  std::vector<uint8_t> bytes = {stored,  0,
                                0,                 // NOCOMPRESS
                                0,                 // pad
                                claimed, 0, 0, 0,  // uncompressed_bytes
                                2,       0,        // skip_bytes
                                63,      63};      // palette
  bytes.insert(bytes.end(), pixels.begin(), pixels.end());
  std::vector<std::byte> out(bytes.size());
  std::ranges::transform(bytes, out.begin(),
                         [](uint8_t byte) { return std::byte{byte}; });
  return out;
}

TEST(ReadPictureTest, SkipsThePalette) {
  const std::vector<std::byte> file = Picture({9, 8, 7}, 3);
  MemoryStream stream(file);

  EXPECT_EQ(ReadPicture(stream), (std::vector<uint8_t>{9, 8, 7}));
}

TEST(ReadPictureTest, RejectsATruncatedFile) {
  std::vector<std::byte> file = Picture({9, 8, 7}, 3);
  file.pop_back();
  MemoryStream stream(file);

  EXPECT_TRUE(ReadPicture(stream).empty());
}

TEST(ReadPictureTest, RejectsMoreDataThanTheBlockHolds) {
  const std::vector<std::byte> file = Picture({9, 8, 7}, 4);
  MemoryStream stream(file);

  EXPECT_TRUE(ReadPicture(stream).empty());
}

TEST(MakeTranslucencyTableTest, LooksUpEachSourceColorsFade) {
  constexpr auto kControl =
      std::to_array<TLucentType>({{5, 40, 0, 0}, {9, 70, 0, 0}});
  const TranslucencyTable<2> table =
      MakeTranslucencyTable(kControl, [](const TLucentType& entry) {
        FadingTable fade{};
        fade.fill(entry.DestColor);
        return fade;
      });
  const std::span<const uint8_t> view(table);

  EXPECT_EQ(base::At(view, 5), 0);
  EXPECT_EQ(base::At(view, 9), 1);
  EXPECT_EQ(base::At(view, 6), 255);
  EXPECT_EQ(base::At(view, 256), 40);
  EXPECT_EQ(base::At(view, 511), 40);
  EXPECT_EQ(base::At(view, 512), 70);
  EXPECT_EQ(base::At(view, 767), 70);
}

TEST(BuildTranslucentTableTest, FadesMatchTheEngineFadingTable) {
  PaletteClass palette(PaletteColor(30, 30, 30));
  palette.at(7) = PaletteColor(200, 10, 10);
  palette.at(8) = PaletteColor(110, 20, 20);
  constexpr auto kControl = std::to_array<TLucentType>({{3, 7, 128, 0}});

  const TranslucencyTable<1> table = BuildTranslucentTable(palette, kControl);

  const FadingTable expected = BuildFadingTable(palette, 7, 128);
  EXPECT_TRUE(std::ranges::equal(std::span(table).subspan(256), expected));
}

TEST(ConquerBuildFadingTableTest, StaysInsideTheShadowRange) {
  // Color 255 matches the target exactly, so only its exclusion keeps it out.
  PaletteClass palette(PaletteColor(200, 200, 200));
  palette.at(255) = PaletteColor(0, 0, 0);
  palette.at(250) = PaletteColor(20, 20, 20);

  const FadingTable table = ConquerBuildFadingTable(palette, 255, 255);

  EXPECT_EQ(table.at(0), 0);
  EXPECT_EQ(table.at(1), 250);
  EXPECT_EQ(table.at(240), 250);
  for (int index = 241; index < 256; index++) {
    EXPECT_EQ(base::At(table, index), index);
  }
}

}  // namespace

}  // namespace ra
