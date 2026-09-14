// Exercise real game serializers without loading MIX files or starting SDL.
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>
#include <span>
#include <vector>

#include "base/numeric.h"
#include "base/types.h"
#include "gtest/gtest.h"
#include "td/cell.h"
#include "td/defines.h"
#include "td/event.h"
#include "td/externs.h"
#include "td/factory.h"
#include "td/globals.h"
#include "td/heap.h"
#include "td/house.h"
#include "td/inline.h"
#include "td/map.h"
#include "td/mapedit.h"
#include "td/special.h"
#include "td/target.h"
#include "td/team.h"
#include "td/trigger.h"
#include "td/unit.h"
#include "tech/archive.h"
#include "tech/xpipe.h"
#include "tech/xstraw.h"

namespace {
class CountingBufferPipe : public BufferPipe {
 public:
  using BufferPipe::BufferPipe;
  int count = 0;
  base::ssize Put(std::span<const std::byte> data) override {
    const base::ssize written = BufferPipe::Put(data);
    EXPECT_EQ(written, std::ssize(data));
    count += static_cast<int>(written);
    return written;
  }
};

template <class T>
std::vector<uint8_t> Save(T& object) {
  std::vector<uint8_t> bytes(8192);
  CountingBufferPipe sink(std::as_writable_bytes(std::span(bytes)));
  ArchiveWriter writer(sink);
  object.Serialize(writer);
  bytes.resize(base::ToSize(sink.count));
  return bytes;
}

template <class T>
bool Restore(T& object, const std::vector<uint8_t>& bytes) {
  BufferStraw source(std::as_bytes(std::span(bytes)));
  ArchiveReader reader(source);
  object.Serialize(reader);
  if (!reader.ok()) {
    ADD_FAILURE() << reader.error();
  }
  return reader.ok();
}

class TdArchiveRoundTripTest : public testing::Test {
 protected:
  static void SetUpTestSuite() {
    GameActive = false;
    Frame = 100;
    Houses.Set_Heap(8);
    Units.Set_Heap(8);
    Triggers.Set_Heap(8);
    Factories.Set_Heap(8);
    Teams.Set_Heap(8);
    Map.Resize(MAP_CELL_TOTAL);
    CellTriggers.Resize(MAP_CELL_TOTAL);
    Map.Init_Cells();
    PlayerPtr = new HouseClass(HOUSE_GOOD);
  }
  static void TearDownTestSuite() {
    Map.Init_Cells();
    CellTriggers.Clear();
    while (Units.Count() != 0) {
      delete Units.Ptr(0);
    }
    while (Triggers.Count() != 0) {
      delete Triggers.Ptr(0);
    }
    delete PlayerPtr;
    PlayerPtr = nullptr;
    Map.Clear();
  }
};

TEST_F(TdArchiveRoundTripTest, TriggerIniPreserves64BitDataAndRejectsOverflow) {
  auto* trigger = new TriggerClass;
  char name[] = "CRED";
  char entry[] = "Credits,None,4294967296,GoodGuy,None,0";
  trigger->Fill_In(name, entry);
  EXPECT_EQ(trigger->Data, INT64_C(4294967296));
  EXPECT_EQ(trigger->DataCopy, INT64_C(4294967296));

  char overflow[] = "Credits,None,9223372036854775808,GoodGuy,None,0";
  trigger->Fill_In(name, overflow);
  EXPECT_EQ(trigger->Data, 0);
  EXPECT_EQ(trigger->DataCopy, 0);
  delete trigger;
}

TEST_F(TdArchiveRoundTripTest, EventConstructorsClearExecutionFlagAndUnusedWireBytes) {
  const auto check = [](auto configure, auto... args) {
    EventClass expected;
    expected.ID = static_cast<unsigned>(Houses.ID(PlayerPtr));
    expected.Frame = static_cast<unsigned>(Frame);
    configure(expected);
    alignas(EventClass) std::array<unsigned char, sizeof(EventClass)> storage{};
    storage.fill(0xff);
    auto* event = new (storage.data()) EventClass(args...);
    EXPECT_EQ(event->IsExecuted, 0U);
    EXPECT_EQ(event->MPlayerID, 0);
    std::array<unsigned char, sizeof(EventClass)> expected_bytes{};
    std::memcpy(expected_bytes.data(), &expected, sizeof(expected));
    EXPECT_EQ(storage, expected_bytes);
    event->~EventClass();
  };
  check([](EventClass& e) { e.Type = EventClass::OPTIONS; }, EventClass::OPTIONS);
  check([](EventClass& e) {
    e.Type = EventClass::GAMESPEED;
    e.Data.General.Value = 3;
  }, EventClass::GAMESPEED, 3);
  check([](EventClass& e) {
    e.Type = EventClass::IDLE;
    e.Data.Target.Whom = TARGET{123};
  }, EventClass::IDLE, TARGET{123});
  check([](EventClass& e) {
    e.Type = EventClass::IDLE;
    e.Data.NavCom.Whom = TARGET{123};
    e.Data.NavCom.Where = TARGET{456};
  }, EventClass::IDLE, TARGET{123}, TARGET{456});
  check([](EventClass& e) {
    e.Type = EventClass::MEGAMISSION;
    e.Data.MegaMission.Whom = TARGET{123};
    e.Data.MegaMission.Mission = MISSION_MOVE;
    e.Data.MegaMission.Target = TARGET{456};
    e.Data.MegaMission.Destination = TARGET{789};
  }, TARGET{123}, MISSION_MOVE, TARGET{456}, TARGET{789});
  check([](EventClass& e) {
    e.Type = EventClass::PRODUCE;
    e.Data.Specific.Type = RTTI_UNITTYPE;
    e.Data.Specific.ID = 2;
  }, EventClass::PRODUCE, RTTI_UNITTYPE, 2);
  check([](EventClass& e) {
    e.Type = EventClass::PLACE;
    e.Data.Place.Type = RTTI_BUILDINGTYPE;
    e.Data.Place.Cell = CELL{100};
  }, EventClass::PLACE, RTTI_BUILDINGTYPE, CELL{100});
  check([](EventClass& e) {
    e.Type = EventClass::SPECIAL_PLACE;
    e.Data.Special.ID = 2;
    e.Data.Special.Cell = CELL{100};
  }, EventClass::SPECIAL_PLACE, 2, CELL{100});
  check([](EventClass& e) {
    e.Type = EventClass::ANIMATION;
    e.Data.Anim.What = ANIM_FIRE_SMALL;
    e.Data.Anim.Owner = HOUSE_GOOD;
    e.Data.Anim.Where = COORDINATE{100};
  }, ANIM_FIRE_SMALL, HOUSE_GOOD, COORDINATE{100});
  SpecialClass options{};
  check([&options](EventClass& e) {
    e.Type = EventClass::SPECIAL;
    e.Data.Options.Data = options;
  }, options);
}

TEST_F(TdArchiveRoundTripTest, CellRestoresFlagsAndGappedObjectAndTriggerReferences) {
  auto* unit = new UnitClass(UNIT_LTANK, HOUSE_GOOD);
  auto* trigger = new TriggerClass;
  auto& cell = Map[100];
  cell.Reset();
  cell.IsPlot = cell.IsCursorHere = cell.IsWaypoint = true;
  cell.IsRadarCursor = cell.IsFlagged = cell.IsTrigger = true;
  cell.IsMapped = cell.IsVisible = true;
  cell.TIcon = 7;
  cell.OverlayData = 3;
  cell.SmudgeData = 2;
  cell.Owner = HOUSE_GOOD;
  cell.InfType = HOUSE_BAD;
  cell.OccupierPtr = unit;
  cell.Overlappers[2] = unit;
  cell.Flag.Composite = 2;
  CellTriggers[100] = trigger;
  const auto bytes = Save(cell);
  cell.Reset();
  CellTriggers[100] = nullptr;
  ASSERT_TRUE(Restore(cell, bytes));
  EXPECT_TRUE(cell.IsMapped && cell.IsVisible && cell.IsTrigger && cell.IsFlagged);
  EXPECT_TRUE(cell.IsPlot && cell.IsCursorHere && cell.IsWaypoint && cell.IsRadarCursor);
  EXPECT_EQ(cell.TIcon, 7);
  EXPECT_EQ(cell.Owner, HOUSE_GOOD);
  EXPECT_EQ(cell.InfType, HOUSE_BAD);
  EXPECT_EQ(cell.OccupierPtr, unit);
  EXPECT_EQ(cell.Overlappers[0], nullptr);
  EXPECT_EQ(cell.Overlappers[1], nullptr);
  EXPECT_EQ(cell.Overlappers[2], unit);
  EXPECT_EQ(CellTriggers[100], trigger);
  EXPECT_EQ(Save(cell), bytes);
}

// An independent field stream populates private scan arrays as well as the
// public map dimensions, rather than relying on the serializer for both ends.
std::vector<uint8_t> MapFields(int32_t growth_count = 2) {
  std::vector<uint8_t> bytes(128);
  CountingBufferPipe sink(std::as_writable_bytes(std::span(bytes)));
  ArchiveWriter writer(sink);
  int32_t x = 1;
  int32_t y = 2;
  int32_t width = 60;
  int32_t height = 59;
  int32_t spread_count = 1;
  int64_t total = (int64_t{1} << 40) + 17;
  int16_t scan = 3072;
  int16_t growth[] = {100, 200};
  int16_t spread = 300;
  bool forward = true;
  writer(x, y, width, height, total, growth_count, spread_count, scan, forward);
  writer(growth, spread);
  bytes.resize(base::ToSize(sink.count));
  return bytes;
}

TEST_F(TdArchiveRoundTripTest, MapMembersPreserveWideValueAndPopulatedScanLists) {
  MapClass& map = Map;
  map.MapClass::Init_Clear();
  const auto bytes = MapFields();
  ASSERT_TRUE(Restore(map, bytes));
  EXPECT_EQ(map.MapCellX, 1);
  EXPECT_EQ(map.MapCellY, 2);
  EXPECT_EQ(map.MapCellWidth, 60);
  EXPECT_EQ(map.MapCellHeight, 59);
  EXPECT_EQ(map.TotalValue, (int64_t{1} << 40) + 17);
  EXPECT_EQ(Save(map), bytes);
}

TEST_F(TdArchiveRoundTripTest, MapRejectsOversizedScanListBeforeReadingEntries) {
  MapClass& map = Map;
  map.MapClass::Init_Clear();
  const auto bytes = MapFields(51);
  BufferStraw source(std::as_bytes(std::span(bytes)));
  ArchiveReader reader(source);
  map.Serialize(reader);
  EXPECT_FALSE(reader.ok());
}

TEST_F(TdArchiveRoundTripTest, HouseRestoresEconomyFlagsTimersAndTypeIdentity) {
  auto& house = *PlayerPtr;
  house.Credits = (int64_t{1} << 40) + 123;
  house.InitialCredits = (int64_t{1} << 39) + 456;
  house.IsHuman = true;
  house.IsAlerted = true;
  house.NukePieces = 5;
  house.BorrowedTime = 777;
  house.UnitsKilled[HOUSE_BAD] = 19;
  const auto bytes = Save(house);
  HouseClass restored;
  ASSERT_TRUE(Restore(restored, bytes));
  EXPECT_EQ(restored.Class, house.Class);
  EXPECT_EQ(restored.RemapTable, house.RemapTable);
  EXPECT_EQ(restored.Credits, house.Credits);
  EXPECT_EQ(restored.InitialCredits, house.InitialCredits);
  EXPECT_TRUE(restored.IsHuman && restored.IsAlerted);
  EXPECT_EQ(restored.NukePieces, 5);
  EXPECT_EQ(static_cast<int>(restored.BorrowedTime), 777);
  EXPECT_EQ(restored.UnitsKilled[HOUSE_BAD], 19);
  EXPECT_EQ(Save(restored), bytes);
}

TEST_F(TdArchiveRoundTripTest, UnitRestoresBaseFieldsReferencesTimersAndPathTail) {
  auto* unit = new UnitClass(UNIT_LTANK, HOUSE_GOOD);
  unit->Coord = Cell_Coord(1000);
  unit->Strength = 123;
  unit->NavCom = As_Target(CELL{1200});
  unit->Flagged = HOUSE_BAD;
  unit->Reload = 87;
  unit->Path[0] = FACING_E;
  unit->Path[1] = FACING_NONE;
  unit->Path[kConquerPathMax - 1] = FACING_S;
  const auto bytes = Save(*unit);
  auto* restored = new UnitClass;
  ASSERT_TRUE(Restore(*restored, bytes));
  EXPECT_EQ(restored->Class, unit->Class);
  EXPECT_EQ(restored->House, PlayerPtr);
  EXPECT_EQ(restored->Coord, unit->Coord);
  EXPECT_EQ(restored->Strength, 123);
  EXPECT_EQ(restored->NavCom, unit->NavCom);
  EXPECT_EQ(restored->Flagged, HOUSE_BAD);
  EXPECT_EQ(static_cast<int>(restored->Reload), 87);
  EXPECT_EQ(restored->Path[0], FACING_E);
  EXPECT_EQ(restored->Path[1], FACING_NONE);
  EXPECT_EQ(restored->Path[kConquerPathMax - 1], FACING_S);
  EXPECT_EQ(Save(*restored), bytes);
}

TEST_F(TdArchiveRoundTripTest, TruncatedCellHouseAndUnitRecordsFail) {
  const auto check = [](auto& object) {
    auto bytes = Save(object);
    bytes.pop_back();
    BufferStraw source(std::as_bytes(std::span(bytes)));
    ArchiveReader reader(source);
    object.Serialize(reader);
    EXPECT_FALSE(reader.ok());
  };
  check(Map[200]);
  check(*PlayerPtr);
  auto* unit = new UnitClass(UNIT_LTANK, HOUSE_GOOD);
  check(*unit);
}
}  // namespace
