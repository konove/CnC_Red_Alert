/*
**	Command & Conquer Red Alert(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ra/ai/tevent.h"

#include <cstdlib>
#include <cstring>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "absl/log/log.h"
#include "base/array.h"
#include "base/enum_array.h"
#include "base/fixed.h"  // IWYU pragma: keep (Fixed < int via operator<=>)
#include "base/strings/number_parse.h"
#include "base/strings/tokenizer.h"
#include "game/core/object_heap.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/ai/scenario.h"
#include "ra/ai/team.h"
#include "ra/ai/team_type.h"
#include "ra/core/defines.h"
#include "ra/core/object_heaps.h"
#include "ra/map/world.h"
#include "ra/objects/house.h"
#include "ra/objects/object.h"

namespace ra {

namespace {

// Returns event number `number` from a scenario INI as an event, or
// TriggerEventType::kNone when it names none. The number indexes the name table
// and picks the check, which takes anything it does not list as having
// happened.
TriggerEventType EventFromNumber(const int number) {
  const std::optional<TriggerEventType> event =
      magic_enum::enum_cast<TriggerEventType>(number);
  if (!event.has_value()) {
    DLOG(WARNING) << "EventFromNumber: " << number << " is no event";
    return TriggerEventType::kNone;
  }
  return *event;
}

// Trips `state` when `happened`, so a moment that has passed keeps counting
// until the trigger is reset. Returns `happened`.
bool Trip(TriggerEventState& state, const bool happened) {
  if (happened) {
    state.is_tripped = true;
  }
  return happened;
}

// Returns whether `condition` holds for the house `house`, or true when that
// house is not in the game: such a condition is not checked.
template <class Condition>
bool HouseMeets(const HousesType house, const Condition& condition) {
  const HouseClass* const object = HouseClass::As_Pointer(house);
  return object == nullptr || condition(*object);
}

// Returns whether an emptied team of type `team_type` has left the map.
bool TeamHasLeftTheMap(const TeamType* const team_type) {
  const auto& teams = TheObjectHeaps().team();
  for (int index = 0; index < teams.Count(); index++) {
    const TeamClass* const team = teams.ActiveAt(index);
    if (team->Class == team_type && team->Is_Empty() && team->IsLeaveMap) {
      return true;
    }
  }
  return false;
}

}  // namespace

// The scenario editor's name for each event, which its lists show and sort by.
constexpr base::EnumArray<TriggerEventType, const char*> kEventNames = {
    "-No Event-",
    "Entered by...",
    "Spied by...",
    "Thieved by...",
    "Discovered by player",
    "House Discovered...",
    "Attacked by anybody",
    "Destroyed by anybody",
    "Any Event",
    "Destroyed, Units, All...",
    "Destroyed, Buildings, All...",
    "Destroyed, All...",
    "Credits exceed (x100)...",
    "Elapsed Time (1/10th min)...",
    "Mission Timer Expired",
    "Destroyed, Buildings, #...",
    "Destroyed, Units, #...",
    "No Factories left",
    "Civilians Evacuated",
    "Build Building Type...",
    "Build Unit Type...",
    "Build Infantry Type...",
    "Build Aircraft Type...",
    "Leaves map (team)...",
    "Zone Entry by...",
    "Crosses Horizontal Line...",
    "Crosses Vertical Line...",
    "Global is set...",
    "Global is clear...",
    "Destroyed, Fakes, All...",
    "Low Power...",
    "All bridges destroyed",
    "Building exists..."};

void TriggerEvent::Reset(TriggerEventState& state) const {
  state.is_tripped = false;
  if (Event == TriggerEventType::kTime) {
    state.timer.Set(value() * (kTicksPerMinute / 10));
  }
}

bool TriggerEvent::HasHappened(TriggerEventState& state, TriggerEventType cause,
                               HousesType trigger_house,
                               const ObjectClass* object, bool forced) const {
  if (forced || state.is_tripped) {
    state.is_tripped = true;
    return true;
  }

  // The moment events are true merely because the check was made for them:
  // the caller springs TriggerEventType::kAttacked from the code that does the
  // attacking. TriggerEventType::kAny lets them through as well, so a house's
  // per-frame check of a trigger counts them as having happened.
  const bool checked_for_it = cause == Event || cause == TriggerEventType::kAny;

  switch (Event) {
    // The game-state events are true while their condition is, never tripped:
    // a global that is cleared again, or a timer that is reset, turns them
    // false.
    case TriggerEventType::kGlobalSet:
      return base::At(TheScenario().GlobalFlags, value());
    case TriggerEventType::kGlobalClear:
      return !base::At(TheScenario().GlobalFlags, value());
    case TriggerEventType::kMissionTimerExpired:
      return TheScenario().MissionTimer.IsRunning() &&
             TheScenario().MissionTimer.IsFinished();
    case TriggerEventType::kTime:
      return state.timer.IsFinished();

    // An unused event never happens.
    case TriggerEventType::kNone:
      return false;

    case TriggerEventType::kAttacked:
    case TriggerEventType::kDestroyed:
    case TriggerEventType::kDiscovered:
    case TriggerEventType::kSpied:
      return checked_for_it;

    // The entering events also need the object that entered to belong to the
    // event's house; the trigger's own house does not matter.
    case TriggerEventType::kPlayerEntered:
    case TriggerEventType::kCrossHorizontal:
    case TriggerEventType::kCrossVertical:
    case TriggerEventType::kEntersZone:
      return Trip(state, checked_for_it && object != nullptr &&
                             object->Owner() == house());

    // Tripped, so the event stays true if a bridge is repaired.
    case TriggerEventType::kAllBridgesDestroyed:
      return Trip(state, TheScenario().BridgeCount == 0);

    // The events below check a house's state, first the trigger's own
    // house's. An event whose house cannot be found is not checked and counts
    // as having happened.

    // A team springs the logic triggers itself once its last member has left
    // the map, just before it deletes itself, so it is still in the heap
    // here. Nothing reads the house.
    case TriggerEventType::kLeavesMap:
      return HouseMeets(trigger_house, [&](const HouseClass& /*owner*/) {
        return Trip(state, TeamHasLeftTheMap(Team));
      });

    // The editor calls this "Credits exceed (x100)", but the value is in
    // credits.
    case TriggerEventType::kCredits:
      return HouseMeets(trigger_house, [this](const HouseClass& owner) {
        return owner.Available_Money() >= value();
      });

    case TriggerEventType::kNoFactories:
      return HouseMeets(trigger_house, [](const HouseClass& owner) {
        return (owner.BScan &
                (kStructFlagAirstrip | kStructFlagTent | kStructFlagWeap |
                 kStructFlagBarracks | kStructFlagConst)) == 0;
      });

    // Only two of the five fakes count: a fake shipyard, sub pen or radar
    // dome left standing does not stop the event.
    case TriggerEventType::kFakesDestroyed:
      return HouseMeets(trigger_house, [](const HouseClass& owner) {
        return (owner.BScan & (kStructFlagFakeConst | kStructFlagFakeWeap)) ==
               0;
      });

    case TriggerEventType::kCivilianEvacuated:
      return HouseMeets(trigger_house, [](const HouseClass& owner) {
        return owner.IsCivEvacuated;
      });

    // A structure past the 64 the scan holds never exists.
    case TriggerEventType::kBuildingExists:
      return HouseMeets(trigger_house, [this](const HouseClass& owner) {
        return (owner.ActiveBScan & ScanBit(static_cast<int>(structure()))) !=
               0;
      });

    // The house only remembers what it built last, so the building events
    // are tripped to outlast the next thing it builds.
    case TriggerEventType::kBuildBuilding:
      return HouseMeets(trigger_house, [&](const HouseClass& owner) {
        return Trip(state, owner.JustBuiltStructure == structure());
      });
    case TriggerEventType::kBuildUnit:
      return HouseMeets(trigger_house, [&](const HouseClass& owner) {
        return Trip(state, owner.JustBuiltUnit == unit());
      });
    case TriggerEventType::kBuildInfantry:
      return HouseMeets(trigger_house, [&](const HouseClass& owner) {
        return Trip(state, owner.JustBuiltInfantry == infantry());
      });
    case TriggerEventType::kBuildAircraft:
      return HouseMeets(trigger_house, [&](const HouseClass& owner) {
        return Trip(state, owner.JustBuiltAircraft == aircraft());
      });

    case TriggerEventType::kBuildingsLost:
      return HouseMeets(trigger_house, [this](const HouseClass& owner) {
        return std::cmp_greater_equal(owner.BuildingsLost, value());
      });
    case TriggerEventType::kUnitsLost:
      return HouseMeets(trigger_house, [this](const HouseClass& owner) {
        return std::cmp_greater_equal(owner.UnitsLost, value());
      });

    // Then the house the event names.
    case TriggerEventType::kLowPower:
      return HouseMeets(house(), [](const HouseClass& named) {
        return named.Power_Fraction() < 1;
      });
    case TriggerEventType::kThieved:
      return HouseMeets(
          house(), [](const HouseClass& named) { return named.IsThieved; });
    case TriggerEventType::kHouseDiscovered:
      return HouseMeets(
          house(), [](const HouseClass& named) { return named.IsDiscovered; });
    case TriggerEventType::kBuildingsDestroyed:
      return HouseMeets(house(), [](const HouseClass& named) {
        return named.ActiveBScan == 0;
      });
    // Vessels and aircraft do not count.
    case TriggerEventType::kUnitsDestroyed:
      return HouseMeets(house(), [](const HouseClass& named) {
        return (named.ActiveUScan | named.ActiveIScan) == 0;
      });
    // Aircraft do not count.
    case TriggerEventType::kAllDestroyed:
      return HouseMeets(house(), [](const HouseClass& named) {
        return (named.ActiveBScan | named.ActiveUScan | named.ActiveIScan |
                named.ActiveVScan) == 0;
      });

    case TriggerEventType::kAny:
    default:
      return true;
  }
}

