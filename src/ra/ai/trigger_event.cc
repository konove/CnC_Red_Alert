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

#include "ra/ai/trigger_event.h"

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
  if (object == nullptr) {
    // Every house is created before the triggers are read, so only a bad
    // house number in the scenario gets here. The original skipped the check,
    // which fires the trigger at once; that stays, but the scenario is wrong.
    // Checked every frame, so warn once for each kind of condition.
    DLOG_FIRST_N(WARNING, 1)
        << "Trigger event names house " << std::to_underlying(house)
        << ", which is not in the game; its condition counts as happened";
    return true;
  }
  return condition(*object);
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

// What the scenario editor and the scenario know about one event.
struct EventInfo {
  // The editor's name, which its lists show and sort by. A name ending in
  // "..." tells the designer the event takes an argument.
  const char* name;
  NeedType need;  // The argument the editor prompts for.
  // What a trigger with the event can be attached to: what is entered,
  // crossed or seen goes on the cell or the object itself, a house's state on
  // the house, and the game's state on the list the game logic springs.
  TriggerAttachment attachment;
};

constexpr auto kCell = TriggerAttachment::kCell;
constexpr auto kObject = TriggerAttachment::kObject;
constexpr auto kMap = TriggerAttachment::kMap;
constexpr auto kHouse = TriggerAttachment::kHouse;
constexpr auto kGeneral = TriggerAttachment::kGeneral;

constexpr base::EnumArray<TriggerEventType, EventInfo> kEvents = {{
    // kNone leaves the trigger free to go on a cell or an object.
    {"-No Event-", NeedType::kNone, kCell | kObject},
    {"Entered by...", NeedType::kHouse, kCell | kObject},
    {"Spied by...", NeedType::kNone, kObject},
    {"Thieved by...", NeedType::kHouse, kHouse},
    {"Discovered by player", NeedType::kNone, kCell | kObject},
    {"House Discovered...", NeedType::kHouse, kHouse},
    {"Attacked by anybody", NeedType::kNone, kObject},
    {"Destroyed by anybody", NeedType::kNone, kObject},
    {"Any Event", NeedType::kNone, kCell | kObject | kMap | kHouse | kGeneral},
    {"Destroyed, Units, All...", NeedType::kHouse, kHouse},
    {"Destroyed, Buildings, All...", NeedType::kHouse, kHouse},
    {"Destroyed, All...", NeedType::kHouse, kHouse},
    {"Credits exceed...", NeedType::kNumber, kHouse},
    {"Elapsed Time (1/10th min)...", NeedType::kNumber, kGeneral},
    {"Mission Timer Expired", NeedType::kNone, kGeneral},
    {"Destroyed, Buildings, #...", NeedType::kNumber, kHouse},
    {"Destroyed, Units, #...", NeedType::kNumber, kHouse},
    {"No Factories left", NeedType::kNone, kHouse},
    {"Civilians Evacuated", NeedType::kNone, kHouse},
    {"Build Building Type...", NeedType::kStructure, kHouse},
    {"Build Unit Type...", NeedType::kUnit, kHouse},
    {"Build Infantry Type...", NeedType::kInfantry, kHouse},
    {"Build Aircraft Type...", NeedType::kAircraft, kHouse},
    {"Leaves map (team)...", NeedType::kTeam, kGeneral},
    {"Zone Entry by...", NeedType::kHouse, kCell | kMap},
    {"Crosses Horizontal Line...", NeedType::kHouse, kCell},
    {"Crosses Vertical Line...", NeedType::kHouse, kCell},
    {"Global is set...", NeedType::kNumber, kGeneral},
    {"Global is clear...", NeedType::kNumber, kGeneral},
    {"Destroyed, Fakes, All...", NeedType::kNone, kHouse},
    {"Low Power...", NeedType::kHouse, kHouse},
    {"All bridges destroyed", NeedType::kNone, kGeneral},
    {"Building exists...", NeedType::kStructure, kHouse},
}};

// Returns the row of `event`. A number that is no event, which only a damaged
// saved game can hold (ReadIni() reads one as kNone), takes nothing and
// attaches to nothing.
const EventInfo& InfoOf(const TriggerEventType event) {
  static constexpr EventInfo kNoEvent = {
      .name = "-No Event-",
      .need = NeedType::kNone,
      .attachment = TriggerAttachment::kNone,
  };
  return magic_enum::enum_contains(event) ? kEvents.at(event) : kNoEvent;
}

}  // namespace

