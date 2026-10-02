// Pins Cell against the golden values coord_inline_test.cc used for the free
// cell helpers it replaced: the unmasked 6-bit packing, the unmasked Y read of
// a sentinel, the 16-bit wrap of Offset() and the conversions to and from
// Coordinate, including Coord_Cell's whole-byte OR of the X cell.

#include "td/core/cell.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <type_traits>
#include <vector>

#include "engine/stream/archive.h"
#include "engine/stream/span_source.h"
#include "engine/stream/vector_sink.h"
#include "gtest/gtest.h"
#include "td/core/coordinate.h"
#include "td/core/defines.h"

namespace td {
namespace {

constexpr Coordinate kSample = Coordinate::FromPacked(0x12345678);

// The cell at X 5, Y 3.
constexpr Cell kCell = Cell::FromXY(5, 3);

// The factories and accessors are usable in constant expressions.
static_assert(kCell.index() == 197);
static_assert(Cell::Containing(kSample) == Cell::FromIndex(0x04D6));

// EventClass carries cells in a union and sends them as raw bytes, so a cell
// is the 16-bit word and nothing else.
static_assert(sizeof(Cell) == 2);
static_assert(std::is_trivially_copyable_v<Cell>);
static_assert(std::is_trivially_default_constructible_v<Cell>);
static_assert(std::is_standard_layout_v<Cell>);

// Components and indices are plain ints at the interface.
static_assert(std::is_same_v<decltype(kCell.index()), int>);
static_assert(std::is_same_v<decltype(kCell.x()), int>);
static_assert(std::is_same_v<decltype(kCell.y()), int>);

// A cell never converts to or from an integer implicitly.
static_assert(!std::is_convertible_v<Cell, int>);
static_assert(!std::is_convertible_v<Cell, bool>);
static_assert(!std::is_convertible_v<int, Cell>);
static_assert(!std::is_convertible_v<int16_t, Cell>);

TEST(TdCellTest, DefaultIsCellZero) {
  EXPECT_EQ(Cell{}.index(), 0);
  EXPECT_EQ(Cell{}, Cell::FromXY(0, 0));
}

TEST(TdCellTest, FromXYPacksSixBitsPerAxis) {
  EXPECT_EQ(kCell.index(), (3 * 64) + 5);
  EXPECT_EQ(kCell.x(), 5);
  EXPECT_EQ(kCell.y(), 3);
  EXPECT_EQ(Cell::FromXY(63, 63).index(), 4095);
}

// Unlike Red Alert's, the components are not masked: they are ORed together
// and the word keeps its low 16 bits.
TEST(TdCellTest, FromXYDoesNotMaskTheComponents) {
  // An X of 64 or more sets Y bits.
  EXPECT_EQ(Cell::FromXY(64, 0), Cell::FromXY(0, 1));
  EXPECT_EQ(Cell::FromXY(64, 0).index(), 64);
  EXPECT_EQ(Cell::FromXY(65, 1).index(), 65);
  // A negative X sets every high bit.
  EXPECT_EQ(Cell::FromXY(-1, 0), Cell::None());
  EXPECT_EQ(Cell::FromXY(-1, 5).index(), -1);
  EXPECT_EQ(Cell::FromXY(0, -1).index(), -64);
  // A Y of 1024 or more leaves the 16-bit word.
  EXPECT_EQ(Cell::FromXY(3, 1024).index(), 3);
  EXPECT_EQ(Cell::FromXY(0, 512).index(), -32768);
}

TEST(TdCellTest, FromIndexKeepsTheLowSixteenBits) {
  EXPECT_EQ(Cell::FromIndex(197), kCell);
  EXPECT_EQ(Cell::FromIndex(0x10000 + 197), kCell);
  EXPECT_EQ(Cell::FromIndex(32768).index(), -32768);
  EXPECT_EQ(Cell::FromIndex(-2).index(), -2);
}

// X keeps 6 bits, but Y is the whole word above them, read after sign
// extension: a negative index has a Y of millions.
TEST(TdCellTest, SentinelsReadUnmaskedY) {
  EXPECT_EQ(Cell::None(), Cell::FromIndex(-1));
  EXPECT_EQ(Cell::None().index(), -1);
  EXPECT_EQ(Cell::None().x(), 63);
  EXPECT_EQ(Cell::None().y(), 67108863);
  EXPECT_EQ(Cell::FromIndex(-2).y(), 67108863);
  EXPECT_EQ(Cell::FromIndex(-32768).x(), 0);
  EXPECT_EQ(Cell::FromIndex(-32768).y(), 67108352);
  EXPECT_EQ(Cell::FromIndex(REFRESH_EOL).x(), 63);
  EXPECT_EQ(Cell::FromIndex(REFRESH_EOL).y(), 511);
  // Off the bottom of the map, still positive.
  EXPECT_EQ(Cell::FromIndex(MAP_CELL_TOTAL).x(), 0);
  EXPECT_EQ(Cell::FromIndex(MAP_CELL_TOTAL).y(), 64);
}

TEST(TdCellTest, IsNoneForEveryNegativeIndex) {
  EXPECT_TRUE(Cell::None().is_none());
  EXPECT_TRUE(Cell::FromIndex(-2).is_none());
  EXPECT_TRUE(Cell::FromIndex(-32768).is_none());
  EXPECT_FALSE(Cell::FromIndex(0).is_none());
  EXPECT_FALSE(Cell::FromIndex(MAP_CELL_TOTAL).is_none());
  EXPECT_FALSE(Cell::FromIndex(REFRESH_EOL).is_none());
}

TEST(TdCellTest, OffsetAddsAndWrapsAtSixteenBits) {
  EXPECT_EQ(kCell.Offset(MAP_CELL_W), Cell::FromXY(5, 4));
  EXPECT_EQ(kCell.Offset(-1), Cell::FromXY(4, 3));
  EXPECT_EQ(Cell::FromIndex(0).Offset(-1), Cell::None());
  EXPECT_EQ(Cell::FromIndex(32767).Offset(1).index(), -32768);
  EXPECT_EQ(Cell::FromIndex(-32768).Offset(-1).index(), 32767);
  // Off the right edge the index runs into the next row, as the old
  // arithmetic did.
  EXPECT_EQ(Cell::FromXY(63, 3).Offset(1), Cell::FromXY(0, 4));
}

TEST(TdCellTest, OrdersBySignedIndex) {
  EXPECT_LT(Cell::None(), Cell::FromIndex(0));
  EXPECT_LT(Cell::FromIndex(0), kCell);
  EXPECT_LT(Cell::FromXY(63, 2), Cell::FromXY(0, 3));
  EXPECT_LT(Cell::FromIndex(-32768), Cell::FromIndex(32767));
  EXPECT_NE(kCell, Cell::FromXY(5, 4));
}

TEST(TdCellTest, CenterIsTheMiddleOfTheCell) {
  EXPECT_EQ(kCell.Center().packed(), 0x03800580U);
  EXPECT_EQ(Cell{}.Center().packed(), 0x00800080U);
  EXPECT_EQ(Cell::FromXY(63, 63).Center().packed(), 0x3F803F80U);
  // None()'s Y of 67108863 keeps the low 8 bits of its cell byte.
  EXPECT_EQ(Cell::None().Center().packed(), 0xFF803F80U);
}

// Containing() keeps the X cell's whole byte: an X cell of 64 or more sets
// bits that belong to the Y cell, as the original assembly's OR did.
TEST(TdCellTest, ContainingOrsTheWholeXCellByte) {
  EXPECT_EQ(Cell::Containing(Coordinate::FromPacked(0x03800580U)), kCell);
  EXPECT_EQ(Cell::Containing(kSample).index(), 0x04D6);
  EXPECT_EQ(Cell::Containing(Coordinate::FromPacked(0x01007F00U)).index(),
            0x007F);
  EXPECT_EQ(Cell::Containing(Coordinate::FromPacked(0x0000FF00U)).index(),
            0x00FF);
  EXPECT_EQ(Cell::Containing(Coordinate::FromPacked(0xFFFFFFFFU)).index(),
            0x3FFF);
}

TEST(TdCellTest, ContainingTheCenterIsTheCell) {
  for (int index = 0; index < MAP_CELL_TOTAL; ++index) {
    const Cell cell = Cell::FromIndex(index);
    ASSERT_EQ(Cell::Containing(cell.Center()), cell) << index;
  }
}

TEST(TdCellTest, SerializesTheWordLittleEndian) {
  std::vector<std::byte> bytes;
  VectorSink sink(bytes);
  // The call operator is non-const; misc-const-correctness misses the call
  // because its argument is template-dependent.
  // NOLINTNEXTLINE(misc-const-correctness)
  ArchiveWriter writer(sink);
  Cell written = kCell;
  Cell none = Cell::None();
  writer(written, none);
  ASSERT_TRUE(writer.ok());
  EXPECT_EQ(bytes, (std::vector<std::byte>{std::byte{0xC5}, std::byte{0x00},
                                           std::byte{0xFF}, std::byte{0xFF}}));

  SpanSource source{std::span<const std::byte>(bytes)};
  // NOLINTNEXTLINE(misc-const-correctness)
  ArchiveReader reader(source);
  Cell read{};
  Cell read_none{};
  reader(read, read_none);
  EXPECT_EQ(std::string(reader.error()), "");
  EXPECT_EQ(read, kCell);
  EXPECT_EQ(read_none, Cell::None());
}

}  // namespace
}  // namespace td