std::string TriggerEvent::ToIniEntry() const {
  return std::format("{},{},{}", std::to_underlying(Event),
                     TheObjectHeaps().team_type().PositionOf(Team), value());
}

void TriggerEvent::ReadIni(base::Tokenizer& tokens) {
  const auto next_number = [&tokens] {
    return base::ParseIntegerOr<int>(tokens.Next(), 0);
  };
  // An empty event field reads as 0, no event.
  Event = EventFromNumber(next_number());

  const int format = TheWorld().new_ini_format();
  if (format == 0) {
    tokens.Next();  // Old-format field, unused.
    tokens.Next();  // Old-format field, unused.
    Team = TeamType::FromName(tokens.Next());
    set_value(next_number());
    tokens.Next();  // Old-format field, unused.
  } else if (format == 1) {
    // One field holds the team or the value, whichever the event takes; an
    // empty one leaves both unset.
    const std::string_view token = tokens.Next();
    Team = nullptr;
    set_value(-1);
    if (!token.empty()) {
      if (EventNeed(Event) == NeedType::kTeam) {
        Team = TheObjectHeaps().team_type().SlotOrNull(
            base::ParseIntegerOr<int>(token, 0));
      } else {
        set_value(base::ParseIntegerOr<int>(token, 0));
      }
    }
  } else {
    Team = TheObjectHeaps().team_type().SlotOrNull(next_number());
    set_value(next_number());
  }
}

