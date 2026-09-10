#include <array>
#include <cstdint>

#include "gtest/gtest.h"
#include "td/abstract.h"
#include "td/audio.h"
#include "td/crew.h"
#include "td/door.h"
#include "td/facing.h"
#include "td/flasher.h"
#include "td/fly.h"
#include "td/ftimer.h"
#include "td/fuse.h"
#include "td/monoc.h"
#include "td/region.h"
#include "td/serialize.h"
#include "td/special.h"
#include "td/stage.h"
#include "td/super.h"
#include "td/teamtype.h"
#include "tech/archive.h"
#include "tech/xpipe.h"
#include "tech/xstraw.h"

// The value tests need a frame source, but no game session or debug display.
int64_t Frame = 0;
SpecialClass Special{};
void Speak(VoxType) {}
void MonoClass::Set_Cursor(int, int) {}
void MonoClass::Printf(const char*, ...) {}
// Fly's legacy pointer-coding hooks are no-ops in ioobj.cc, which otherwise
// needs the entire game. They are unrelated to the field-wise path under test.
void FlyClass::Code_Pointers() {}
void FlyClass::Decode_Pointers() {}

namespace {

template <class T>
std::array<uint8_t, 64> Save(T& value) {
  std::array<uint8_t, 64> bytes{};
  BufferPipe sink(bytes.data(), static_cast<int>(bytes.size()));
  ArchiveWriter writer(sink);
  writer(value);
  return bytes;
}

template <class T>
void Restore(T& value, const std::array<uint8_t, 64>& bytes) {
  BufferStraw source(bytes.data(), static_cast<int>(bytes.size()));
  ArchiveReader reader(source);
  reader(value);
  ASSERT_TRUE(reader.ok()) << reader.error();
}

TEST(TdSaveValuesTest, CountdownReanchorsAndKeepsWideRemainingTime) {
  Frame = 100;
  TCountDownTimerClass timer(int64_t{1} << 40);
  Frame += 17;
  const int64_t remaining = timer.Time();
  const auto bytes = Save(timer);
  Frame = 10000;
  TCountDownTimerClass loaded;
  Restore(loaded, bytes);
  EXPECT_TRUE(loaded.Active());
  EXPECT_EQ(loaded.Get_Start(), Frame);
  EXPECT_EQ(loaded.Time(), remaining);
  Frame += 9;
  EXPECT_EQ(loaded.Time(), remaining - 9);
}

TEST(TdSaveValuesTest, ClearedAndExpiredCountdownsRemainDistinct) {
  Frame = 100;
  TCountDownTimerClass timer(1);
  Frame = 102;
  TCountDownTimerClass loaded;
  Restore(loaded, Save(timer));
  EXPECT_TRUE(loaded.Active());
  EXPECT_EQ(loaded.Time(), 0);
  timer.Clear();
  Restore(loaded, Save(timer));
  EXPECT_FALSE(loaded.Active());
  EXPECT_EQ(loaded.Get_Start(), -1);
  EXPECT_EQ(loaded.Get_Delay(), 0);
}

TEST(TdSaveValuesTest, CountdownRejectsTruncationAndNegativeDuration) {
  TCountDownTimerClass timer;
  std::array<uint8_t, 9> bytes{};
  BufferStraw short_source(bytes.data(), 8);
  ArchiveReader short_reader(short_source);
  short_reader(timer);
  EXPECT_FALSE(short_reader.ok());
  bytes.fill(0xff);
  bytes.back() = 1;
  BufferStraw negative_source(bytes.data(), 9);
  ArchiveReader negative_reader(negative_source);
  negative_reader(timer);
  EXPECT_FALSE(negative_reader.ok());
}

TEST(TdSaveValuesTest, FacingPreservesCurrentAndDesiredSeparately) {
  FacingClass facing(DIR_N);
  std::array<uint8_t, 8> bytes{};
  BufferPipe sink(bytes.data(), 8);
  ArchiveWriter writer(sink);
  DirType current = DIR_E;
  DirType desired = DIR_SW;
  writer(current, desired);
  BufferStraw source(bytes.data(), 8);
  ArchiveReader reader(source);
  reader(facing);
  ASSERT_TRUE(reader.ok());
  EXPECT_EQ(facing.Current(), current);
  EXPECT_EQ(facing.Desired(), desired);
  FacingClass loaded(DIR_N);
  Restore(loaded, Save(facing));
  EXPECT_EQ(loaded.Current(), current);
  EXPECT_EQ(loaded.Desired(), desired);
}

TEST(TdSaveValuesTest, FlightPreservesFractionalDistanceAndSpeed) {
  std::array<uint8_t, 64> bytes{};
  BufferPipe sink(bytes.data(), static_cast<int>(bytes.size()));
  ArchiveWriter writer(sink);
  uint32_t accumulator = 511;
  MPHType speed = MPH_FAST;
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

TEST(TdSaveValuesTest, FlashStateAndCrewKillsSurviveRoundTrip) {
  FlasherClass flash;
  flash.FlashCount = 127;
  flash.IsBlushing = true;
  FlasherClass loaded;
  Restore(loaded, Save(flash));
  EXPECT_EQ(loaded.FlashCount, 127);
  EXPECT_TRUE(loaded.IsBlushing);
  EXPECT_TRUE(loaded.Process());
  EXPECT_EQ(loaded.FlashCount, 126);
  EXPECT_FALSE(loaded.IsBlushing);
  CrewClass crew;
  crew.Kills = 50000;
  CrewClass loaded_crew;
  Restore(loaded_crew, Save(crew));
  EXPECT_EQ(loaded_crew.Made_A_Kill(), 50001);
}

TEST(TdSaveValuesTest, FlashCountCannotOverflowItsBitfield) {
  std::array<uint8_t, 2> bytes{128, 0};
  BufferStraw source(bytes.data(), 2);
  ArchiveReader reader(source);
  FlasherClass flash;
  reader(flash);
  EXPECT_FALSE(reader.ok());
}

TEST(TdSaveValuesTest, FusePreservesArmingAndProximityState) {
  FuseClass fuse;
  fuse.Arm_Fuse(0, 0x00400000, 12, 3);
  EXPECT_FALSE(fuse.Fuse_Checkup(0x00100000));
  FuseClass loaded;
  const auto bytes = Save(fuse);
  Restore(loaded, bytes);
  EXPECT_EQ(Save(loaded), bytes);
  EXPECT_EQ(loaded.Fuse_Target(), fuse.Fuse_Target());
  for (int frame = 0; frame < 12; ++frame) {
    EXPECT_EQ(loaded.Fuse_Checkup(0x00200000), fuse.Fuse_Checkup(0x00200000));
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
    BufferStraw source(bytes.data(), 4);
    ArchiveReader reader(source);
    reader(TypePtr(value));
    EXPECT_FALSE(reader.ok());
    EXPECT_EQ(value, nullptr);
  }
  std::array<uint8_t, 3> bytes{};
  const UnitRecord* value = &UnitRecord::As_Reference(UNIT_MCV);
  BufferStraw source(bytes.data(), 3);
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
  BufferStraw source(bytes.data(), static_cast<int>(bytes.size()));
  ArchiveReader reader(source);
  reader(mission);
  EXPECT_FALSE(reader.ok());
  mission.Mission = TMISSION_GUARD;
  bytes = Save(mission);
  BufferStraw truncated(bytes.data(), 7);
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
  value.Coord = 0x12345678;
  ActiveObject loaded;
  Restore(loaded, Save(value));
  EXPECT_EQ(loaded.Coord, value.Coord);
  EXPECT_TRUE(loaded.IsActive);
  value.IsActive = false;
  auto bytes = Save(value);
  BufferStraw source(bytes.data(), static_cast<int>(bytes.size()));
  ArchiveReader reader(source);
  reader(loaded);
  EXPECT_FALSE(reader.ok());
}

TEST(TdSaveValuesTest, RegionPreservesWideAndNegativeThreat) {
  for (int64_t threat : {int64_t{1} << 40, int64_t{-17}}) {
    std::array<uint8_t, 64> bytes{};
    BufferPipe sink(bytes.data(), static_cast<int>(bytes.size()));
    ArchiveWriter writer(sink);
    writer(threat);
    RegionClass region;
    Restore(region, bytes);
    EXPECT_EQ(Save(region), bytes);
  }
}

TEST(TdSaveValuesTest, SuperweaponResumesPartialChargeAtRestoredFrame) {
  Frame = 100;
  SuperClass weapon(100, VOX_ION_READY, VOX_ION_CHARGING);
  ASSERT_TRUE(weapon.Enable());
  Frame = 125;
  auto bytes = Save(weapon);
  Frame = 1000;
  SuperClass loaded;
  Restore(loaded, bytes);
  EXPECT_TRUE(loaded.Is_Present());
  EXPECT_FALSE(loaded.Is_Ready());
  EXPECT_EQ(Save(loaded), bytes);
  Frame += 74;
  loaded.AI();
  EXPECT_FALSE(loaded.Is_Ready());
  ++Frame;
  loaded.AI();
  EXPECT_TRUE(loaded.Is_Ready());
}

TEST(TdSaveValuesTest, SuperweaponSuspensionPreservesRemainingCharge) {
  Frame = 200;
  SuperClass weapon(80);
  ASSERT_TRUE(weapon.Enable());
  Frame += 30;
  ASSERT_TRUE(weapon.Suspend(true));
  auto bytes = Save(weapon);
  Frame = 2000;
  SuperClass loaded;
  Restore(loaded, bytes);
  Frame += 500;
  loaded.AI();
  EXPECT_FALSE(loaded.Is_Ready());
  ASSERT_TRUE(loaded.Suspend(false));
  Frame += 49;
  loaded.AI();
  EXPECT_FALSE(loaded.Is_Ready());
  ++Frame;
  loaded.AI();
  EXPECT_TRUE(loaded.Is_Ready());
}

TEST(TdSaveValuesTest, SuperweaponRejectsInvalidVoiceAndTruncation) {
  SuperClass weapon(100, VOX_COUNT);
  auto bytes = Save(weapon);
  SuperClass loaded;
  BufferStraw source(bytes.data(), static_cast<int>(bytes.size()));
  ArchiveReader reader(source);
  reader(loaded);
  EXPECT_FALSE(reader.ok());
  weapon = SuperClass(100);
  bytes = Save(weapon);
  BufferStraw truncated(bytes.data(), 40);
  ArchiveReader short_reader(truncated);
  short_reader(loaded);
  EXPECT_FALSE(short_reader.ok());
}
