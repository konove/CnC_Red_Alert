// Exercise real game serializers without loading MIX files or starting SDL.
#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <new>
#include <span>
#include <vector>

#include "base/array.h"
#include "base/buffer.h"
#include "base/installed.h"
#include "base/numeric.h"
#include "base/pooled.h"
#include "engine/stream/archive.h"
#include "engine/stream/span_sink.h"
#include "engine/stream/span_source.h"
#include "game/core/game_clock.h"
#include "game/core/heap.h"
#include "game/sim/direction.h"
#include "gtest/gtest.h"
#include "td/ai/special.h"
#include "td/ai/team.h"
#include "td/ai/trigger.h"
#include "td/core/defines.h"
#include "td/core/inline.h"
#include "td/core/object_heaps.h"
#include "td/debug/debug_state.h"
#include "td/editor/mapedit.h"
#include "td/map/map.h"
#include "td/map/world.h"
#include "td/net/event.h"
#include "td/net/session.h"
#include "td/objects/cell.h"
#include "td/objects/factory.h"
#include "td/objects/house.h"
#include "td/objects/target.h"
#include "td/objects/unit.h"
#include "td/startup/game_state.h"

namespace td {

// The frame counter and the object heaps every suite in this binary needs;
// Game owns them in the real game.
namespace {
game::GameClock game_clock;
const base::Installed<game::GameClock>::Scope game_clock_scope(game_clock);
ObjectHeaps object_heaps;
const base::Installed<ObjectHeaps>::Scope object_heaps_scope(object_heaps);
// NOLINTBEGIN(bugprone-throwing-static-initialization): a test binary
// that runs out of memory building the world has nothing to report.
World world;
const base::Installed<World>::Scope world_scope(world);
// The serializers read the game type out of the session, and the team
// parser reports through the message list it owns.
SessionClass session;
const base::Installed<SessionClass>::Scope session_scope(session);
// Object validation reports through the debug switches, which read the
// command overrides.
SpecialClass special{};
const base::Installed<SpecialClass>::Scope special_scope(special);
// The serializers also read the run-state flags.
GameState game_state;
const base::Installed<GameState>::Scope game_state_scope(game_state);
// NOLINTEND(bugprone-throwing-static-initialization)
}  // namespace

namespace {
template <class T>
std::vector<uint8_t> Save(T& object) {
  std::vector<uint8_t> bytes(8192);
  SpanSink sink(std::as_writable_bytes(std::span(bytes)));
  ArchiveWriter writer(sink);
  object.Serialize(writer);
  EXPECT_TRUE(writer.ok());
  bytes.resize(base::ToSize(sink.bytes_written()));
  return bytes;
}

template <class T>
bool Restore(T& object, const std::vector<uint8_t>& bytes) {
  SpanSource source(std::as_bytes(std::span(bytes)));
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
    // Selecting and drawing an object reads the debug switches, which Game
    // installs in the real game.
    debug_state_scope_ =
        std::make_unique<base::Installed<DebugState>::Scope>(debug_state_);
    TheGameState().active() = false;
    game::TheGameClock().set_frame(100);
    TheObjectHeaps().house().Reset(8);
    TheObjectHeaps().unit().Reset(8);
    TheObjectHeaps().trigger().Reset(8);
    TheObjectHeaps().factory().Reset(8);
    TheObjectHeaps().team().Reset(8);
    TheMap().Replace_Cells(MAP_CELL_TOTAL);
    TheWorld().cell_triggers().resize(MAP_CELL_TOTAL);
    TheMap().Init_Cells();
    ThePlayer() = base::NewPooled<HouseClass>(HOUSE_GOOD);
  }
  static void TearDownTestSuite() {
    TheMap().Init_Cells();
    TheWorld().cell_triggers().clear();
    while (TheObjectHeaps().unit().Count() != 0) {
      base::DeletePooled(TheObjectHeaps().unit().ActiveAt(0));
    }
    while (TheObjectHeaps().trigger().Count() != 0) {
      base::DeletePooled(TheObjectHeaps().trigger().ActiveAt(0));
    }
    base::DeletePooled(ThePlayer());
    ThePlayer() = nullptr;
    TheMap().Free_Cells();
    debug_state_scope_.reset();
  }

