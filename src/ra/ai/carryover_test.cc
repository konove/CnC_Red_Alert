// Tests that a carryover record survives the saved game and rebuilds its
// object, against a real Game with no game data: the type objects, one house
// and one vehicle are built by hand.

#include "ra/ai/carryover.h"

#include <cstddef>
#include <span>
#include <string>
#include <vector>

#include "base/pooled.h"
#include "engine/stream/archive.h"
#include "engine/stream/span_source.h"
#include "engine/stream/vector_sink.h"
#include "gtest/gtest.h"
#include "ra/core/cell.h"
#include "ra/core/defines.h"
#include "ra/core/object_heaps.h"
#include "ra/core/target.h"
#include "ra/map/map.h"
#include "ra/map/world.h"
#include "ra/objects/house.h"
#include "ra/objects/type.h"
#include "ra/objects/unit.h"
#include "ra/startup/game_state.h"
#include "ra/startup/game_test_util.h"

namespace ra {
namespace {

// Where the recorded vehicle stands.
constexpr Cell kCell = Cell::FromXY(40, 50);

std::vector<std::byte> Write(Carryover& record) {
  std::vector<std::byte> bytes;
  VectorSink sink(bytes);
  // NOLINTNEXTLINE(misc-const-correctness): the call operator is non-const.
  ArchiveWriter writer(sink);
  writer(record);
  EXPECT_TRUE(writer.ok());
  return bytes;
}

// Reads a record from `bytes` and returns the reader's error, empty on
// success.
std::string Read(const std::vector<std::byte>& bytes, Carryover& record) {
  SpanSource source{std::span<const std::byte>(bytes)};
  // NOLINTNEXTLINE(misc-const-correctness): the call operator is non-const.
  ArchiveReader reader(source);
  reader(record);
  return std::string(reader.error());
}

class CarryoverTest : public GameFixture {
 protected:
  CarryoverTest() {
    base::NewPooled<HouseClass>(HOUSE_GREECE);
    // Recreate() runs while a scenario is being read, in a running game.
    TheWorld().scenario_init() = 1;
    TheGameState().active() = true;
  }

  // A damaged ranger owned by Greece, standing in kCell.
  static UnitClass& MakeUnit() {
    auto* unit = base::NewPooled<UnitClass>(UNIT_JEEP, HOUSE_GREECE);
    EXPECT_NE(unit, nullptr);
    unit->Coord = kCell.Center();
    unit->Strength = 37;
    return *unit;
  }
};

TEST_F(CarryoverTest, WritesTheTypeAsATarget) {
  Carryover record(MakeUnit());
  const std::vector<std::byte> bytes = Write(record);

  Target type = Target::Make(RTTI_UNITTYPE, static_cast<int>(UNIT_JEEP));
  std::vector<std::byte> expected;
  VectorSink sink(expected);
  // NOLINTNEXTLINE(misc-const-correctness): the call operator is non-const.
  ArchiveWriter writer(sink);
  writer(type);
  ASSERT_GE(bytes.size(), expected.size());
  EXPECT_EQ(std::vector<std::byte>(bytes.begin(),
                                   bytes.begin() + std::ssize(expected)),
            expected);
}

TEST_F(CarryoverTest, RoundTripsThroughTheSavedGame) {
  Carryover written(MakeUnit());
  const std::vector<std::byte> bytes = Write(written);

  Carryover read;
  EXPECT_EQ(Read(bytes, read), "");
  EXPECT_EQ(Write(read), bytes);
}

TEST_F(CarryoverTest, RejectsAnEmptyRecord) {
  Carryover empty;
  const std::vector<std::byte> bytes = Write(empty);

  Carryover read;
  EXPECT_EQ(Read(bytes, read), "invalid carryover object type");
}

TEST_F(CarryoverTest, RecreatesTheObjectFromALoadedRecord) {
  UnitClass& original = MakeUnit();
  Carryover written(original);
  const std::vector<std::byte> bytes = Write(written);
  base::DeletePooled(&original);
  ASSERT_EQ(TheObjectHeaps().unit().Count(), 0);

  Carryover read;
  ASSERT_EQ(Read(bytes, read), "");
  read.Recreate();

  ASSERT_EQ(TheObjectHeaps().unit().Count(), 1);
  const UnitClass* unit = TheObjectHeaps().unit().ActiveAt(0);
  EXPECT_EQ(unit->Class->Type, UNIT_JEEP);
  EXPECT_EQ(unit->Owner(), HOUSE_GREECE);
  EXPECT_EQ(unit->Strength, 37);
  EXPECT_EQ(Cell::Containing(unit->Coord), kCell);
  EXPECT_FALSE(unit->IsInLimbo);
}

TEST_F(CarryoverTest, RecreatesNothingForAHouseNotInTheScenario) {
  UnitClass& original = MakeUnit();
  const Carryover record(original);
  base::DeletePooled(&original);
  base::DeletePooled(HouseClass::As_Pointer(HOUSE_GREECE));

  record.Recreate();

  EXPECT_EQ(TheObjectHeaps().unit().Count(), 0);
}

}  // namespace
}  // namespace ra