NeedType EventNeed(TriggerEventType event) {
  switch (event) {
    case TriggerEventType::kThieved:
    case TriggerEventType::kPlayerEntered:
    case TriggerEventType::kCrossHorizontal:
    case TriggerEventType::kCrossVertical:
    case TriggerEventType::kEntersZone:
    case TriggerEventType::kHouseDiscovered:
    case TriggerEventType::kBuildingsDestroyed:
    case TriggerEventType::kUnitsDestroyed:
    case TriggerEventType::kAllDestroyed:
    case TriggerEventType::kLowPower:
      return NeedType::kHouse;

    case TriggerEventType::kUnitsLost:
    case TriggerEventType::kBuildingsLost:
    case TriggerEventType::kCredits:
    case TriggerEventType::kTime:
    case TriggerEventType::kGlobalSet:
    case TriggerEventType::kGlobalClear:
      return NeedType::kNumber;

    case TriggerEventType::kBuildingExists:
    case TriggerEventType::kBuildBuilding:
      return NeedType::kStructure;

    case TriggerEventType::kBuildUnit:
      return NeedType::kUnit;

    case TriggerEventType::kBuildInfantry:
      return NeedType::kInfantry;

    case TriggerEventType::kBuildAircraft:
      return NeedType::kAircraft;

    case TriggerEventType::kLeavesMap:
      return NeedType::kTeam;

    case TriggerEventType::kNone:
    case TriggerEventType::kSpied:
    case TriggerEventType::kDiscovered:
    case TriggerEventType::kAttacked:
    case TriggerEventType::kDestroyed:
    case TriggerEventType::kAny:
    case TriggerEventType::kMissionTimerExpired:
    case TriggerEventType::kNoFactories:
    case TriggerEventType::kCivilianEvacuated:
    case TriggerEventType::kFakesDestroyed:
    case TriggerEventType::kAllBridgesDestroyed:
    default:
      break;
  }
  return NeedType::kNone;
}

