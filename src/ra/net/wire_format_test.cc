// Golden bytes for what Red Alert sends over the wire: one event per
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
#include "ra/core/cell.h"
#include "ra/core/coordinate.h"
#include "ra/core/defines.h"
#include "ra/core/special_flags.h"
#include "ra/core/target.h"
#include "ra/map/world.h"
#include "ra/net/event.h"
#include "ra/net/session.h"
#include "ra/objects/house.h"
#include "ra/startup/game_test_util.h"

namespace ra {
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
  flags.shadow_grows = 1;
  flags.speed_build = 1;
  flags.from_install = 1;
  flags.capture_the_flag = 1;
  flags.inert_weapons = 1;
  flags.three_point_turns = 1;
  flags.ore_grows = 1;
  flags.ore_spreads = 1;
  return flags;
}

constexpr Target kWhom = Target::FromPacked(0x01000005);
constexpr Target kTarget = Target::FromPacked(0x02000007);
constexpr Target kDestination = Target::FromPacked(0x03000009);
constexpr Cell kCell = Cell::FromIndex(0x1234);
constexpr Coordinate kWhere = Coordinate::FromPacked(0x00120034);

class WireFormatTest : public GameFixture {
 protected:
  WireFormatTest() { ThePlayer() = base::NewPooled<HouseClass>(HOUSE_GREECE); }

  // The bytes of `event` with the sender and the frame cleared, which the
  // constructors take from the game.
  static std::string EventHex(EventClass event) {
    event.ID = 0;
    event.Frame = 0;
    return Hex(base::ObjectBytes(event));
  }
};

TEST_F(WireFormatTest, SpecialEvent) {
  EXPECT_EQ(EventHex(EventClass(AllSpecialFlags())),
            "1500000000000000ff0000000000000000000000000000000000000000000000");
}

TEST_F(WireFormatTest, TargetEvent) {
  EXPECT_EQ(EventHex(EventClass(EventClass::SELL, kWhom)),
            "1300000000000000050000010000000000000000000000000000000000000000");
}

TEST_F(WireFormatTest, TypeOnlyEvent) {
  EXPECT_EQ(EventHex(EventClass(EventClass::IDLE)),
            "0400000000000000000000000000000000000000000000000000000000000000");
}

TEST_F(WireFormatTest, ValueEvent) {
  EXPECT_EQ(EventHex(EventClass(EventClass::GAMESPEED, 0x0A0B0C0D)),
            "0a000000000000000d0c0b0a0000000000000000000000000000000000000000");
}

TEST_F(WireFormatTest, SellCellEvent) {
  EXPECT_EQ(EventHex(EventClass(EventClass::SELLCELL, kCell)),
            "1400000000000000341200000000000000000000000000000000000000000000");
}

TEST_F(WireFormatTest, NavComEvent) {
  EXPECT_EQ(EventHex(EventClass(EventClass::ARCHIVE, kWhom, kTarget)),
            "1b00000000000000050000010700000200000000000000000000000000000000");
}

TEST_F(WireFormatTest, MegaMissionEvent) {
  EXPECT_EQ(EventHex(EventClass(kWhom, MISSION_ATTACK, kTarget, kDestination)),
            "0200000000000000050000010100000007000002090000030000000000000000");
}

TEST_F(WireFormatTest, MegaMissionFormationEvent) {
  EXPECT_EQ(EventHex(EventClass(kWhom, MISSION_ATTACK, kTarget, kDestination,
                                SPEED_TRACK, 0x0102)),
            "0300000000000000050000010100000007000002090000030100000002010000");
}

TEST_F(WireFormatTest, SpecificEvent) {
  EXPECT_EQ(
      EventHex(EventClass(EventClass::PRODUCE, RTTI_INFANTRYTYPE, 0x0506)),
      "0b000000000000000e0000000605000000000000000000000000000000000000");
}

TEST_F(WireFormatTest, PlaceEvent) {
  EXPECT_EQ(EventHex(EventClass(EventClass::PLACE, RTTI_BUILDINGTYPE, kCell)),
            "0800000000000000060000003412000000000000000000000000000000000000");
}