void TriggerEvent::Reset(TriggerEventState& state) const {
  state.is_tripped = false;
  if (type_ == TriggerEventType::kTime) {
    state.timer.Set(value() * (kTicksPerMinute / 10));
  }
}

void TriggerEvent::Detach(const Target target) {
  if (target.kind() == RTTI_TEAMTYPE && team_type_ == target.AsTeamType()) {
    team_type_ = nullptr;
  }
}

bool TriggerEvent::HasHappened(TriggerEventState& state,
                               const TriggerEventType cause,
                               const HousesType trigger_house,
                               const ObjectClass* object,
                               const bool forced) const {
  if (forced || state.is_tripped) {
    state.is_tripped = true;
    return true;
  }

  // The moment events are true merely because the check was made for them:
  // the caller springs TriggerEventType::kAttacked from the code that does the
  // attacking. The original let TriggerEventType::kAny through as well, so a
  // house's per-frame check (HouseClass::AI()) counted "attacked" as having
  // happened every frame for a trigger whose other event is a house's; no
  // shipped scenario has such a trigger.
  const bool checked_for_it = cause == type_;

  switch (type_) {
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
        return Trip(state, TeamHasLeftTheMap(team_type_));
      });

    case TriggerEventType::kCredits:
      return HouseMeets(trigger_house, [this](const HouseClass& owner) {
        return owner.Available_Money() >= value();
      });

    // A shipyard or sub pen does not count, as in the original: Soviet 5's
    // Greek engineer reinforcements arrive while the Greek shipyard stands.
    case TriggerEventType::kNoFactories:
      return HouseMeets(trigger_house, [](const HouseClass& owner) {
        return (owner.BScan &
                (kStructFlagAirstrip | kStructFlagTent | kStructFlagWeap |
                 kStructFlagBarracks | kStructFlagConst)) == 0;
      });

    // The original counted only the fake construction yard and war factory,
    // so a fake shipyard, sub pen or radar dome left standing did not stop the
    // event.
    case TriggerEventType::kFakesDestroyed:
      return HouseMeets(trigger_house, [](const HouseClass& owner) {
        return (owner.BScan & (kStructFlagFakeConst | kStructFlagFakeWeap |
                               kStructFlagFakeYard | kStructFlagFakePen |
                               kStructFlagFakeRadar)) == 0;
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

void TriggerEvent::ReadIni(base::Tokenizer& tokens) {
  const auto next_number = [&tokens] {
    return base::ParseIntegerOr<int>(tokens.Next(), 0);
  };
  // The team types are read before the trigger types, so a slot that holds no
  // team type names one the scenario does not have.
  const auto team_in_slot = [](const int slot_index) {
    TeamType* const team_type =
        TheObjectHeaps().team_type().FilledSlotOrNull(slot_index);
    if (team_type == nullptr && slot_index != -1) {
      DLOG(WARNING) << "TriggerEvent::ReadIni: team slot " << slot_index
                    << " holds no team type";
    }
    return team_type;
  };
  // An empty event field reads as 0, no event.
  type_ = EventFromNumber(next_number());

  const int format = TheWorld().new_ini_format();
  if (format == 0) {
    tokens.Next();  // Old-format field, unused.
    tokens.Next();  // Old-format field, unused.
    team_type_ = TeamType::FromName(tokens.Next());
    set_value(next_number());
    tokens.Next();  // Old-format field, unused.
  } else if (format == 1) {
    // One field holds the team or the value, whichever the event takes; an
    // empty one leaves both unset.
    const std::string_view token = tokens.Next();
    team_type_ = nullptr;
    set_value(-1);
    if (!token.empty()) {
      if (EventNeed(type_) == NeedType::kTeam) {
        team_type_ = team_in_slot(base::ParseIntegerOr<int>(token, 0));
      } else {
        set_value(base::ParseIntegerOr<int>(token, 0));
      }
    }
  } else {
    team_type_ = team_in_slot(next_number());
    set_value(next_number());
  }
}

std::string TriggerEvent::ToIniEntry() const {
  return std::format("{},{},{}", std::to_underlying(type_),
                     TheObjectHeaps().team_type().PositionOf(team_type_),
                     value());
}

NeedType EventNeed(const TriggerEventType event) { return InfoOf(event).need; }

const char* EventName(const TriggerEventType event) {
  return InfoOf(event).name;
}

TriggerAttachment EventAttachment(const TriggerEventType event) {
  return InfoOf(event).attachment;
}

}  // namespace ra
