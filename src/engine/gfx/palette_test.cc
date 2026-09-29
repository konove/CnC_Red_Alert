#include "engine/gfx/palette.h"

#include <array>
#include <cstddef>
#include <cstdint>

#include "engine/gfx/palette_color.h"
#include "gtest/gtest.h"

namespace engine::gfx {
namespace {

// A VGA palette whose byte i is i % 64, so every gun differs from its
// neighbors and stays in the VGA's 0..63.
std::array<uint8_t, Palette::kVgaSize> CountingVga() {
  std::array<uint8_t, Palette::kVgaSize> vga{};
  for (int i = 0; i < Palette::kVgaSize; ++i) {
    vga.at(static_cast<size_t>(i)) = static_cast<uint8_t>(i % 64);
  }
  return vga;
}

TEST(PaletteTest, StartsBlack) {
  const Palette palette;
  for (const PaletteColor& color : palette.colors()) {
    EXPECT_EQ(color, kBlackColor);
  }
}

TEST(PaletteTest, FromVgaReadsRgbTriples) {
  const Palette palette = Palette::FromVga(CountingVga());
  EXPECT_EQ(palette.at(0), PaletteColor::FromGuns(0, 1, 2));
  EXPECT_EQ(palette.at(1), PaletteColor::FromGuns(3, 4, 5));
  EXPECT_EQ(palette.at(255), PaletteColor::FromGuns(61, 62, 63));
}

TEST(PaletteTest, ToVgaWritesWhatFromVgaRead) {
  const std::array<uint8_t, Palette::kVgaSize> vga = CountingVga();
  EXPECT_EQ(Palette::FromVga(vga).ToVga(), vga);
}

TEST(PaletteTest, FilledPaletteHoldsOneColor) {
  const PaletteColor grey = PaletteColor::FromGuns(20, 20, 20);
  const Palette palette(grey);
  for (const PaletteColor& color : palette.colors()) {
    EXPECT_EQ(color, grey);
  }
}

TEST(PaletteTest, EqualityComparesEveryEntry) {
  const Palette a;
  Palette b;
  EXPECT_EQ(a, b);
  b.at(200) = PaletteColor::FromGuns(1, 0, 0);
  EXPECT_NE(a, b);
}

TEST(PaletteDeathTest, AtRejectsIndicesOutsideThePalette) {
  const Palette palette;
  // GoogleTest's death-test macro formats subprocess diagnostics with libc.
  // NOLINTBEGIN(clang-diagnostic-switch-default,clang-diagnostic-unsafe-buffer-usage-in-libc-call)
  EXPECT_DEATH(static_cast<void>(palette.at(256)), "Check failed");
  EXPECT_DEATH(static_cast<void>(palette.at(-1)), "Check failed");
  // NOLINTEND(clang-diagnostic-switch-default,clang-diagnostic-unsafe-buffer-usage-in-libc-call)
}

}  // namespace
}  // namespace engine::gfx
