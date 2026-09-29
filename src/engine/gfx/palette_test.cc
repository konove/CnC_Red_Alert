#include "engine/gfx/palette.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

#include "engine/gfx/palette_color.h"
#include "engine/stream/memory_stream.h"
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

TEST(PaletteTest, FromVgaExpandsRgbTriples) {
  const Palette palette = Palette::FromVga(CountingVga());
  EXPECT_EQ(palette.at(0), PaletteColor(0, 4, 8));
  EXPECT_EQ(palette.at(1), PaletteColor(12, 16, 20));
  EXPECT_EQ(palette.at(255), PaletteColor(247, 251, 255));
}

TEST(PaletteTest, ToVgaKeepsTheTopSixBits) {
  Palette palette;
  palette.at(0) = PaletteColor(255, 130, 3);
  const std::array<uint8_t, Palette::kVgaSize> vga = palette.ToVga();
  EXPECT_EQ(vga.at(0), 63);
  EXPECT_EQ(vga.at(1), 32);
  EXPECT_EQ(vga.at(2), 0);
}

TEST(PaletteTest, ToVgaWritesWhatFromVgaRead) {
  const std::array<uint8_t, Palette::kVgaSize> vga = CountingVga();
  EXPECT_EQ(Palette::FromVga(vga).ToVga(), vga);
}

TEST(PaletteTest, FromVgaIgnoresTheTopTwoBits) {
  std::array<uint8_t, Palette::kVgaSize> vga{};
  vga.at(0) = 0xFF;
  EXPECT_EQ(Palette::FromVga(vga).at(0), PaletteColor(255, 0, 0));
}

TEST(PaletteTest, RgbRoundTripsEveryComponent) {
  std::array<uint8_t, Palette::kVgaSize> rgb{};
  for (int i = 0; i < Palette::kVgaSize; ++i) {
    rgb.at(static_cast<size_t>(i)) = static_cast<uint8_t>(i * 7);
  }
  const Palette palette = Palette::FromRgb(rgb);
  EXPECT_EQ(palette.at(1), PaletteColor(21, 28, 35));
  EXPECT_EQ(palette.ToRgb(), rgb);
}

TEST(PaletteTest, FilledPaletteHoldsOneColor) {
  const PaletteColor grey(80, 80, 80);
  const Palette palette(grey);
  for (const PaletteColor& color : palette.colors()) {
    EXPECT_EQ(color, grey);
  }
}

TEST(PaletteTest, EqualityComparesEveryEntry) {
  const Palette a;
  Palette b;
  EXPECT_EQ(a, b);
  b.at(200) = PaletteColor(1, 0, 0);
  EXPECT_NE(a, b);
}

TEST(BlendedPaletteTest, StartsAtFromAndEndsAtTo) {
  const Palette from(PaletteColor(0, 100, 255));
  const Palette to(PaletteColor(255, 0, 5));
  EXPECT_EQ(BlendedPalette(from, to, 0, 40), from);
  EXPECT_EQ(BlendedPalette(from, to, 40, 40), to);
}

TEST(BlendedPaletteTest, StepsEachComponentLinearly) {
  const Palette from(PaletteColor(0, 100, 255));
  const Palette to(PaletteColor(255, 0, 5));
  // A quarter of the way, each component rounding toward `from`.
  EXPECT_EQ(BlendedPalette(from, to, 1, 4).at(9), PaletteColor(63, 75, 193));
}

TEST(BlendedPaletteTest, TakesEveryEightBitStep) {
  const Palette from;
  const Palette to(PaletteColor(255, 255, 255));
  int steps = 0;
  int last = -1;
  for (int step = 0; step <= 1000; ++step) {
    const int red = BlendedPalette(from, to, step, 1000).at(0).red();
    if (red != last) {
      ++steps;
      last = red;
    }
  }
  EXPECT_EQ(steps, 256);
}

TEST(ReadVgaPaletteTest, ReadsTheFileFormat) {
  const std::array<uint8_t, Palette::kVgaSize> vga = CountingVga();
  MemoryStream stream(std::as_bytes(std::span(vga)));
  const std::optional<Palette> palette = ReadVgaPalette(stream);
  ASSERT_TRUE(palette.has_value());
  EXPECT_EQ(*palette, Palette::FromVga(vga));
}

TEST(ReadVgaPaletteTest, RejectsAShortFile) {
  const std::array<uint8_t, Palette::kVgaSize - 1> vga{};
  MemoryStream stream(std::as_bytes(std::span(vga)));
  EXPECT_FALSE(ReadVgaPalette(stream).has_value());
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