TEST_F(WireFormatTest, SpecialPlaceEvent) {
  EXPECT_EQ(
      EventHex(EventClass(EventClass::SPECIAL_PLACE, 0x0708, kCell, kWhere)),
      "0f00000000000000080700003412000034001200000000000000000000000000");
}

TEST_F(WireFormatTest, AnimationEvent) {
  EXPECT_EQ(EventHex(EventClass(static_cast<AnimType>(3), HOUSE_USSR, kWhere)),
            "1100000000000000030000000200000034001200000000000000000000000000");
}

// The payload pointer is this process's own; only its size is pinned.
TEST_F(WireFormatTest, VariableEvent) {
  std::array<std::byte, 0x0123> payload{};
  EventClass event(EventClass::ADDPLAYER, payload);
  event.UpdateData<EventClass::VariableData>(
      [](EventClass::VariableData& d) { d.Pointer = nullptr; });
  EXPECT_EQ(EventHex(event),
            "1c00000000000000000000000000000023010000000000000000000000000000");
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
            "1900000000000000443322116655770000000000000000000000000000000000");
}

TEST_F(WireFormatTest, TimingEvent) {
  EventClass event;
  event.Type = EventClass::TIMING;
  event.UpdateData<EventClass::TimingData>([](EventClass::TimingData& d) {
    d.DesiredFrameRate = 0x1122;
    d.MaxAhead = 0x3344;
  });
  EXPECT_EQ(EventHex(event),
            "1d00000000000000221144330000000000000000000000000000000000000000");
}

TEST_F(WireFormatTest, ProcessTimeEvent) {
  EventClass event;
  event.Type = EventClass::PROCESS_TIME;
  event.UpdateData<EventClass::ProcessTimeData>(
      [](EventClass::ProcessTimeData& d) { d.AverageTicks = 0x5566; });
  EXPECT_EQ(EventHex(event),
            "1e00000000000000665500000000000000000000000000000000000000000000");
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
            "0200000000000000090000030500000007000002090000030000000000000000");
}

TEST_F(WireFormatTest, GlobalGameInfoPacket) {
  auto packet = Zeroed<GlobalPacketType>();
  packet.Command = NET_ANSWER_GAME;
  base::SafeCopy(packet.Name, "Game");
  packet.GameInfo.IsOpen = 1;
  EXPECT_EQ(Hex(base::ObjectBytes(packet)),
            "010047616d65000000000000000000000100000000000000000000000000000000"
            "000000000000000000000000000000000000000000000000000000000000000000"
            "000000000000000000000000000000000000000000000000000000000000000000"
            "000000000000000000000000000000000000000000000000000000000000000000"
            "000000000000000000000000");
}

TEST_F(WireFormatTest, GlobalPlayerInfoPacket) {
  auto packet = Zeroed<GlobalPacketType>();
  packet.Command = NET_ANSWER_PLAYER;
  base::SafeCopy(packet.Name, "Player");
  packet.PlayerInfo.House = HOUSE_USSR;
  packet.PlayerInfo.Color = PCOLOR_GREEN;
  packet.PlayerInfo.NameCRC = 0x11223344;
  packet.PlayerInfo.MinVersion = 0x55667788;
  packet.PlayerInfo.MaxVersion = 0x99AABBCC;
  packet.PlayerInfo.CheatCheck = 0x0D0E0F10;
  EXPECT_EQ(Hex(base::ObjectBytes(packet)),
            "0300506c61796572000000000000000002000000030000004433221188776655cc"
            "bbaa99100f0e0d0000000000000000000000000000000000000000000000000000"
            "000000000000000000000000000000000000000000000000000000000000000000"
            "000000000000000000000000000000000000000000000000000000000000000000"
            "000000000000000000000000");
}

