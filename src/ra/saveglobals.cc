// Field lists for Red Alert's scenario and other non-heap save state.

#include <cstdint>

#include "magic_enum/magic_enum.hpp"
#include "ra/carry.h"
#include "ra/defines.h"
#include "ra/options.h"
#include "ra/scenario.h"
#include "ra/score.h"
#include "ra/session.h"
#include "ra/special.h"
#include "tech/archive.h"
#include "tech/ftimer.h"
#include "tech/random.h"

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
  if constexpr (Archive::kIsReading) {
    ChangingGun = nullptr;
  }
}
template void ScoreClass::Serialize(ArchiveWriter&);
template void ScoreClass::Serialize(ArchiveReader&);

template <class Archive>
void CarryoverClass::Serialize(Archive& ar) {
  ar(RTTI);
  switch (RTTI) {
    case RTTI_BUILDING:
      if constexpr (Archive::kIsReading) {
        Type.Building = STRUCT_NONE;
      }
      ar(Type.Building);
      if constexpr (Archive::kIsReading) {
        if (!magic_enum::enum_contains(Type.Building)) {
          ar.Fail("invalid carryover object type");
        }
      }
      break;
    case RTTI_UNIT:
      if constexpr (Archive::kIsReading) {
        Type.Unit = UNIT_NONE;
      }
      ar(Type.Unit);
      if constexpr (Archive::kIsReading) {
        if (!magic_enum::enum_contains(Type.Unit)) {
          ar.Fail("invalid carryover object type");
        }
      }
      break;
    case RTTI_INFANTRY:
      if constexpr (Archive::kIsReading) {
        Type.Infantry = INFANTRY_NONE;
      }
      ar(Type.Infantry);
      if constexpr (Archive::kIsReading) {
        if (!magic_enum::enum_contains(Type.Infantry)) {
          ar.Fail("invalid carryover object type");
        }
      }
      break;
    case RTTI_VESSEL:
      if constexpr (Archive::kIsReading) {
        Type.Vessel = VESSEL_NONE;
      }
      ar(Type.Vessel);
      if constexpr (Archive::kIsReading) {
        if (!magic_enum::enum_contains(Type.Vessel)) {
          ar.Fail("invalid carryover object type");
        }
      }
      break;
    case RTTIType::RTTI_NONE:
    case RTTIType::RTTI_AIRCRAFT:
    case RTTIType::RTTI_AIRCRAFTTYPE:
    case RTTIType::RTTI_ANIM:
    case RTTIType::RTTI_ANIMTYPE:
    case RTTIType::RTTI_BUILDINGTYPE:
    case RTTIType::RTTI_BULLET:
    case RTTIType::RTTI_BULLETTYPE:
    case RTTIType::RTTI_CELL:
    case RTTIType::RTTI_FACTORY:
    case RTTIType::RTTI_HOUSE:
    case RTTIType::RTTI_HOUSETYPE:
    case RTTIType::RTTI_INFANTRYTYPE:
    case RTTIType::RTTI_OVERLAY:
    case RTTIType::RTTI_OVERLAYTYPE:
    case RTTIType::RTTI_SMUDGE:
    case RTTIType::RTTI_SMUDGETYPE:
    case RTTIType::RTTI_SPECIAL:
    case RTTIType::RTTI_TEAM:
    case RTTIType::RTTI_TEAMTYPE:
    case RTTIType::RTTI_TEMPLATE:
    case RTTIType::RTTI_TEMPLATETYPE:
    case RTTIType::RTTI_TERRAIN:
    case RTTIType::RTTI_TERRAINTYPE:
    case RTTIType::RTTI_TRIGGER:
    case RTTIType::RTTI_TRIGGERTYPE:
    case RTTIType::RTTI_UNITTYPE:
    case RTTIType::RTTI_VESSELTYPE:
    default:
      if constexpr (Archive::kIsReading) {
        ar.Fail("invalid carryover object kind");
      }
      return;
  }
  ar(Cell, Strength, House);
  if constexpr (Archive::kIsReading) {
    if (Cell < 0 || Cell >= MAP_CELL_TOTAL ||
        !magic_enum::enum_contains(House)) {
      ar.Fail("invalid carryover cell or house");
    }
  }
}
template void CarryoverClass::Serialize(ArchiveWriter&);
template void CarryoverClass::Serialize(ArchiveReader&);

template <class Archive>
void SpecialClass::Serialize(Archive& ar) {
  uint8_t flags = 0;
  flags |= static_cast<uint8_t>(IsShadowGrow << 0);
  flags |= static_cast<uint8_t>(IsSpeedBuild << 1);
  flags |= static_cast<uint8_t>(IsFromInstall << 2);
  flags |= static_cast<uint8_t>(IsCaptureTheFlag << 3);
  flags |= static_cast<uint8_t>(IsInert << 4);
  flags |= static_cast<uint8_t>(IsThreePoint << 5);
  flags |= static_cast<uint8_t>(IsTGrowth << 6);
  flags |= static_cast<uint8_t>(IsTSpread << 7);
  ar(flags);
  if constexpr (Archive::kIsReading) {
    IsShadowGrow = (flags & (1U << 0)) != 0;
    IsSpeedBuild = (flags & (1U << 1)) != 0;
    IsFromInstall = (flags & (1U << 2)) != 0;
    IsCaptureTheFlag = (flags & (1U << 3)) != 0;
    IsInert = (flags & (1U << 4)) != 0;
    IsThreePoint = (flags & (1U << 5)) != 0;
    IsTGrowth = (flags & (1U << 6)) != 0;
    IsTSpread = (flags & (1U << 7)) != 0;
  }
}
template void SpecialClass::Serialize(ArchiveWriter&);
template void SpecialClass::Serialize(ArchiveReader&);

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
