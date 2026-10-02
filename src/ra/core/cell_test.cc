// Pins Cell against the golden values packed_test.cc used for the free cell
// helpers it replaced: the 7-bit packing and its truncation, the sentinels,
// the 16-bit wrap of Offset() and the conversions to and from Coordinate.

#include "ra/core/cell.h"

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
#include "ra/core/coordinate.h"
#include "ra/core/defines.h"

namespace ra {
namespace {

constexpr Coordinate kSample = Coordinate::FromPacked(0x12345678);

// The cell at X 5, Y 3.
constexpr Cell kCell = Cell::FromXY(5, 3);

// The factories and accessors are usable in constant expressions.
static_assert(kCell.index() == 389);
static_assert(Cell::Containing(kSample) == Cell::FromXY(0x56, 0x12));

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

TEST(CellTest, DefaultIsCellZero) {
  EXPECT_EQ(Cell{}.index(), 0);
  EXPECT_EQ(Cell{}, Cell::FromXY(0, 0));
}

TEST(CellTest, FromXYPacksSevenBitsPerAxis) {
  EXPECT_EQ(kCell.index(), (3 * 128) + 5);
  EXPECT_EQ(kCell.x(), 5);
  EXPECT_EQ(kCell.y(), 3);
  EXPECT_EQ(Cell::FromXY(127, 127).index(), 16383);
}

TEST(CellTest, FromXYTruncatesOutOfRangeComponents) {
  EXPECT_EQ(Cell::FromXY(-1, 0).index(), 127);
  EXPECT_EQ(Cell::FromXY(130, 1).index(), 128 + 2);
  EXPECT_EQ(Cell::FromXY(0, 128).index(), 0);
  EXPECT_EQ(Cell::FromXY(0, -1).index(), 127 * 128);
}

TEST(CellTest, FromIndexKeepsTheLowSixteenBits) {
  EXPECT_EQ(Cell::FromIndex(389), kCell);
  EXPECT_EQ(Cell::FromIndex(0x10000 + 389), kCell);
  EXPECT_EQ(Cell::FromIndex(32768).index(), -32768);
  EXPECT_EQ(Cell::FromIndex(-2).index(), -2);
}

TEST(CellTest, SentinelsReadAsFullComponents) {
  EXPECT_EQ(Cell::None(), Cell::FromIndex(-1));
  EXPECT_EQ(Cell::None().index(), -1);
  EXPECT_EQ(Cell::None().x(), 127);
  EXPECT_EQ(Cell::None().y(), 127);
  EXPECT_EQ(Cell::FromIndex(kRefreshEol).x(), 127);
  EXPECT_EQ(Cell::FromIndex(kRefreshEol).y(), 127);
}

TEST(CellTest, IsNoneForEveryNegativeIndex) {
  EXPECT_TRUE(Cell::None().is_none());
  EXPECT_TRUE(Cell::FromIndex(-2).is_none());
  EXPECT_TRUE(Cell::FromIndex(-32768).is_none());
  EXPECT_FALSE(Cell::FromIndex(0).is_none());
  EXPECT_FALSE(Cell::FromIndex(MAP_CELL_TOTAL).is_none());
  EXPECT_FALSE(Cell::FromIndex(kRefreshEol).is_none());
}

TEST(CellTest, OffsetAddsAndWrapsAtSixteenBits) {
  EXPECT_EQ(kCell.Offset(MAP_CELL_W), Cell::FromXY(5, 4));
  EXPECT_EQ(kCell.Offset(-1), Cell::FromXY(4, 3));
  EXPECT_EQ(Cell::FromIndex(0).Offset(-1), Cell::None());
  EXPECT_EQ(Cell::FromIndex(32767).Offset(1).index(), -32768);
  EXPECT_EQ(Cell::FromIndex(-32768).Offset(-1).index(), 32767);
  // Off the right edge the index runs into the next row, as the old
  // arithmetic did.
  EXPECT_EQ(Cell::FromXY(127, 3).Offset(1), Cell::FromXY(0, 4));
}

TEST(CellTest, OrdersBySignedIndex) {
  EXPECT_LT(Cell::None(), Cell::FromIndex(0));
  EXPECT_LT(Cell::FromIndex(0), kCell);
  EXPECT_LT(Cell::FromXY(127, 2), Cell::FromXY(0, 3));
  EXPECT_LT(Cell::FromIndex(-32768), Cell::FromIndex(32767));
  EXPECT_NE(kCell, Cell::FromXY(5, 4));
}

TEST(CellTest, CenterIsTheMiddleOfTheCell) {
  EXPECT_EQ(kCell.Center().packed(), 0x03800580U);
  EXPECT_EQ(Cell::None().Center().packed(), 0x7F807F80U);
}

TEST(CellTest, ContainingReadsSevenBitsOfEachCellByte) {
  EXPECT_EQ(Cell::Containing(kSample), Cell::FromXY(0x56, 0x12));
  // An X cell byte of 128 is off the map and wraps to column 0.
  EXPECT_EQ(Cell::Containing(Coordinate::FromPacked(0x00008000U)),
            Cell::FromXY(0, 0));
}

TEST(CellTest, ContainingTheCenterIsTheCell) {
  for (int index = 0; index < MAP_CELL_TOTAL; ++index) {
    const Cell cell = Cell::FromIndex(index);
    ASSERT_EQ(Cell::Containing(cell.Center()), cell) << index;
  }
}

TEST(CellTest, SerializesTheWordLittleEndian) {
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
  EXPECT_EQ(bytes, (std::vector<std::byte>{std::byte{0x85}, std::byte{0x01},
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
}  // namespace ra
