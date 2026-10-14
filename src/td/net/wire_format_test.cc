// Golden bytes for what Tiberian Dawn sends over the wire: one event per
// constructor and one packet per payload arm. The events and the packets go
// out as raw bytes, so any change to these strings is a protocol change.

#include <array>
#include <bit>
#include <cstddef>
#include <span>
#include <string>

#include "absl/strings/str_format.h"
#include "base/buffer.h"
#include "base/pooled.h"
#include "base/strings/safe_string.h"
#include "gtest/gtest.h"
#include "magic_enum/magic_enum.hpp"
#include "td/core/cell.h"
#include "td/core/coordinate.h"
#include "td/core/defines.h"
#include "td/core/special_flags.h"
#include "td/core/target.h"
#include "td/map/world.h"
#include "td/net/event.h"
#include "td/objects/house.h"
#include "td/startup/game_test_util.h"

namespace td {
namespace {

std::string Hex(const std::span<const std::byte> bytes) {
  std::string out;
  for (const std::byte b : bytes) {
    absl::StrAppendFormat(&out, "%02x", std::to_integer<int>(b));
  }
  return out;
}

// Returns a T whose every byte, padding included, is zero.
template <class T>
T Zeroed() {
  return std::bit_cast<T>(std::array<std::byte, sizeof(T)>{});
}

// Every special flag on.
SpecialFlags AllSpecialFlags() {
  SpecialFlags flags{};
  flags.enemy_health_bars = 1;
  flags.easy = 1;
  flags.difficult = 1;
  flags.speed_build = 1;
  flags.separate_helipad = 1;
  flags.defender_advantage = 1;
  flags.named_civilians = 1;
  flags.from_install = 1;
  flags.capture_the_flag = 1;
  flags.visible_target = 1;
  flags.juvenile_sounds = 1;
  flags.smart_defense = 1;
  flags.tree_target = 1;
  flags.mcv_deploy = 1;
  flags.retired_mono_flag = 1;
  flags.inert_weapons = 1;
  flags.show_find_path = 1;
  flags.all_health_bars = 1;
  flags.three_point_turns = 1;
  flags.tiberium_grows = 1;
  flags.tiberium_spreads = 1;
  flags.tiberium_fast = 1;
  flags.no_road_pieces = 1;
  flags.scatter = 1;
  flags.jurassic = 1;
  flags.score_variations = 1;
  flags.squish_marks = 1;
  flags.no_scroll_over_tabs = 1;
  flags.from_wchat = 1;
  return flags;
}

constexpr Target kWhom = Target::FromPacked(0x1005);
constexpr Target kTarget = Target::FromPacked(0x2007);
constexpr Target kDestination = Target::FromPacked(0x3009);
constexpr Cell kCell = Cell::FromIndex(0x0234);
constexpr Coordinate kWhere = Coordinate::FromPacked(0x00120034);

class WireFormatTest : public GameFixture {
 protected:
  WireFormatTest() { ThePlayer() = base::NewPooled<HouseClass>(HOUSE_GOOD); }