 private:
  static inline DebugState debug_state_;
  static inline std::unique_ptr<base::Installed<DebugState>::Scope>
      debug_state_scope_;
};

TEST_F(TdArchiveRoundTripTest, TriggerIniPreserves64BitDataAndRejectsOverflow) {
  auto* trigger = base::NewPooled<TriggerClass>();
  char name[] = "CRED";
  char entry[] = "Credits,None,4294967296,GoodGuy,None,0";
  trigger->Fill_In(name, entry);
  EXPECT_EQ(trigger->Data, INT64_C(4294967296));
  EXPECT_EQ(trigger->DataCopy, INT64_C(4294967296));

  char overflow[] = "Credits,None,9223372036854775808,GoodGuy,None,0";
  trigger->Fill_In(name, overflow);
  EXPECT_EQ(trigger->Data, 0);
  EXPECT_EQ(trigger->DataCopy, 0);
  base::DeletePooled(trigger);
}

TEST_F(TdArchiveRoundTripTest, EventConstructorsClearExecutionFlagAndUnusedWireBytes) {
  const auto check = [](auto configure, auto... args) {
    EventClass expected;
    expected.ID = static_cast<unsigned>(
        TheObjectHeaps().house().SlotIndexOf(ThePlayer()));
    expected.Frame = static_cast<unsigned>(game::CurrentFrame());
    configure(expected);
    alignas(EventClass) std::array<unsigned char, sizeof(EventClass)> storage{};
    storage.fill(0xff);
    // Constructed in the test's own storage; there is nothing to free.
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    auto* event = new (storage.data()) EventClass(args...);
    EXPECT_EQ(event->IsExecuted, 0U);
    EXPECT_EQ(event->MPlayerID, 0);
    std::array<unsigned char, sizeof(EventClass)> expected_bytes{};
    base::CopyBytes(std::as_writable_bytes(std::span(expected_bytes)),
                    base::ObjectBytes(expected), sizeof(expected));
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
  auto* unit = base::NewPooled<UnitClass>(UNIT_LTANK, HOUSE_GOOD);
  auto* trigger = base::NewPooled<TriggerClass>();
  auto& cell = TheMap().at(100);
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
  base::At(TheWorld().cell_triggers(), 100) = trigger;
  const auto bytes = Save(cell);
  cell.Reset();
  base::At(TheWorld().cell_triggers(), 100) = nullptr;
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
  EXPECT_EQ(base::At(TheWorld().cell_triggers(), 100), trigger);
  EXPECT_EQ(Save(cell), bytes);
}

// An independent field stream populates private scan arrays as well as the
// public map dimensions, rather than relying on the serializer for both ends.
std::vector<uint8_t> MapFields(int32_t growth_count = 2) {
  std::vector<uint8_t> bytes(128);
  SpanSink sink(std::as_writable_bytes(std::span(bytes)));
  ArchiveWriter writer(sink);
  int32_t x = 1;
  int32_t y = 2;
  int32_t width = 60;
  int32_t height = 59;
  int32_t spread_count = 1;
  int64_t total = (int64_t{1} * 1024 * 1024 * 1024 * 1024) + 17;
  int16_t scan = 3072;
  int16_t growth[] = {100, 200};
  int16_t spread = 300;
  bool forward = true;
  writer(x, y, width, height, total, growth_count, spread_count, scan, forward);
  writer(growth, spread);
  EXPECT_TRUE(writer.ok());
  bytes.resize(base::ToSize(sink.bytes_written()));
  return bytes;
}

TEST_F(TdArchiveRoundTripTest, MapMembersPreserveWideValueAndPopulatedScanLists) {
  MapClass& map = TheMap();
  map.MapClass::Init_Clear();
  const auto bytes = MapFields();
  ASSERT_TRUE(Restore(map, bytes));
  EXPECT_EQ(map.MapCellX, 1);
  EXPECT_EQ(map.MapCellY, 2);
  EXPECT_EQ(map.MapCellWidth, 60);
  EXPECT_EQ(map.MapCellHeight, 59);
  EXPECT_EQ(map.TotalValue, (int64_t{1} * 1024 * 1024 * 1024 * 1024) + 17);
  EXPECT_EQ(Save(map), bytes);
}

TEST_F(TdArchiveRoundTripTest, MapRejectsOversizedScanListBeforeReadingEntries) {
  MapClass& map = TheMap();
  map.MapClass::Init_Clear();
  const auto bytes = MapFields(51);
  SpanSource source(std::as_bytes(std::span(bytes)));
  ArchiveReader reader(source);
  map.Serialize(reader);
  EXPECT_FALSE(reader.ok());
}

TEST_F(TdArchiveRoundTripTest, HouseRestoresEconomyFlagsTimersAndTypeIdentity) {
  auto& house = *ThePlayer();
  house.Credits = (int64_t{1} * 1024 * 1024 * 1024 * 1024) + 123;
  house.InitialCredits = (int64_t{1} * 512 * 1024 * 1024 * 1024) + 456;
  house.IsHuman = true;
  house.IsAlerted = true;
  house.NukePieces = 5;
  house.BorrowedTime.Set(777);
  house.UnitsKilled.at(HOUSE_BAD) = 19;
  const auto bytes = Save(house);
  HouseClass restored;
  ASSERT_TRUE(Restore(restored, bytes));
  EXPECT_EQ(restored.Class, house.Class);
  EXPECT_EQ(restored.RemapTable.data(), house.RemapTable.data());
  EXPECT_EQ(restored.Credits, house.Credits);
  EXPECT_EQ(restored.InitialCredits, house.InitialCredits);
  EXPECT_TRUE(restored.IsHuman && restored.IsAlerted);
  EXPECT_EQ(restored.NukePieces, 5);
  EXPECT_EQ(restored.BorrowedTime.Remaining(), 777);
  EXPECT_EQ(restored.UnitsKilled.at(HOUSE_BAD), 19);
  EXPECT_EQ(Save(restored), bytes);
}

TEST_F(TdArchiveRoundTripTest, UnitRestoresBaseFieldsReferencesTimersAndPathTail) {
  auto* unit = base::NewPooled<UnitClass>(UNIT_LTANK, HOUSE_GOOD);
  unit->Coord = Cell_Coord(1000);
  unit->Strength = 123;
  unit->NavCom = As_Target(CELL{1200});
  unit->Flagged = HOUSE_BAD;
  unit->Reload.Set(87);
  unit->Path[0] = game::FACING_E;
  unit->Path[1] = game::FACING_NONE;
  unit->Path[kConquerPathMax - 1] = game::FACING_S;
  const auto bytes = Save(*unit);
  auto* restored = base::NewPooled<UnitClass>();
  ASSERT_TRUE(Restore(*restored, bytes));
  EXPECT_EQ(restored->Class, unit->Class);
  EXPECT_EQ(restored->House, ThePlayer());
  EXPECT_EQ(restored->Coord, unit->Coord);
  EXPECT_EQ(restored->Strength, 123);
  EXPECT_EQ(restored->NavCom, unit->NavCom);
  EXPECT_EQ(restored->Flagged, HOUSE_BAD);
  EXPECT_EQ(restored->Reload.Remaining(), 87);
  EXPECT_EQ(restored->Path[0], game::FACING_E);
  EXPECT_EQ(restored->Path[1], game::FACING_NONE);
  EXPECT_EQ(restored->Path[kConquerPathMax - 1], game::FACING_S);
  EXPECT_EQ(Save(*restored), bytes);
}

TEST_F(TdArchiveRoundTripTest, TruncatedCellHouseAndUnitRecordsFail) {
  const auto check = [](auto& object) {
    auto bytes = Save(object);
    bytes.pop_back();
    SpanSource source(std::as_bytes(std::span(bytes)));
    ArchiveReader reader(source);
    object.Serialize(reader);
    EXPECT_FALSE(reader.ok());
  };
  check(TheMap().at(200));
  check(*ThePlayer());
  auto* unit = base::NewPooled<UnitClass>(UNIT_LTANK, HOUSE_GOOD);
  check(*unit);
}
}  // namespace

}  // namespace td
