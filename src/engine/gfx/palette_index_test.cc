#include "engine/gfx/palette_index.h"

#include <array>
#include <cstddef>
#include <cstdint>

#include "base/array.h"
#include "engine/gfx/palette.h"
#include "engine/gfx/palette_color.h"
#include "engine/stream/archive.h"
#include "engine/stream/span_sink.h"
#include "engine/stream/span_source.h"
#include "gtest/gtest.h"

namespace engine::gfx {
namespace {

TEST(PaletteIndexTest, DefaultIsSlotZero) {
  constexpr PaletteIndex index;
  EXPECT_EQ(index.value(), 0);
  EXPECT_TRUE(index.IsTransparent());
}

TEST(PaletteIndexTest, HoldsTheSlotItWasGiven) {
  constexpr PaletteIndex index{200};
  EXPECT_EQ(index.value(), 200);
  EXPECT_EQ(index.byte(), uint8_t{200});
  EXPECT_FALSE(index.IsTransparent());
}

TEST(PaletteIndexTest, FromIntAcceptsTheWholeRange) {
  EXPECT_EQ(PaletteIndex::FromInt(0), PaletteIndex{0});
  EXPECT_EQ(PaletteIndex::FromInt(255), PaletteIndex{255});
}

TEST(PaletteIndexDeathTest, FromIntRejectsValuesOutsideThePalette) {
  // The switch and formatted failure diagnostic are inside GoogleTest's macro.
  // NOLINTNEXTLINE(clang-diagnostic-switch-default,clang-diagnostic-unsafe-buffer-usage-in-libc-call)
  EXPECT_DEATH(static_cast<void>(PaletteIndex::FromInt(-1)), "palette index");
  // NOLINTNEXTLINE(clang-diagnostic-switch-default,clang-diagnostic-unsafe-buffer-usage-in-libc-call)
  EXPECT_DEATH(static_cast<void>(PaletteIndex::FromInt(256)), "palette index");
}

TEST(PaletteIndexTest, ComparesBySlot) {
  EXPECT_EQ(PaletteIndex{7}, PaletteIndex{7});
  EXPECT_NE(PaletteIndex{7}, PaletteIndex{8});
}

TEST(PaletteIndexTest, RemappedByLooksTheSlotUpInTheTable) {
  std::array<uint8_t, 256> table{};
  base::At(table, 12) = 99;
  EXPECT_EQ(PaletteIndex{12}.RemappedBy(table), PaletteIndex{99});
  EXPECT_EQ(PaletteIndex{13}.RemappedBy(table), PaletteIndex{0});
}

TEST(PaletteIndexTest, IndexesAPalette) {
  Palette palette;
  palette.at(PaletteIndex{15}) = kWhiteColor;
  const Palette& read_only = palette;
  EXPECT_EQ(read_only.at(PaletteIndex{15}), kWhiteColor);
  EXPECT_EQ(read_only.at(15), kWhiteColor);
}

TEST(PaletteIndexTest, PrintsItsSlotNumberInAFailedExpectation) {
  EXPECT_EQ(testing::PrintToString(PaletteIndex{171}), "171");
}

TEST(PaletteIndexTest, IsSavedAsOneByte) {
  // One byte of room: a wider write would fail the sink.
  std::array<std::byte, 1> bytes{};
  SpanSink sink(bytes);
  ArchiveWriter writer(sink);
  PaletteIndex saved{171};
  writer(saved);
  EXPECT_TRUE(writer.ok());
  EXPECT_EQ(bytes.front(), std::byte{171});

  SpanSource source(bytes);
  ArchiveReader reader(source);
  PaletteIndex loaded;
  reader(loaded);
  EXPECT_TRUE(reader.ok());
  EXPECT_EQ(loaded, saved);
}

}  // namespace
}  // namespace engine::gfx