  // The bytes of `event` with the sender and the frame cleared, which the
  // constructors take from the game.
  static std::string EventHex(EventClass event) {
    event.ID = 0;
    event.MPlayerID = 0;
    event.Frame = 0;
    return Hex(base::ObjectBytes(event));
  }
};

TEST_F(WireFormatTest, SpecialEvent) {
  EXPECT_EQ(EventHex(EventClass(AllSpecialFlags())),
            "130000000000000000000000ffffff1f0000000000000000");
}

TEST_F(WireFormatTest, TargetEvent) {
  EXPECT_EQ(EventHex(EventClass(EventClass::SELL, kWhom)),
            "120000000000000000000000051000000000000000000000");
}

TEST_F(WireFormatTest, TypeOnlyEvent) {
  EXPECT_EQ(EventHex(EventClass(EventClass::IDLE)),
            "030000000000000000000000000000000000000000000000");
}

TEST_F(WireFormatTest, ValueEvent) {
  EXPECT_EQ(EventHex(EventClass(EventClass::GAMESPEED, 0x0A0B0C0D)),
            "0900000000000000000000000d0c0b0a0000000000000000");
}

TEST_F(WireFormatTest, MegaMissionEvent) {
  EXPECT_EQ(EventHex(EventClass(kWhom, MISSION_ATTACK, kTarget, kDestination)),
            "020000000000000000000000051000000100000007200930");
}

TEST_F(WireFormatTest, SpecificEvent) {
  EXPECT_EQ(
      EventHex(EventClass(EventClass::PRODUCE, RTTI_INFANTRYTYPE, 0x0506)),
      "0a0000000000000000000000020000000605000000000000");
}

TEST_F(WireFormatTest, PlaceEvent) {
  EXPECT_EQ(EventHex(EventClass(EventClass::PLACE, RTTI_BUILDINGTYPE, kCell)),
            "070000000000000000000000080000003402000000000000");
}

TEST_F(WireFormatTest, SpecialPlaceEvent) {
  EXPECT_EQ(EventHex(EventClass(EventClass::SPECIAL_PLACE, 0x0708, kCell)),
            "0e0000000000000000000000080700003402000000000000");
}

TEST_F(WireFormatTest, AnimationEvent) {
  EXPECT_EQ(EventHex(EventClass(static_cast<AnimType>(3), HOUSE_BAD, kWhere)),
            "100000000000000000000000030000000100000034001200");
}

TEST_F(WireFormatTest, FrameInfoEvent) {
  EventClass event;
  event.Type = EventClass::FRAMEINFO;
  event.UpdateData<EventClass::FrameInfoData>([](EventClass::FrameInfoData& d) {
    d.CRC = 0x11223344;
    d.CommandCount = 0x5566;
    d.Delay = 0x77;
  });
  EXPECT_EQ(EventHex(event),
            "170000000000000000000000443322116655770000000000");
}

TEST_F(WireFormatTest, TimingEvent) {
  EventClass event;
  event.Type = EventClass::TIMING;
  event.UpdateData<EventClass::TimingData>([](EventClass::TimingData& d) {
    d.DesiredFrameRate = 0x1122;
    d.MaxAhead = 0x3344;
  });
  EXPECT_EQ(EventHex(event),
            "180000000000000000000000221144330000000000000000");
}

TEST_F(WireFormatTest, ProcessTimeEvent) {
  EventClass event;
  event.Type = EventClass::PROCESS_TIME;
  event.UpdateData<EventClass::ProcessTimeData>(
      [](EventClass::ProcessTimeData& d) { d.AverageTicks = 0x5566; });
  EXPECT_EQ(EventHex(event),
            "190000000000000000000000665500000000000000000000");
}

// Writing a narrower arm over a wider one leaves the wider arm's tail, as a
// store into a union member does.
TEST_F(WireFormatTest, ASecondArmKeepsTheFirstArmsTail) {
  EventClass event(kWhom, MISSION_ATTACK, kTarget, kDestination);
  event.UpdateData<EventClass::MissionData>([](EventClass::MissionData& d) {
    d.Whom = kDestination;
    d.Mission = MISSION_GUARD;
  });
  EXPECT_EQ(EventHex(event),
            "020000000000000000000000093000000400000007200930");
}

TEST_F(WireFormatTest, GameInfoPacket) {
  auto packet = Zeroed<GlobalPacketType>();
  packet.Command = NET_ANSWER_GAME;
  base::SafeCopy(packet.Name, "Game");
  packet.UpdateData<GlobalPacketType::GameInfoData>(
      [](GlobalPacketType::GameInfoData& d) {
        d.Version = 0x11223344;
        d.IsOpen = 1;
      });
  EXPECT_EQ(Hex(base::ObjectBytes(packet)),
            "0100000047616d6500000000000000004433221101000000000000000000000000"
            "00000000000000000000000000000000000000");
}

TEST_F(WireFormatTest, PlayerInfoPacket) {
  auto packet = Zeroed<GlobalPacketType>();
  packet.Command = NET_ANSWER_PLAYER;
  base::SafeCopy(packet.Name, "Player");
  packet.UpdateData<GlobalPacketType::PlayerInfoData>(
      [](GlobalPacketType::PlayerInfoData& d) {
        d.House = HOUSE_BAD;
        d.Color = 0x05;
        d.NameCRC = 0x11223344;
      });
  EXPECT_EQ(Hex(base::ObjectBytes(packet)),
            "03000000506c617965720000000000000100000005000000443322110000000000"
            "00000000000000000000000000000000000000");
}

TEST_F(WireFormatTest, ScenarioInfoPacket) {
  auto packet = Zeroed<GlobalPacketType>();
  packet.Command = NET_GAME_OPTIONS;
  base::SafeCopy(packet.Name, "Host");
  packet.UpdateData<GlobalPacketType::ScenarioInfoData>(
      [](GlobalPacketType::ScenarioInfoData& d) {
        d.Scenario = 0x2A;
        d.Credits = 0x00001388;
        d.IsBases = 1;
        d.IsTiberium = 1;
        d.IsGoodies = 1;
        d.IsGhosties = 1;
        d.BuildLevel = 0x0A;
        d.UnitCount = 0x0B;
        d.Seed = 0x11223344;
        d.Special = AllSpecialFlags();
        d.GameSpeed = 0x05;
      });
  EXPECT_EQ(Hex(base::ObjectBytes(packet)),
            "07000000486f737400000000000000002a000000881300000f0a0b0044332211ff"
            "ffff1f05000000000000000000000000000000");
}

TEST_F(WireFormatTest, MessagePacket) {
  auto packet = Zeroed<GlobalPacketType>();
  packet.Command = NET_MESSAGE;
  base::SafeCopy(packet.Name, "Sender");
  packet.UpdateData<GlobalPacketType::MessageData>(
      [](GlobalPacketType::MessageData& d) {
        base::SafeCopy(d.Buf, "Hello");
        d.ID = 0x42;
        d.NameCRC = 0x11223344;
      });
  EXPECT_EQ(Hex(base::ObjectBytes(packet)),
            "0a00000053656e64657200000000000048656c6c6f000000000000000000000000"
            "00000000000000000000004200000044332211");
}

TEST_F(WireFormatTest, ResponseTimePacket) {
  auto packet = Zeroed<GlobalPacketType>();
  packet.Command = NET_GO;
  packet.UpdateData<GlobalPacketType::ResponseTimeData>(
      [](GlobalPacketType::ResponseTimeData& d) { d.OneWay = 0x11223344; });
  EXPECT_EQ(Hex(base::ObjectBytes(packet)),
            "090000000000000000000000000000004433221100000000000000000000000000"
            "00000000000000000000000000000000000000");
}

// True if some payload arm names `type` in its Carries().
bool AnArmCarries(const EventClass::EventType type) {
  using E = EventClass;
  return E::OptionsData::Carries(type) || E::TargetData::Carries(type) ||
         E::AnimData::Carries(type) || E::GeneralData::Carries(type) ||
         E::MegaMissionData::Carries(type) || E::MissionData::Carries(type) ||
         E::SpecificData::Carries(type) || E::PlaceData::Carries(type) ||
         E::SpecialData::Carries(type) || E::FrameInfoData::Carries(type) ||
         E::TimingData::Carries(type) || E::ProcessTimeData::Carries(type);
}

TEST(EventArmsTest, EveryTypeWithALengthCarriesAnArm) {
  for (const auto type : magic_enum::enum_values<EventClass::EventType>()) {
    if (type == EventClass::LAST_EVENT) {
      continue;
    }
    if (EventClass::EventLength.at(type) != 0) {
      EXPECT_TRUE(AnArmCarries(type)) << magic_enum::enum_name(type);
    }
  }
}

// The packet code copies an event's header and its payload separately, at
// kPayloadOffset.
TEST(EventArmsTest, ThePayloadStartsAtItsOffset) {
  EventClass event;
  event.Type = EventClass::GAMESPEED;
  event.UpdateData<EventClass::GeneralData>(
      [](EventClass::GeneralData& d) { d.Value = 0x5A; });
  EXPECT_EQ(
      base::ObjectBytes(event).subspan(EventClass::kPayloadOffset).front(),
      std::byte{0x5A});
  EXPECT_EQ(event.payload_bytes().front(), std::byte{0x5A});
}

}  // namespace
}  // namespace td