TEST_F(WireFormatTest, GlobalScenarioInfoPacket) {
  auto packet = Zeroed<GlobalPacketType>();
  packet.Command = NET_GAME_OPTIONS;
  base::SafeCopy(packet.Name, "Host");
  base::SafeCopy(packet.ScenarioInfo.Scenario, "Scenario");
  packet.ScenarioInfo.Credits = 0x00001388;
  packet.ScenarioInfo.IsBases = 1;
  packet.ScenarioInfo.IsTiberium = 1;
  packet.ScenarioInfo.IsGoodies = 1;
  packet.ScenarioInfo.IsGhosties = 1;
  packet.ScenarioInfo.OfficialScenario = 1;
  packet.ScenarioInfo.BuildLevel = 0x0A;
  packet.ScenarioInfo.UnitCount = 0x0B;
  packet.ScenarioInfo.AIPlayers = 0x0C;
  packet.ScenarioInfo.Seed = 0x11223344;
  packet.ScenarioInfo.Special = AllSpecialFlags();
  packet.ScenarioInfo.GameSpeed = 0x05;
  packet.ScenarioInfo.Version = 0x55667788;
  packet.ScenarioInfo.FileLength = 0x99AABBCC;
  base::SafeCopy(packet.ScenarioInfo.ShortFileName, "SCM01EA.INI");
  base::SafeCopy(packet.ScenarioInfo.FileDigest, "digest");
  EXPECT_EQ(Hex(base::ObjectBytes(packet)),
            "0900486f7374000000000000000000005363656e6172696f000000000000000000"
            "000000000000000000000000000000000000000000000000000000881300001f0a"
            "0b0c44332211ff0000000500000088776655ccbbaa9953434d303145412e494e49"
            "000064696765737400000000000000000000000000000000000000000000000000"
            "000000000000000000000000");
}

TEST_F(WireFormatTest, GlobalMessagePacket) {
  auto packet = Zeroed<GlobalPacketType>();
  packet.Command = NET_MESSAGE;
  base::SafeCopy(packet.Name, "Sender");
  base::SafeCopy(packet.Message.Buf, "Hello");
  packet.Message.Color = PCOLOR_RED;
  packet.Message.NameCRC = 0x11223344;
  EXPECT_EQ(Hex(base::ObjectBytes(packet)),
            "0c0053656e646572000000000000000048656c6c6f000000000000000000000000"
            "000000000000000000000000000000000000000000000000000000000000000000"
            "000000000000000000000000000000000000000000000000000000000000000000"
            "000000000000000000000000000000000000000000000000000000000000000000"
            "000000000200000044332211");
}

TEST_F(WireFormatTest, GlobalResponseTimePacket) {
  auto packet = Zeroed<GlobalPacketType>();
  packet.Command = NET_GO;
  packet.ResponseTime.OneWay = 0x11223344;
  EXPECT_EQ(Hex(base::ObjectBytes(packet)),
            "0b0000000000000000000000000000004433221100000000000000000000000000"
            "000000000000000000000000000000000000000000000000000000000000000000"
            "000000000000000000000000000000000000000000000000000000000000000000"
            "000000000000000000000000000000000000000000000000000000000000000000"
            "000000000000000000000000");
}

TEST_F(WireFormatTest, GlobalRejectPacket) {
  auto packet = Zeroed<GlobalPacketType>();
  packet.Command = NET_REJECT_JOIN;
  packet.Reject.Why = 0x11223344;
  EXPECT_EQ(Hex(base::ObjectBytes(packet)),
            "080000000000000000000000000000004433221100000000000000000000000000"
            "000000000000000000000000000000000000000000000000000000000000000000"
            "000000000000000000000000000000000000000000000000000000000000000000"
            "000000000000000000000000000000000000000000000000000000000000000000"
            "000000000000000000000000");
}

TEST_F(WireFormatTest, GlobalChatPacket) {
  auto packet = Zeroed<GlobalPacketType>();
  packet.Command = NET_CHAT_ANNOUNCE;
  packet.Chat.ID = 0x11223344;
  packet.Chat.Color = PCOLOR_LTBLUE;
  EXPECT_EQ(Hex(base::ObjectBytes(packet)),
            "040000000000000000000000000000004433221101000000000000000000000000"
            "000000000000000000000000000000000000000000000000000000000000000000"
            "000000000000000000000000000000000000000000000000000000000000000000"
            "000000000000000000000000000000000000000000000000000000000000000000"
            "000000000000000000000000");
}