const char* EventName(TriggerEventType event) { return kEventNames.at(event); }

TriggerAttachment EventAttachment(const TriggerEventType event) {
  constexpr TriggerAttachment kCell = TriggerAttachment::kCell;
  constexpr TriggerAttachment kObject = TriggerAttachment::kObject;
  constexpr TriggerAttachment kMap = TriggerAttachment::kMap;
  constexpr TriggerAttachment kHouse = TriggerAttachment::kHouse;
  constexpr TriggerAttachment kGeneral = TriggerAttachment::kGeneral;

  switch (event) {
    // What is entered, crossed or seen: the cell or the object itself. An
    // unset event leaves the trigger free to go on either.
    case TriggerEventType::kNone:
    case TriggerEventType::kPlayerEntered:
    case TriggerEventType::kDiscovered:
      return kCell | kObject;
    case TriggerEventType::kSpied:
    case TriggerEventType::kAttacked:
    case TriggerEventType::kDestroyed:
      return kObject;
    case TriggerEventType::kCrossHorizontal:
    case TriggerEventType::kCrossVertical:
      return kCell;
    case TriggerEventType::kEntersZone:
      return kCell | kMap;

    // A house's state, checked with the house.
    case TriggerEventType::kThieved:
    case TriggerEventType::kHouseDiscovered:
    case TriggerEventType::kUnitsDestroyed:
    case TriggerEventType::kBuildingsDestroyed:
    case TriggerEventType::kAllDestroyed:
    case TriggerEventType::kCredits:
    case TriggerEventType::kBuildingsLost:
    case TriggerEventType::kUnitsLost:
    case TriggerEventType::kNoFactories:
    case TriggerEventType::kCivilianEvacuated:
    case TriggerEventType::kBuildBuilding:
    case TriggerEventType::kBuildUnit:
    case TriggerEventType::kBuildInfantry:
    case TriggerEventType::kBuildAircraft:
    case TriggerEventType::kFakesDestroyed:
    case TriggerEventType::kLowPower:
    case TriggerEventType::kBuildingExists:
      return kHouse;

    // The game's state, on the list of triggers the game logic springs.
    case TriggerEventType::kTime:
    case TriggerEventType::kMissionTimerExpired:
    case TriggerEventType::kLeavesMap:
    case TriggerEventType::kGlobalSet:
    case TriggerEventType::kGlobalClear:
    case TriggerEventType::kAllBridgesDestroyed:
      return kGeneral;

    case TriggerEventType::kAny:
      return kCell | kObject | kMap | kHouse | kGeneral;

    default:
      break;
  }
  return TriggerAttachment::kNone;
}

}  // namespace ra
