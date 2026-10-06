// Field lists for Red Alert's scenario and other non-heap save state.

#include <cstdint>

#include "base/random.h"
#include "engine/platform/timer.h"
#include "engine/stream/archive.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/ai/scenario.h"
#include "ra/core/special_flags.h"
#include "ra/data/options.h"
#include "ra/net/session.h"
#include "ra/screens/score.h"

namespace ra {

template <class Archive>
void ScenarioClass::Serialize(Archive& ar) {
  uint32_t seed = sync_rng_.seed();
  ar(seed, Difficulty, CDifficulty, ElapsedTime, Waypoint, MissionTimer,
     ShroudTimer, Scenario, Theater, ScenarioName, Description, IntroMovie,
     BriefMovie, WinMovie, LoseMovie, ActionMovie, BriefingText, TransitTheme,
     PlayerHouse, CarryOverPercent, CarryOverMoney, CarryOverCap, Percent,
     GlobalFlags, Views, BridgeCount, CarryOverTimer);
  uint16_t flags = 0;
  flags |= static_cast<uint16_t>(IsBridgeChanged << 0);
  flags |= static_cast<uint16_t>(IsGlobalChanged << 1);
  flags |= static_cast<uint16_t>(IsToCarryOver << 2);
  flags |= static_cast<uint16_t>(IsToInherit << 3);
  flags |= static_cast<uint16_t>(IsTanyaEvac << 4);
  flags |= static_cast<uint16_t>(IsEndOfGame << 5);
  flags |= static_cast<uint16_t>(IsInheritTimer << 6);
  flags |= static_cast<uint16_t>(IsNoSpyPlane << 7);
  flags |= static_cast<uint16_t>(IsSkipScore << 8);
  flags |= static_cast<uint16_t>(IsOneTimeOnly << 9);
  flags |= static_cast<uint16_t>(IsNoMapSel << 10);
  flags |= static_cast<uint16_t>(IsTruckCrate << 11);
  flags |= static_cast<uint16_t>(IsMoneyTiberium << 12);
  ar(flags);
  if constexpr (Archive::kIsReading) {
    sync_rng_.set_seed(seed);
    IsBridgeChanged = (flags & (1U << 0)) != 0;
    IsGlobalChanged = (flags & (1U << 1)) != 0;
    IsToCarryOver = (flags & (1U << 2)) != 0;
    IsToInherit = (flags & (1U << 3)) != 0;
    IsTanyaEvac = (flags & (1U << 4)) != 0;
    IsEndOfGame = (flags & (1U << 5)) != 0;
    IsInheritTimer = (flags & (1U << 6)) != 0;
    IsNoSpyPlane = (flags & (1U << 7)) != 0;
    IsSkipScore = (flags & (1U << 8)) != 0;
    IsOneTimeOnly = (flags & (1U << 9)) != 0;
    IsNoMapSel = (flags & (1U << 10)) != 0;
    IsTruckCrate = (flags & (1U << 11)) != 0;
    IsMoneyTiberium = (flags & (1U << 12)) != 0;
    IsFadingBW = IsFadingColor = false;
    FadeTimer.Set(0);
    AutoSonarTimer.Set(0);
    bLocalProposesDraw = bOtherProposesDraw = false;
    ScenarioName[sizeof(ScenarioName) - 1] = '\0';
    Description[sizeof(Description) - 1] = '\0';
    BriefingText[sizeof(BriefingText) - 1] = '\0';
    if (!magic_enum::enum_contains(Theater)) {
      ar.Fail("invalid scenario theater");
    }
  }
}
template void ScenarioClass::Serialize(ArchiveWriter&);
template void ScenarioClass::Serialize(ArchiveReader&);

template <class Archive>
void ScoreClass::Serialize(Archive& ar) {
  ar(Score, NKilled, GKilled, CKilled, NBKilled, GBKilled, CBKilled,
     NHarvested, GHarvested, CHarvested, ElapsedTime, RealTime);
}
template void ScoreClass::Serialize(ArchiveWriter&);
template void ScoreClass::Serialize(ArchiveReader&);

template <class Archive>
void SpecialFlags::Serialize(Archive& ar) {
  uint8_t flags = 0;
  flags |= static_cast<uint8_t>(shadow_grows << 0);
  flags |= static_cast<uint8_t>(speed_build << 1);
  flags |= static_cast<uint8_t>(from_install << 2);
  flags |= static_cast<uint8_t>(capture_the_flag << 3);
  flags |= static_cast<uint8_t>(inert_weapons << 4);
  flags |= static_cast<uint8_t>(three_point_turns << 5);
  flags |= static_cast<uint8_t>(ore_grows << 6);
  flags |= static_cast<uint8_t>(ore_spreads << 7);
  ar(flags);
  if constexpr (Archive::kIsReading) {
    shadow_grows = (flags & (1U << 0)) != 0;
    speed_build = (flags & (1U << 1)) != 0;
    from_install = (flags & (1U << 2)) != 0;
    capture_the_flag = (flags & (1U << 3)) != 0;
    inert_weapons = (flags & (1U << 4)) != 0;
    three_point_turns = (flags & (1U << 5)) != 0;
    ore_grows = (flags & (1U << 6)) != 0;
    ore_spreads = (flags & (1U << 7)) != 0;
  }
}
template void SpecialFlags::Serialize(ArchiveWriter&);
template void SpecialFlags::Serialize(ArchiveReader&);

template <class Archive>
void OptionsClass::Serialize(Archive& ar) {
  // Key bindings, volume and display settings belong to the local player.
  ar(GameSpeed);
}
template void OptionsClass::Serialize(ArchiveWriter&);
template void OptionsClass::Serialize(ArchiveReader&);

template <class Archive>
void NodeNameTag::Serialize(Archive& ar) {
  if constexpr (Archive::kIsReading) {
    Player = {};
  }
  ar(Name, Address, Player.House, Player.Color, Player.ID, Player.ProcessTime);
  if constexpr (Archive::kIsReading) {
    Name[sizeof(Name) - 1] = '\0';
  }
}
template void NodeNameTag::Serialize(ArchiveWriter&);
template void NodeNameTag::Serialize(ArchiveReader&);

}  // namespace ra
