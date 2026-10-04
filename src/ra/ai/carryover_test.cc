// Tests that a carryover record survives the saved game and rebuilds its
// object, against a real Game with no game data: the type objects, one house
// and one vehicle are built by hand.

#include "ra/ai/carryover.h"

#include <cstddef>
#include <span>
#include <string>
#include <vector>

#include "base/fixed.h"
#include "base/pooled.h"
#include "engine/stream/archive.h"
#include "engine/stream/span_source.h"
#include "engine/stream/vector_sink.h"
#include "game/sim/direction.h"
#include "gtest/gtest.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/core/cell.h"
#include "ra/core/defines.h"
#include "ra/core/object_heaps.h"
#include "ra/core/target.h"
#include "ra/core/type_heaps.h"
#include "ra/data/rules.h"
#include "ra/map/map.h"
#include "ra/map/world.h"
#include "ra/objects/building.h"
#include "ra/objects/house.h"
#include "ra/objects/infantry.h"
#include "ra/objects/type.h"
#include "ra/objects/unit.h"
#include "ra/startup/game_state.h"
#include "ra/startup/game_test_util.h"

namespace ra {
namespace {

// Where the recorded object stands.
constexpr Cell kCell = Cell::FromXY(40, 50);

// A cell in the border the fixture's map leaves around its playable area.
constexpr Cell kBorderCell = Cell::FromXY(0, 50);

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
    TheTypeHeaps().infantry().Reset(magic_enum::enum_count<InfantryType>());
    InfantryTypeClass::Init_Heap();
    TheObjectHeaps().infantry().Reset(4);
    TheTypeHeaps().building().Reset(magic_enum::enum_count<StructType>());
    BuildingTypeClass::Init_Heap();
    TheObjectHeaps().building().Reset(2);
    // No rules are loaded, and infantry are only placed on ground that foot
    // soldiers can cross.
    TheRules().ground().at(LAND_CLEAR).Cost.at(SPEED_FOOT) = Fixed(1);
    // A building put on the map asks whether the player has spied on it.
    ThePlayer() = base::NewPooled<HouseClass>(HOUSE_GREECE);
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

  // A damaged rifleman owned by Greece, standing in `cell`.
  static InfantryClass& MakeInfantry(const Cell cell) {
    auto* infantry = base::NewPooled<InfantryClass>(INFANTRY_E1, HOUSE_GREECE);
    EXPECT_NE(infantry, nullptr);
    infantry->Coord = cell.Center();
    infantry->Strength = 21;
    return *infantry;
  }

