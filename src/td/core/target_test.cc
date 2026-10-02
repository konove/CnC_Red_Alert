// Pins how Tiberian Dawn's Target packs its kind and value into the 16-bit
// word that saves and network events carry, including Make()'s unmasked
// value, which spills into the kind bits.

#include "td/core/target.h"

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
#include "td/core/cell.h"
#include "td/core/coordinate.h"

namespace td {
namespace {

// The cell at X 5, Y 3.
constexpr Cell kCell = Cell::FromXY(5, 3);

// The factories and accessors are usable in constant expressions.
static_assert(Target::Make(KIND_NONE, 0) == Target{});
static_assert(Target::Make(KIND_UNIT, 5).value() == 5);
static_assert(Target::FromCellIndex(kCell).kind() == KIND_CELL);

// EventClass carries targets in a union and sends them as raw bytes, so a
// target is the 16-bit word and nothing else.
static_assert(sizeof(Target) == 2);
static_assert(std::is_trivially_copyable_v<Target>);
static_assert(std::is_trivially_default_constructible_v<Target>);
static_assert(std::is_standard_layout_v<Target>);
static_assert(std::is_same_v<decltype(Target{}.packed()), uint16_t>);

// A target converts to bool only explicitly, and never to or from an integer.
static_assert(!std::is_convertible_v<Target, bool>);
static_assert(!std::is_convertible_v<Target, uint16_t>);
static_assert(!std::is_convertible_v<uint16_t, Target>);

TEST(TargetTest, DefaultIsTheNoneWord) {
  EXPECT_EQ(Target{}.packed(), 0);
  EXPECT_EQ(Target{}.kind(), KIND_NONE);
  EXPECT_EQ(Target{}.value(), 0);
  EXPECT_FALSE(static_cast<bool>(Target{}));
}

TEST(TargetTest, MakePacksKindOverValue) {
  const Target target = Target::Make(KIND_BUILDING, 389);
  EXPECT_EQ(target.packed(), 0x4185);
  EXPECT_EQ(target.kind(), KIND_BUILDING);
  EXPECT_EQ(target.value(), 389);
  EXPECT_TRUE(static_cast<bool>(target));
  EXPECT_EQ(Target::Make(KIND_TEAMTYPE, 0xFFF).packed(), 0xCFFF);
}

// Make() ORs the whole value over the kind, as the original packer did: a value
// of 4096 or more, or a negative one, changes the kind.
TEST(TargetTest, MakeDoesNotMaskTheValue) {
  const Target overflow = Target::Make(KIND_UNIT, 4096);
  EXPECT_EQ(overflow.packed(), 0x3000);
  EXPECT_EQ(overflow.kind(), KIND_INFANTRY);
  EXPECT_EQ(overflow.value(), 0);

  EXPECT_EQ(Target::Make(KIND_BUILDING, 0x1234).packed(), 0x5234);
  EXPECT_EQ(Target::Make(KIND_BUILDING, 0x1234).kind(), KIND_TERRAIN);

  const Target negative = Target::Make(KIND_UNIT, -1);
  EXPECT_EQ(negative.packed(), 0xFFFF);
  EXPECT_EQ(negative.value(), 0xFFF);
  EXPECT_TRUE(static_cast<bool>(negative));

  // Only the low 16 bits of the result survive.
  EXPECT_EQ(Target::Make(KIND_UNIT, 0x10005).packed(), 0x2005);
}

TEST(TargetTest, FromPackedKeepsTheWord) {
  const Target target = Target::FromPacked(0x4123);
  EXPECT_EQ(target.packed(), 0x4123);
  EXPECT_EQ(target.kind(), KIND_BUILDING);
  EXPECT_EQ(target.value(), 0x123);
  EXPECT_EQ(target, Target::Make(KIND_BUILDING, 0x123));
}

// value() keeps the low 12 bits; kind() is the rest of the word shifted down.
TEST(TargetTest, KindAndValueSplitAnyWord) {
  EXPECT_EQ(Target::FromPacked(0x0FFF).kind(), KIND_NONE);
  EXPECT_EQ(Target::FromPacked(0x0FFF).value(), 0xFFF);
  EXPECT_TRUE(static_cast<bool>(Target::FromPacked(0x0FFF)));
  EXPECT_EQ(Target::FromPacked(0xC001).kind(), KIND_TEAMTYPE);
  EXPECT_EQ(Target::FromPacked(0xC001).value(), 1);
  EXPECT_EQ(Target::FromPacked(0xF00A).value(), 0xA);
}

TEST(TargetTest, EqualityComparesTheWord) {
  EXPECT_EQ(Target::Make(KIND_UNIT, 12), Target::Make(KIND_UNIT, 12));
  EXPECT_NE(Target::Make(KIND_UNIT, 12), Target::Make(KIND_UNIT, 13));
  EXPECT_NE(Target::Make(KIND_UNIT, 12), Target::Make(KIND_INFANTRY, 12));
  EXPECT_EQ(Target::Make(KIND_UNIT, 4096), Target::Make(KIND_INFANTRY, 0));
}

// A cell target's value is the cell index itself.
TEST(TargetTest, FromCellIndexHoldsTheCellIndex) {
  const Target target = Target::FromCellIndex(kCell);
  EXPECT_EQ(target, Target::Make(KIND_CELL, 197));
  EXPECT_EQ(target.packed(), 0x10C5);
  EXPECT_EQ(target.value(), kCell.index());
  EXPECT_EQ(target.AsCoordinate(), kCell.Center());
  EXPECT_EQ(target.AsCoordinate().packed(), 0x03800580U);
  EXPECT_EQ(target.AsMovementCoordinate(), kCell.Center());
  EXPECT_EQ(target.AsCell(), kCell);
}

// The cell is not masked either: cell -1 fills the whole word.
TEST(TargetTest, FromCellIndexOfANegativeCellFillsTheWord) {
  EXPECT_EQ(Target::FromCellIndex(Cell::None()).packed(), 0xFFFF);
}

TEST(TargetTest, NoTargetDecodesToNothing) {
  EXPECT_EQ(Target{}.AsCoordinate(), Coordinate{});
  EXPECT_EQ(Target{}.AsMovementCoordinate(), Coordinate{});
  EXPECT_EQ(Target{}.AsCell(), Cell::Containing(Coordinate{}));
  EXPECT_EQ(Target{}.AsObject(), nullptr);
  EXPECT_EQ(Target{}.AsTechno(), nullptr);
  EXPECT_EQ(Target{}.AsAircraft(), nullptr);
  EXPECT_EQ(Target{}.AsBuilding(), nullptr);
  EXPECT_EQ(Target{}.AsUnit(), nullptr);
}

TEST(TargetTest, SerializesTheWordLittleEndian) {
  std::vector<std::byte> bytes;
  VectorSink sink(bytes);
  // The call operator is non-const; misc-const-correctness misses the call
  // because its argument is template-dependent.
  // NOLINTNEXTLINE(misc-const-correctness)
  ArchiveWriter writer(sink);
  Target written = Target::Make(KIND_UNIT, 0x123);
  writer(written);
  ASSERT_TRUE(writer.ok());
  EXPECT_EQ(bytes, (std::vector<std::byte>{std::byte{0x23}, std::byte{0x21}}));

  SpanSource source{std::span<const std::byte>(bytes)};
  // NOLINTNEXTLINE(misc-const-correctness)
  ArchiveReader reader(source);
  Target read{};
  reader(read);
  EXPECT_EQ(std::string(reader.error()), "");
  EXPECT_EQ(read, written);
}

}  // namespace
}  // namespace td
