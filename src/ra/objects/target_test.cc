// Pins how Target packs its kind and value, which saves, network events and
// the multiplayer CRC carry as the raw word, and both encodings of a cell.

#include "ra/core/target.h"

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
#include "ra/core/packed.h"
#include "ra/objects/abstract.h"
#include "ra/objects/type.h"

namespace ra {
namespace {

// The cell at X 5, Y 3.
constexpr CELL kCell = (3 * 128) + 5;

// The factories and accessors are usable in constant expressions.
static_assert(Target::Make(RTTI_NONE, 0) == Target{});
static_assert(Target::Make(RTTI_UNIT, 5).value() == 5);
static_assert(Target::FromCell(kCell).kind() == RTTI_CELL);

// EventClass carries targets in a union and sends them as raw bytes, so a
// target is the 32-bit word and nothing else.
static_assert(sizeof(Target) == 4);
static_assert(std::is_trivially_copyable_v<Target>);
static_assert(std::is_trivially_default_constructible_v<Target>);
static_assert(std::is_standard_layout_v<Target>);

// A target converts to bool only explicitly, and never to or from an integer.
static_assert(!std::is_convertible_v<Target, bool>);
static_assert(!std::is_convertible_v<Target, int32_t>);
static_assert(!std::is_convertible_v<int32_t, Target>);

TEST(TargetTest, DefaultIsTheNoneWord) {
  EXPECT_EQ(Target{}.packed(), 0);
  EXPECT_EQ(Target{}.kind(), RTTI_NONE);
  EXPECT_EQ(Target{}.value(), 0);
  EXPECT_FALSE(static_cast<bool>(Target{}));
  EXPECT_FALSE(Target{}.is_valid());
}

TEST(TargetTest, MakePacksKindOverValue) {
  const Target target = Target::Make(RTTI_CELL, 389);
  EXPECT_EQ(static_cast<uint32_t>(target.packed()),
            (static_cast<uint32_t>(RTTI_CELL) << 24) | 389U);
  EXPECT_EQ(target.kind(), RTTI_CELL);
  EXPECT_EQ(target.value(), 389);
  EXPECT_TRUE(static_cast<bool>(target));
  EXPECT_TRUE(target.is_valid());
}

TEST(TargetTest, MakeKeepsTwentyFourValueBits) {
  const Target target = Target::Make(RTTI_UNIT, -1);
  EXPECT_EQ(target.kind(), RTTI_UNIT);
  EXPECT_EQ(target.value(), 0xFFFFFF);
  EXPECT_EQ(target.packed(), 0x1CFFFFFF);
  EXPECT_EQ(Target::Make(RTTI_INFANTRY, 0x1234567).value(), 0x234567);
}

TEST(TargetTest, FromPackedKeepsTheWord) {
  const Target target = Target::FromPacked(0x05123456);
  EXPECT_EQ(target.packed(), 0x05123456);
  EXPECT_EQ(target.kind(), RTTI_BUILDING);
  EXPECT_EQ(target.value(), 0x123456);
  EXPECT_EQ(target, Target::Make(RTTI_BUILDING, 0x123456));
}

// What a default-constructed target used to be: no kind, every value bit set.
// It is not Target{}, so it reads true as a bool while is_valid() is false.
TEST(TargetTest, NoneWithAllValueBitsIsTrueButNotValid) {
  const Target target = Target::NoneWithAllValueBits();
  EXPECT_EQ(static_cast<uint32_t>(target.packed()), 0x00FFFFFFU);
  EXPECT_EQ(target.kind(), RTTI_NONE);
  EXPECT_TRUE(static_cast<bool>(target));
  EXPECT_FALSE(target.is_valid());
  EXPECT_NE(target, Target{});
}

TEST(TargetTest, EqualityComparesTheWord) {
  EXPECT_EQ(Target::Make(RTTI_BUILDING, 0x123456),
            Target::Make(RTTI_BUILDING, 0x123456));
  EXPECT_NE(Target::Make(RTTI_BUILDING, 0x123456),
            Target::Make(RTTI_BUILDING, 0x123457));
  EXPECT_NE(Target::Make(RTTI_BUILDING, 1), Target::Make(RTTI_UNIT, 1));
}

TEST(TargetTest, IsObjectCoversTheMapObjectKinds) {
  for (const RTTIType kind : {RTTI_TERRAIN, RTTI_UNIT, RTTI_VESSEL,
                              RTTI_INFANTRY, RTTI_BUILDING, RTTI_AIRCRAFT}) {
    EXPECT_TRUE(Target::Make(kind, 0).is_object());
  }
  for (const RTTIType kind :
       {RTTI_NONE, RTTI_CELL, RTTI_BULLET, RTTI_ANIM, RTTI_TEAM, RTTI_TRIGGER,
        RTTI_UNITTYPE, RTTI_OVERLAY, RTTI_SMUDGE}) {
    EXPECT_FALSE(Target::Make(kind, 0).is_object());
  }
}

// The cell index encoding: the value is the CELL itself, sign and all.
TEST(TargetTest, FromCellIndexHoldsTheCellIndex) {
  const Target target = Target::FromCellIndex(kCell);
  EXPECT_EQ(target, Target::Make(RTTI_CELL, 389));
  EXPECT_EQ(target.cell_index(), kCell);
  EXPECT_EQ(target.AsMovementCoordinate(), Cell_Coord(kCell));
  EXPECT_EQ(target.AsMovementCoordinate().packed(), 0x03800580U);
}

TEST(TargetTest, FromCellIndexKeepsTwentyFourBitsOfANegativeCell) {
  const Target target = Target::FromCellIndex(static_cast<CELL>(-1));
  EXPECT_EQ(target.kind(), RTTI_CELL);
  EXPECT_EQ(target.value(), 0xFFFFFF);
  EXPECT_EQ(target.cell_index(), -1);
}

// The sub-cell encoding: 12 bits per axis, Y over X, in units of 16 leptons.
// A cell maps to the unit at its center.
TEST(TargetTest, FromCellHoldsTheSubCellCenter) {
  const Target target = Target::FromCell(kCell);
  EXPECT_EQ(target.kind(), RTTI_CELL);
  EXPECT_EQ(target.value(), 0x38058);  // Y unit 0x38 over X unit 0x58.
  EXPECT_EQ(target.packed(), 0x09038058);
  // Decoding returns the center of that 16-lepton unit, not of the cell.
  EXPECT_EQ(target.AsCoordinate(), Coordinate::FromLeptons(0x588, 0x388));
  EXPECT_EQ(target.AsCell(), kCell);
}

TEST(TargetTest, FromCellReadsASentinelCellAsFullComponents) {
  EXPECT_EQ(Target::FromCell(static_cast<CELL>(-1)).value(), 0x7F87F8);
}

TEST(TargetTest, FromCoordinateKeepsTwelveBitsPerAxis) {
  const Target target =
      Target::FromCoordinate(Coordinate::FromLeptons(0x1234, 0x5678));
  EXPECT_EQ(target.kind(), RTTI_CELL);
  EXPECT_EQ(target.value(), 0x567123);
  EXPECT_EQ(target.AsCoordinate(), Coordinate::FromLeptons(0x1238, 0x5678));
  EXPECT_EQ(target.AsCell(), XY_Cell(0x12, 0x56));
  EXPECT_EQ(Target::FromCoordinate(Coordinate::FromPacked(0xFFFFFFFFU)).value(),
            0xFFFFFF);
}

TEST(TargetTest, ACellsCenterEncodesLikeTheCell) {
  EXPECT_EQ(Target::FromCoordinate(Cell_Coord(kCell)), Target::FromCell(kCell));
}

// The two encodings disagree: each decoder reads only its own.
TEST(TargetTest, TheTwoCellEncodingsDiffer) {
  EXPECT_NE(Target::FromCell(kCell), Target::FromCellIndex(kCell));
  EXPECT_NE(Target::FromCell(kCell).cell_index(), kCell);
}

TEST(TargetTest, NoTargetDecodesToNothing) {
  EXPECT_EQ(Target{}.AsCoordinate(), Coordinate{});
  EXPECT_EQ(Target{}.AsMovementCoordinate(), Coordinate{});
  EXPECT_EQ(Target{}.AsObject(), nullptr);
  EXPECT_EQ(Target{}.AsTechno(), nullptr);
  EXPECT_EQ(Target{}.AsObjectInSlot(), nullptr);
  EXPECT_EQ(Target{}.AsTechnoInSlot(), nullptr);
  EXPECT_EQ(Target{}.AsUnit(), nullptr);
  EXPECT_EQ(Target{}.AsTechnoType(), nullptr);
}

TEST(TargetTest, NullPointersAreNoTarget) {
  EXPECT_EQ(Target::Of(static_cast<const AbstractClass*>(nullptr)), Target{});
}

TEST(TargetTest, OfAnObjectIsItsKindAndHeapIndex) {
  const AbstractClass object(RTTI_UNIT, 7);
  EXPECT_EQ(Target::Of(&object), Target::Make(RTTI_UNIT, 7));
  EXPECT_EQ(object.As_Target(), Target::Make(RTTI_UNIT, 7));
}

TEST(TargetTest, ATypeIsItsKindAndIndex) {
  const AbstractTypeClass type(RTTI_UNITTYPE, 3, 0, "TEST");
  EXPECT_EQ(type.As_Target(), Target::Make(RTTI_UNITTYPE, 3));
}

TEST(TargetTest, SerializesTheWordLittleEndian) {
  std::vector<std::byte> bytes;
  VectorSink sink(bytes);
  // The call operator is non-const; misc-const-correctness misses the call
  // because its argument is template-dependent.
  // NOLINTNEXTLINE(misc-const-correctness)
  ArchiveWriter writer(sink);
  Target written = Target::Make(RTTI_UNIT, 0x123456);
  writer(written);
  ASSERT_TRUE(writer.ok());
  EXPECT_EQ(bytes, (std::vector<std::byte>{std::byte{0x56}, std::byte{0x34},
                                           std::byte{0x12}, std::byte{0x1C}}));

  SpanSource source{std::span<const std::byte>(bytes)};
  // NOLINTNEXTLINE(misc-const-correctness)
  ArchiveReader reader(source);
  Target read{};
  reader(read);
  EXPECT_EQ(std::string(reader.error()), "");
  EXPECT_EQ(read, written);
}

}  // namespace
}  // namespace ra
