#include "td/support.h"

#include <array>
#include <cstdint>
#include <span>

#include "base/array.h"
#include "gtest/gtest.h"

namespace {

using Palette = std::array<uint8_t, 768>;

void SetColor(std::span<uint8_t> palette, int index, uint8_t red, uint8_t green,
              uint8_t blue) {
  base::At(palette, (index * 3) + 0) = red;
  base::At(palette, (index * 3) + 1) = green;
  base::At(palette, (index * 3) + 2) = blue;
}

TEST(ConquerBuildFadingTableTest, FullFadeReachesTargetWithDistinctGuns) {
  // Only the last 16 colors are candidates. Every gun of the target (240)
  // differs, and the other candidates sit where a builder that mixes up the
  // guns would aim.
  Palette palette{};
  for (int index = 241; index < 256; index++) {
    SetColor(palette, index, 10, 10, 10);
  }
  SetColor(palette, 240, 10, 20, 50);
  SetColor(palette, 1, 50, 20, 10);
  std::array<uint8_t, 256> table{};

  Conquer_Build_Fading_Table(palette, table, 240, 255);

  EXPECT_EQ(table.at(0), 0);
  EXPECT_EQ(table.at(1), 240);
}

TEST(ConquerBuildFadingTableTest, CandidateColorsMapToThemselves) {
  Palette palette{};
  palette.fill(30);
  std::array<uint8_t, 256> table{};

  Conquer_Build_Fading_Table(palette, table, 245, 200);

  for (int index = 240; index < 256; index++) {
    EXPECT_EQ(base::At(std::span(table), index), index);
  }
}

}  // namespace