  // A record of a barracks owned by Greece whose top left cell is kCell. The
  // barracks itself is gone again.
  static Carryover RecordBarracks() {
    auto* building =
        base::NewPooled<BuildingClass>(STRUCT_BARRACKS, HOUSE_GREECE);
    EXPECT_NE(building, nullptr);
    building->Coord = kCell.Center();
    building->Strength = 300;
    const Carryover record(*building);
    base::DeletePooled(building);
    return record;
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

TEST_F(CarryoverTest, RecreatesNothingForAnEmptyRecord) {
  const Carryover empty;

  empty.Recreate();

  EXPECT_EQ(TheObjectHeaps().unit().Count(), 0);
  EXPECT_EQ(TheObjectHeaps().infantry().Count(), 0);
}

TEST_F(CarryoverTest, RecreatesNothingWhenTheObjectPoolIsFull) {
  const Carryover record(MakeUnit());
  while (base::NewPooled<UnitClass>(UNIT_JEEP, HOUSE_GREECE) != nullptr) {
  }
  const int full = TheObjectHeaps().unit().Count();

  record.Recreate();

  EXPECT_EQ(TheObjectHeaps().unit().Count(), full);
}

TEST_F(CarryoverTest, RecreatesInfantry) {
  InfantryClass& original = MakeInfantry(kCell);
  const Carryover record(original);
  base::DeletePooled(&original);

  record.Recreate();

  ASSERT_EQ(TheObjectHeaps().infantry().Count(), 1);
  const InfantryClass* infantry = TheObjectHeaps().infantry().ActiveAt(0);
  EXPECT_EQ(infantry->Class->Type, INFANTRY_E1);
  EXPECT_EQ(infantry->Owner(), HOUSE_GREECE);
  EXPECT_EQ(infantry->Strength, 21);
  EXPECT_EQ(Cell::Containing(infantry->Coord), kCell);
  EXPECT_FALSE(infantry->IsInLimbo);
  EXPECT_EQ(TheWorld().scenario_init(), 1);
}

TEST_F(CarryoverTest, DeletesInfantryOutsideThePlayableMap) {
  InfantryClass& original = MakeInfantry(kBorderCell);
  const Carryover record(original);
  base::DeletePooled(&original);

  record.Recreate();

  EXPECT_EQ(TheObjectHeaps().infantry().Count(), 0);
  EXPECT_EQ(TheWorld().scenario_init(), 1);
}

TEST_F(CarryoverTest, DeletesInfantryWhoseCellAVehicleHolds) {
  InfantryClass& original = MakeInfantry(kCell);
  const Carryover record(original);
  base::DeletePooled(&original);
  ASSERT_TRUE(MakeUnit().Unlimbo(kCell.Center()));

  record.Recreate();

  EXPECT_EQ(TheObjectHeaps().infantry().Count(), 0);
  EXPECT_EQ(TheWorld().scenario_init(), 1);
}

TEST_F(CarryoverTest, RecreatesNoVehicleOnAnOccupiedCell) {
  // The original stays on the map, in the cell the record names.
  UnitClass& original = MakeUnit();
  ASSERT_TRUE(original.Unlimbo(kCell.Center()));
  const Carryover record(original);

  record.Recreate();

  ASSERT_EQ(TheObjectHeaps().unit().Count(), 1);
  EXPECT_EQ(TheObjectHeaps().unit().ActiveAt(0), &original);
}

TEST_F(CarryoverTest, RecreatesNoVehicleOnACellInfantryHold) {
  const Carryover record(MakeUnit());
  base::DeletePooled(TheObjectHeaps().unit().ActiveAt(0));
  TheWorld().scenario_init() = 0;
  ASSERT_TRUE(MakeInfantry(kCell).Unlimbo(kCell.Center(), game::DIR_N));
  TheWorld().scenario_init() = 1;

  record.Recreate();

  EXPECT_EQ(TheObjectHeaps().unit().Count(), 0);
}

TEST_F(CarryoverTest, RecreatesNoVehicleOutsideThePlayableMap) {
  UnitClass& original = MakeUnit();
  original.Coord = kBorderCell.Center();
  const Carryover record(original);
  base::DeletePooled(&original);

  record.Recreate();

  EXPECT_EQ(TheObjectHeaps().unit().Count(), 0);
}

TEST_F(CarryoverTest, RecreatesABuilding) {
  const Carryover record = RecordBarracks();

  record.Recreate();

  ASSERT_EQ(TheObjectHeaps().building().Count(), 1);
  const BuildingClass* building = TheObjectHeaps().building().ActiveAt(0);
  EXPECT_EQ(building->Class->Type, STRUCT_BARRACKS);
  EXPECT_EQ(building->Strength, 300);
  EXPECT_EQ(Cell::Containing(building->Coord), kCell);
  EXPECT_FALSE(building->IsInLimbo);
}

TEST_F(CarryoverTest, RecreatesNoBuildingOverAnObjectInItsFootprint) {
  const Carryover record = RecordBarracks();
  // Not the cell the record names, but one the barracks covers.
  const Cell corner = Cell::FromXY(kCell.x() + 1, kCell.y() + 1);
  ASSERT_TRUE(MakeUnit().Unlimbo(corner.Center()));

  record.Recreate();

  EXPECT_EQ(TheObjectHeaps().building().Count(), 0);
}

}  // namespace
}  // namespace ra