TEST_F(WireFormatTest, SerialScenarioInfoPacket) {
  auto packet = Zeroed<SerialPacketType>();
  packet.Command = SERIAL_GAME_OPTIONS;
  base::SafeCopy(packet.Name, "Host");
  packet.ID = 0x42;
  packet.ScenarioInfo.House = HOUSE_USSR;
  packet.ScenarioInfo.Color = PCOLOR_GREEN;
  packet.ScenarioInfo.MinVersion = 0x11223344;
  packet.ScenarioInfo.MaxVersion = 0x55667788;
  base::SafeCopy(packet.ScenarioInfo.Scenario, "Scenario");
  packet.ScenarioInfo.Credits = 0x00001388;
  packet.ScenarioInfo.IsBases = 1;
  packet.ScenarioInfo.IsTiberium = 1;
  packet.ScenarioInfo.IsGoodies = 1;
  packet.ScenarioInfo.IsGhosties = 1;
  packet.ScenarioInfo.OfficialScenario = 1;
  packet.ScenarioInfo.CheatCheck = 0x0D0E0F10;
  packet.ScenarioInfo.BuildLevel = 0x0A;
  packet.ScenarioInfo.UnitCount = 0x0B;
  packet.ScenarioInfo.AIPlayers = 0x0C;
  packet.ScenarioInfo.Seed = 0x21222324;
  packet.ScenarioInfo.Special = AllSpecialFlags();
  packet.ScenarioInfo.GameSpeed = 0x05;
  packet.ScenarioInfo.ResponseTime = 0x31323334;
  packet.ScenarioInfo.FileLength = 0x41424344;
  base::SafeCopy(packet.ScenarioInfo.ShortFileName, "SCM01EA.INI");
  base::SafeCopy(packet.ScenarioInfo.FileDigest, "digest");
  EXPECT_EQ(Hex(base::ObjectBytes(packet)),
            "6500486f7374000000000000000042000200000003000000443322118877665553"
            "63656e6172696f0000000000000000000000000000000000000000000000000000"
            "00000000000000000000881300001f000000100f0e0d0a0b0c0024232221ff0000"
            "0005000000343332314443424153434d303145412e494e49000064696765737400"
            "00000000000000000000000000000000000000000000000000000000");
}

TEST_F(WireFormatTest, SerialMessagePacket) {
  auto packet = Zeroed<SerialPacketType>();
  packet.Command = SERIAL_MESSAGE;
  base::SafeCopy(packet.Name, "Sender");
  packet.ID = 0x42;
  base::SafeCopy(packet.Message.Message, "Hello");
  packet.Message.Color = PCOLOR_RED;
  EXPECT_EQ(Hex(base::ObjectBytes(packet)),
            "680053656e646572000000000000420048656c6c6f000000000000000000000000"
            "000000000000000000000000000000000000000000000000000000000000000000"
            "000000000000000000000000000000000000000000000000000000000000000000"
            "000000000000000000000000000000000000000000000000000000000000000000"
            "00000000020000000000000000000000000000000000000000000000");
}

TEST_F(WireFormatTest, SerialChatPacket) {
  auto packet = Zeroed<SerialPacketType>();
  packet.Command = SERIAL_CONNECT;
  base::SafeCopy(packet.Name, "Caller");
  packet.ID = 0x42;
  packet.Chat.Color = PCOLOR_LTBLUE;
  EXPECT_EQ(Hex(base::ObjectBytes(packet)),
            "640043616c6c657200000000000042000100000000000000000000000000000000"
            "000000000000000000000000000000000000000000000000000000000000000000"
            "000000000000000000000000000000000000000000000000000000000000000000"
            "000000000000000000000000000000000000000000000000000000000000000000"
            "00000000000000000000000000000000000000000000000000000000");
}

// True if some payload arm names `type` in its Carries().
bool AnArmCarries(const EventClass::EventType type) {
  using E = EventClass;
  return E::OptionsData::Carries(type) || E::SellCellData::Carries(type) ||
         E::TargetData::Carries(type) || E::AnimData::Carries(type) ||
         E::GeneralData::Carries(type) || E::MegaMissionData::Carries(type) ||
         E::MegaMissionFormationData::Carries(type) ||
         E::MissionData::Carries(type) || E::NavComData::Carries(type) ||
         E::SpecificData::Carries(type) || E::PlaceData::Carries(type) ||
         E::SpecialData::Carries(type) || E::FrameInfoData::Carries(type) ||
         E::VariableData::Carries(type) || E::TimingData::Carries(type) ||
         E::ProcessTimeData::Carries(type);
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
}  // namespace ra
