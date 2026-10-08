#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "base/installed.h"
#include "base/random.h"
#include "engine/platform/timer.h"
#include "engine/stream/archive.h"
#include "engine/stream/span_sink.h"
#include "engine/stream/span_source.h"
#include "game/core/game_clock.h"
#include "game/sim/enums.h"
#include "game/sim/flasher.h"
#include "gtest/gtest.h"
#include "td/ai/teamtype.h"
#include "td/core/defines.h"
#include "td/core/rand.h"
#include "td/core/randomstate.h"
#include "td/core/special_flags.h"
#include "td/objects/abstract.h"
#include "td/objects/cell.h"
#include "td/objects/door.h"
#include "td/objects/fly.h"
#include "td/objects/fuse.h"
#include "td/objects/stage.h"
#include "td/objects/super.h"
#include "td/saveload/serialize.h"
#include "td/screens/score.h"

namespace td {

// The value tests need a frame source and the command overrides the
// serializers read, but no game session.
namespace {
game::GameClock game_clock;
const base::Installed<game::GameClock>::Scope game_clock_scope(game_clock);
SpecialFlags special{};
const base::Installed<SpecialFlags>::Scope special_scope(special);

template <class T>
std::array<uint8_t, 64> Save(T& value) {
  std::array<uint8_t, 64> bytes{};
  SpanSink sink(std::as_writable_bytes(std::span(bytes)));
  // The call operator is non-const; misc-const-correctness misses the call
  // because its argument is template-dependent.
  // NOLINTNEXTLINE(misc-const-correctness)
  ArchiveWriter writer(sink);
  writer(value);
  return bytes;
}

template <class T>
void Restore(T& value, const std::array<uint8_t, 64>& bytes) {
  SpanSource source(std::as_bytes(std::span(bytes)));
  // The call operator is non-const; misc-const-correctness misses the call
  // because its argument is template-dependent.
  // NOLINTNEXTLINE(misc-const-correctness)
  ArchiveReader reader(source);
  reader(value);
  ASSERT_TRUE(reader.ok()) << reader.error();
}

TEST(TdSaveValuesTest, CountdownReanchorsAndKeepsWideRemainingTime) {
  game::TheGameClock().set_frame(100);
  engine::platform::Timer<game::FrameTickSource> timer(int64_t{1} * 1024 *
                                                       1024 * 1024 * 1024);
  game::TheGameClock().set_frame(game::CurrentFrame() + 17);
  const int64_t remaining = timer.Remaining();
  const auto bytes = Save(timer);
  game::TheGameClock().set_frame(10000);
  engine::platform::Timer<game::FrameTickSource> loaded;
  Restore(loaded, bytes);
  EXPECT_TRUE(loaded.IsRunning());
  EXPECT_EQ(loaded.Remaining(), remaining);
  game::TheGameClock().set_frame(game::CurrentFrame() + 9);
  EXPECT_EQ(loaded.Remaining(), remaining - 9);
}

TEST(TdSaveValuesTest, ClearedAndExpiredCountdownsRemainDistinct) {
  game::TheGameClock().set_frame(100);
  engine::platform::Timer<game::FrameTickSource> timer(1);
  game::TheGameClock().set_frame(102);
  engine::platform::Timer<game::FrameTickSource> loaded;
  Restore(loaded, Save(timer));
  EXPECT_TRUE(loaded.IsRunning());
  EXPECT_EQ(loaded.Remaining(), 0);
  // What TCountDownTimerClass::Clear() did: nothing left, and not running.
  timer.Set(0);
  timer.Stop();
  Restore(loaded, Save(timer));
  EXPECT_FALSE(loaded.IsRunning());
  EXPECT_TRUE(loaded.IsFinished());
}

TEST(TdSaveValuesTest, CountdownRejectsTruncationAndNegativeDuration) {
  engine::platform::Timer<game::FrameTickSource> timer;
  std::array<uint8_t, 9> bytes{};
  SpanSource short_source(std::as_bytes(std::span(bytes).first(8)));
  ArchiveReader short_reader(short_source);
  short_reader(timer);
  EXPECT_FALSE(short_reader.ok());
  bytes.fill(0xff);
  bytes.back() = 1;
  SpanSource negative_source(std::as_bytes(std::span(bytes).first(9)));
  ArchiveReader negative_reader(negative_source);
  negative_reader(timer);
  EXPECT_FALSE(negative_reader.ok());
}

TEST(TdSaveValuesTest, FlightPreservesFractionalDistanceAndSpeed) {
  std::array<uint8_t, 64> bytes{};
  SpanSink sink(std::as_writable_bytes(std::span(bytes)));
  ArchiveWriter writer(sink);
  uint32_t accumulator = 511;
  game::MPHType speed = game::MPH_FAST;
  writer(accumulator, speed);
  FlyClass flight;
  Restore(flight, bytes);
  EXPECT_EQ(flight.Get_Speed(), speed);
  EXPECT_EQ(Save(flight), bytes);
}

TEST(TdSaveValuesTest, StagePreservesPartialAnimationCountdown) {
  StageClass stage;
  stage.Set_Stage(12);
  stage.Set_Rate(3);
  EXPECT_FALSE(stage.Graphic_Logic());
  StageClass loaded;
  Restore(loaded, Save(stage));
  EXPECT_EQ(loaded.Fetch_Stage(), 12);
  EXPECT_EQ(loaded.Fetch_Rate(), 3);
  EXPECT_FALSE(loaded.Graphic_Logic());
  EXPECT_TRUE(loaded.Graphic_Logic());
  EXPECT_EQ(loaded.Fetch_Stage(), 13);
}

TEST(TdSaveValuesTest, DoorResumesOpeningAndPreservesRedraw) {
  DoorClass door;
  ASSERT_TRUE(door.Open_Door(2, 4));
  door.AI();
  DoorClass loaded;
  Restore(loaded, Save(door));
  EXPECT_TRUE(loaded.Is_Door_Opening());
  EXPECT_EQ(loaded.Time_To_Redraw(), door.Time_To_Redraw());
  for (int frame = 0; frame < 10; ++frame) {
    loaded.AI();
    door.AI();
    EXPECT_EQ(loaded.Door_Stage(), door.Door_Stage());
    EXPECT_EQ(loaded.Is_Door_Open(), door.Is_Door_Open());
  }
  EXPECT_TRUE(loaded.Is_Door_Open());
}

TEST(TdSaveValuesTest, FlashStateSurvivesRoundTrip) {
  game::FlasherClass flash;
  flash.FlashCount = 127;
  flash.IsBlushing = true;
  game::FlasherClass loaded;
  Restore(loaded, Save(flash));
  EXPECT_EQ(loaded.FlashCount, 127);
  EXPECT_TRUE(loaded.IsBlushing);
  EXPECT_TRUE(loaded.Process());
  EXPECT_EQ(loaded.FlashCount, 126);
  EXPECT_FALSE(loaded.IsBlushing);
}

TEST(TdSaveValuesTest, FlashCountCannotOverflowItsBitfield) {
  std::array<uint8_t, 2> bytes{128, 0};
  SpanSource source(std::as_bytes(std::span(bytes).first(2)));
  ArchiveReader reader(source);
  game::FlasherClass flash;
  reader(flash);
  EXPECT_FALSE(reader.ok());
}

TEST(TdSaveValuesTest, FusePreservesArmingAndProximityState) {
  FuseClass fuse;
  fuse.Arm_Fuse(Coordinate{}, Coordinate::FromLeptons(0, 0x40), 12, 3);
  EXPECT_FALSE(fuse.Fuse_Checkup(Coordinate::FromLeptons(0, 0x10)));
  FuseClass loaded;
  const auto bytes = Save(fuse);
  Restore(loaded, bytes);
  EXPECT_EQ(Save(loaded), bytes);
  EXPECT_EQ(loaded.Fuse_Target(), fuse.Fuse_Target());
  for (int frame = 0; frame < 12; ++frame) {
    EXPECT_EQ(loaded.Fuse_Checkup(Coordinate::FromLeptons(0, 0x20)),
              fuse.Fuse_Checkup(Coordinate::FromLeptons(0, 0x20)));
    EXPECT_EQ(loaded.Timer, fuse.Timer);
  }
}

// Exercise the production TypePtr template without linking game type tables.
struct UnitRecord {
  UnitType Type;
  static const UnitRecord& As_Reference(UnitType type) {
    static const UnitRecord unit{UNIT_MCV};
    EXPECT_EQ(type, UNIT_MCV);
    return unit;
  }
};
struct HouseRecord {
  HousesType House;
  static const HouseRecord& As_Reference(HousesType house) {
    static const HouseRecord value{HOUSE_GOOD};
    EXPECT_EQ(house, HOUSE_GOOD);
    return value;
  }
};

TEST(TdSaveValuesTest, TypePointersResolveEnumsAndPreserveNull) {
  const UnitRecord* unit = &UnitRecord::As_Reference(UNIT_MCV);
  const UnitRecord* loaded = nullptr;
  TypePtr proxy(unit);
  TypePtr loaded_proxy(loaded);
  Restore(loaded_proxy, Save(proxy));
  EXPECT_EQ(loaded, unit);
  unit = nullptr;
  Restore(loaded_proxy, Save(proxy));
  EXPECT_EQ(loaded, nullptr);
  const HouseRecord* house = &HouseRecord::As_Reference(HOUSE_GOOD);
  const HouseRecord* loaded_house = nullptr;
  TypePtr house_proxy(house);
  TypePtr loaded_house_proxy(loaded_house);
  Restore(loaded_house_proxy, Save(house_proxy));
  EXPECT_EQ(loaded_house, house);
}

TEST(TdSaveValuesTest, TypePointersRejectInvalidIdsAndTruncation) {
  for (int32_t id : {-2, static_cast<int32_t>(UNIT_COUNT), INT32_MAX}) {
    const auto bytes = Save(id);
    const UnitRecord* value = nullptr;
    SpanSource source(std::as_bytes(std::span(bytes).first(4)));
    ArchiveReader reader(source);
    reader(TypePtr(value));
    EXPECT_FALSE(reader.ok());
    EXPECT_EQ(value, nullptr);
  }
  std::array<uint8_t, 3> bytes{};
  const UnitRecord* value = &UnitRecord::As_Reference(UNIT_MCV);
  SpanSource source(std::as_bytes(std::span(bytes).first(3)));
  ArchiveReader reader(source);
  reader(TypePtr(value));
  EXPECT_FALSE(reader.ok());
  EXPECT_EQ(value, nullptr);
}

}  // namespace

TEST(TdSaveValuesTest, TeamMissionPreservesOrderAndArgument) {
  TeamMissionStruct mission{TMISSION_MOVECELL, 4095};
  TeamMissionStruct loaded;
  Restore(loaded, Save(mission));
  EXPECT_EQ(loaded.Mission, TMISSION_MOVECELL);
  EXPECT_EQ(loaded.Argument, 4095);
}

TEST(TdSaveValuesTest, TeamMissionRejectsUnknownOrderAndTruncation) {
  TeamMissionStruct mission{TMISSION_COUNT, 1};
  auto bytes = Save(mission);
  SpanSource source(std::as_bytes(std::span(bytes)));
  ArchiveReader reader(source);
  reader(mission);
  EXPECT_FALSE(reader.ok());
  mission.Mission = TMISSION_GUARD;
  bytes = Save(mission);
  SpanSource truncated(std::as_bytes(std::span(bytes).first(7)));
  ArchiveReader short_reader(truncated);
  short_reader(mission);
  EXPECT_FALSE(short_reader.ok());
}

TEST(TdSaveValuesTest,
     AbstractStatePreservesCoordinatesAndRejectsInactiveSlots) {
  struct ActiveObject : AbstractClass {
    ActiveObject() { IsActive = true; }
  };
  ActiveObject value;
  value.Coord = Coordinate::FromPacked(0x12345678);
  ActiveObject loaded;
  Restore(loaded, Save(value));
  EXPECT_EQ(loaded.Coord, value.Coord);
  EXPECT_TRUE(loaded.IsActive);
  value.IsActive = false;
  auto bytes = Save(value);
  SpanSource source(std::as_bytes(std::span(bytes)));
  ArchiveReader reader(source);
  reader(loaded);
  EXPECT_FALSE(reader.ok());
}

TEST(TdSaveValuesTest, SuperweaponResumesPartialChargeAtRestoredFrame) {
  game::TheGameClock().set_frame(100);
  SuperClass weapon(100, VOX_ION_READY, VOX_ION_CHARGING);
  ASSERT_TRUE(weapon.Enable());
  game::TheGameClock().set_frame(125);
  const auto bytes = Save(weapon);
  game::TheGameClock().set_frame(1000);
  SuperClass loaded;
  Restore(loaded, bytes);
  EXPECT_TRUE(loaded.Is_Present());
  EXPECT_FALSE(loaded.Is_Ready());
  EXPECT_EQ(Save(loaded), bytes);
  game::TheGameClock().set_frame(game::CurrentFrame() + 74);
  loaded.AI();
  EXPECT_FALSE(loaded.Is_Ready());
  game::TheGameClock().Advance();
  loaded.AI();
  EXPECT_TRUE(loaded.Is_Ready());
}

TEST(TdSaveValuesTest, SuperweaponSuspensionPreservesRemainingCharge) {
  game::TheGameClock().set_frame(200);
  SuperClass weapon(80);
  ASSERT_TRUE(weapon.Enable());
  game::TheGameClock().set_frame(game::CurrentFrame() + 30);
  ASSERT_TRUE(weapon.Suspend(true));
  const auto bytes = Save(weapon);
  game::TheGameClock().set_frame(2000);
  SuperClass loaded;
  Restore(loaded, bytes);
  game::TheGameClock().set_frame(game::CurrentFrame() + 500);
  loaded.AI();
  EXPECT_FALSE(loaded.Is_Ready());
  ASSERT_TRUE(loaded.Suspend(false));
  game::TheGameClock().set_frame(game::CurrentFrame() + 49);
  loaded.AI();
  EXPECT_FALSE(loaded.Is_Ready());
  game::TheGameClock().Advance();
  loaded.AI();
  EXPECT_TRUE(loaded.Is_Ready());
}

TEST(TdSaveValuesTest, SuperweaponRejectsInvalidVoiceAndTruncation) {
  SuperClass weapon(100, VOX_COUNT);
  auto bytes = Save(weapon);
  SuperClass loaded;
  SpanSource source(std::as_bytes(std::span(bytes)));
  ArchiveReader reader(source);
  reader(loaded);
  EXPECT_FALSE(reader.ok());
  weapon = SuperClass(100);
  bytes = Save(weapon);
  SpanSource truncated(std::as_bytes(std::span(bytes).first(40)));
  ArchiveReader short_reader(truncated);
  short_reader(loaded);
  EXPECT_FALSE(short_reader.ok());
}

TEST(TdSaveValuesTest, RandomStreamsResumeAfterMixedDraws) {
  SeedGameRandom(12345);
  for (int i = 0; i < 37; ++i) {
    GameRandomRange(0, 1000);
    Random();
    Sim_Random();
  }
  auto state = CaptureRandomState();
  const auto bytes = Save(state);
  std::array<int, 90> expected{};
  for (size_t i = 0; i < 30; ++i) {
    expected.at(i * 3) = GameRandomRange(-30, 70);
    expected.at((i * 3) + 1) = Random();
    expected.at((i * 3) + 2) = Sim_Random();
  }
  SeedGameRandom(999);
  Restore(state, bytes);
  RestoreRandomState(state);
  for (size_t i = 0; i < 30; ++i) {
    EXPECT_EQ(GameRandomRange(-30, 70), expected.at(i * 3));
    EXPECT_EQ(Random(), expected.at((i * 3) + 1));
    EXPECT_EQ(Sim_Random(), expected.at((i * 3) + 2));
  }
}

TEST(TdSaveValuesTest, RandomStateRejectsInvalidIndexAndTruncation) {
  for (const int32_t index : {-1, 256}) {
    TdRandomState state{1, 2, index};
    auto bytes = Save(state);
    SpanSource source(std::as_bytes(std::span(bytes)));
    ArchiveReader reader(source);
    reader(state);
    EXPECT_FALSE(reader.ok());
  }
  TdRandomState state;
  auto bytes = Save(state);
  SpanSource source(std::as_bytes(std::span(bytes).first(11)));
  ArchiveReader reader(source);
  reader(state);
  EXPECT_FALSE(reader.ok());
}

TEST(TdSaveValuesTest, CellDefaultsAndResetNeedNoSparseRecord) {
  CellClass cell;
  EXPECT_FALSE(cell.Should_Save());
  cell.Owner = HOUSE_GOOD;
  cell.IsTrigger = true;
  cell.Flag.Composite = 255;
  cell.Reset();
  EXPECT_FALSE(cell.Should_Save());
  EXPECT_EQ(cell.Land_Type(), LAND_CLEAR);
}

TEST(TdSaveValuesTest, PreviouslyOmittedCellFieldsRequireSaving) {
  CellClass cell;
  const auto check = [&cell](auto change) {
    cell.Reset();
    change(cell);
    EXPECT_TRUE(cell.Should_Save());
  };
  check([](CellClass& value) { value.IsPlot = true; });
  check([](CellClass& value) { value.IsCursorHere = true; });
  check([](CellClass& value) { value.IsWaypoint = true; });
  check([](CellClass& value) { value.IsRadarCursor = true; });
  check([](CellClass& value) { value.IsFlagged = true; });
  check([](CellClass& value) { value.TIcon = 7; });
  check([](CellClass& value) { value.OverlayData = 3; });
  check([](CellClass& value) { value.SmudgeData = 2; });
  check([](CellClass& value) { value.Owner = HOUSE_GOOD; });
  check([](CellClass& value) { value.InfType = HOUSE_BAD; });
}

TEST(TdSaveValuesTest, ScorePreservesEveryCounterAndWideElapsedTime) {
  ScoreClass score{};
  score.Score = 101;
  score.NKilled = 2; score.GKilled = 3; score.CKilled = 4;
  score.NBKilled = 5; score.GBKilled = 6; score.CBKilled = 7;
  score.NHarvested = 8; score.GHarvested = 9; score.CHarvested = 10;
  score.ElapsedTime = (int64_t{1} * 1024 * 1024 * 1024 * 1024) + 11;
  const auto bytes = Save(score);
  ScoreClass restored{};
  Restore(restored, bytes);
  EXPECT_EQ(Save(restored), bytes);
  EXPECT_EQ(restored.ElapsedTime, score.ElapsedTime);
  SpanSource source(std::as_bytes(
      std::span(bytes).first(51)));  // SCOR + ten int32 + uint64 needs 52.
  ArchiveReader reader(source);
  restored.Serialize(reader);
  EXPECT_FALSE(reader.ok());
}

}  // namespace td
