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

#include <array>
#include <cstdlib>
#include <cstring>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "absl/log/log.h"
#include "absl/strings/match.h"
#include "base/array.h"
#include "base/enum_array.h"
#include "base/fixed.h"  // IWYU pragma: keep (Fixed < int via operator<=>)
#include "base/strings/number_parse.h"
#include "base/strings/tokenizer.h"
#include "engine/gfx/pixel_buffer.h"
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
#include "ra/ui/ui_skin.h"

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

base::EnumArray<TriggerEventType, EventChoice> event_choices = {{
    {TriggerEventType::kNone},
    {TriggerEventType::kPlayerEntered},
    {TriggerEventType::kSpied},
    {TriggerEventType::kThieved},
    {TriggerEventType::kDiscovered},
    {TriggerEventType::kHouseDiscovered},
    {TriggerEventType::kAttacked},
    {TriggerEventType::kDestroyed},
    {TriggerEventType::kAny},
    {TriggerEventType::kUnitsDestroyed},
    {TriggerEventType::kBuildingsDestroyed},
    {TriggerEventType::kAllDestroyed},
    {TriggerEventType::kCredits},
    {TriggerEventType::kTime},
    {TriggerEventType::kMissionTimerExpired},
    {TriggerEventType::kBuildingsLost},
    {TriggerEventType::kUnitsLost},
    {TriggerEventType::kNoFactories},
    {TriggerEventType::kCivilianEvacuated},
    {TriggerEventType::kBuildBuilding},
    {TriggerEventType::kBuildUnit},
    {TriggerEventType::kBuildInfantry},
    {TriggerEventType::kBuildAircraft},
    {TriggerEventType::kLeavesMap},
    {TriggerEventType::kEntersZone},
    {TriggerEventType::kCrossHorizontal},
    {TriggerEventType::kCrossVertical},
    {TriggerEventType::kGlobalSet},
    {TriggerEventType::kGlobalClear},
    {TriggerEventType::kFakesDestroyed},
    {TriggerEventType::kLowPower},
    {TriggerEventType::kAllBridgesDestroyed},
    {TriggerEventType::kBuildingExists},
}};

void EventChoice::Draw_It(PixelView& view, int /*unused*/, int x, int y,
                          int width, int height, bool selected, FontType font,
                          TextPrintType flags) const {
  // Tab stops in pixels; the event names hold no tabs.
  static constexpr std::array<int, 2> kTabStops = {13, 40};
  UiSkin::Draw_List_Entry(view, Description(), x, y, width, height, font, flags,
                          selected, kTabStops);
}

void TriggerEvent::Reset(TriggerEventState& state) const {
  state.is_tripped = false;
  if (Event == TriggerEventType::kTime) {
    state.timer.Set(value() * (kTicksPerMinute / 10));
  }
}

bool TriggerEvent::HasHappened(TriggerEventState& state, TriggerEventType cause,
                               HousesType trigger_house,
                               const ObjectClass* object, bool forced) const {
  if (forced) {
    state.is_tripped = true;
  }

  if (state.is_tripped) {
    return true;
  }

  // The game-state events are true while their condition is, never tripped:
  // a global that is cleared again, or a timer that is reset, turns them
  // false.
  switch (Event) {
    case TriggerEventType::kGlobalSet:
      return base::At(TheScenario().GlobalFlags, value());

    case TriggerEventType::kGlobalClear:
      return !base::At(TheScenario().GlobalFlags, value());

    case TriggerEventType::kMissionTimerExpired:
      return TheScenario().MissionTimer.IsRunning() &&
             TheScenario().MissionTimer.IsFinished();

    case TriggerEventType::kTime:
      return state.timer.IsFinished();
    case TriggerEventType::kNone:
    case TriggerEventType::kPlayerEntered:
    case TriggerEventType::kSpied:
    case TriggerEventType::kThieved:
    case TriggerEventType::kDiscovered:
    case TriggerEventType::kHouseDiscovered:
    case TriggerEventType::kAttacked:
    case TriggerEventType::kDestroyed:
    case TriggerEventType::kAny:
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
    case TriggerEventType::kLeavesMap:
    case TriggerEventType::kEntersZone:
    case TriggerEventType::kCrossHorizontal:
    case TriggerEventType::kCrossVertical:
    case TriggerEventType::kFakesDestroyed:
    case TriggerEventType::kLowPower:
    case TriggerEventType::kAllBridgesDestroyed:
    case TriggerEventType::kBuildingExists:
    default:
      break;
  }

  // An unused event never happens.
  if (Event == TriggerEventType::kNone) {
    return false;
  }

  // The moment events are true merely because the check was made for them:
  // the caller springs TriggerEventType::kAttacked from the code that does the
  // attacking. TriggerEventType::kAny lets them through as well, so a house's
  // per-frame check of a trigger counts them as having happened.
  if ((Event == TriggerEventType::kAttacked ||
       Event == TriggerEventType::kDestroyed ||
       Event == TriggerEventType::kDiscovered ||
       Event == TriggerEventType::kSpied || Event == TriggerEventType::kNone ||
       Event == TriggerEventType::kCrossHorizontal ||
       Event == TriggerEventType::kCrossVertical ||
       Event == TriggerEventType::kEntersZone ||
       Event == TriggerEventType::kPlayerEntered) &&
      (cause != Event && cause != TriggerEventType::kAny)) {
    return false;
  }

  // The entering events also need the object that entered to belong to the
  // event's house; the trigger's own house does not matter.
  if (Event == TriggerEventType::kPlayerEntered ||
      Event == TriggerEventType::kCrossHorizontal ||
      Event == TriggerEventType::kCrossVertical ||
      Event == TriggerEventType::kEntersZone) {
    if (!object || object->Owner() != this->house()) {
      return false;
    }
    state.is_tripped = true;
    return true;
  }

  // Tripped, so the event stays true if a bridge is repaired.
  if (Event == TriggerEventType::kAllBridgesDestroyed) {
    if (TheScenario().BridgeCount) {
      return false;
    }
    state.is_tripped = true;
    return true;
  }

  // The events below check a house's state, first the trigger's own house's.
  // An event whose house cannot be found is not checked and counts as having
  // happened.
  const HouseClass* const owner = HouseClass::As_Pointer(trigger_house);
  int team_index = 0;
  if (owner != nullptr) {
    switch (Event) {
      // A team springs the logic triggers itself once its last member has
      // left the map, just before it deletes itself, so it is still in the
      // heap here.
      case TriggerEventType::kLeavesMap:
        for (team_index = 0; team_index < TheObjectHeaps().team().Count();
             team_index++) {
          const TeamClass* team = TheObjectHeaps().team().ActiveAt(team_index);
          if (team->Class == Team && team->Is_Empty() && team->IsLeaveMap) {
            state.is_tripped = true;
            break;
          }
        }
        if (team_index == TheObjectHeaps().team().Count()) {
          return false;
        }
        break;

      // The editor calls this "Credits exceed (x100)", but the value is in
      // credits.
      case TriggerEventType::kCredits:
        if (owner->Available_Money() < value()) {
          return false;
        }
        break;

      case TriggerEventType::kNoFactories:
        if (owner->BScan &
            (kStructFlagAirstrip | kStructFlagTent | kStructFlagWeap |
             kStructFlagBarracks | kStructFlagConst)) {
          return false;
        }
        break;

      // Only two of the five fakes count: a fake shipyard, sub pen or radar
      // dome left standing does not stop the event.
      case TriggerEventType::kFakesDestroyed:
        if (owner->BScan & (kStructFlagFakeConst | kStructFlagFakeWeap)) {
          return false;
        }
        break;

      case TriggerEventType::kCivilianEvacuated:
        if (!owner->IsCivEvacuated) {
          return false;
        }
        break;

      // A structure past the 64 the scan holds never exists.
      case TriggerEventType::kBuildingExists:
        if ((owner->ActiveBScan & ScanBit(static_cast<int>(structure()))) ==
            0) {
          return false;
        }
        break;

      // The house only remembers what it built last, so the building
      // events are tripped to outlast the next thing it builds.
      case TriggerEventType::kBuildBuilding:
        if (owner->JustBuiltStructure != structure()) {
          return false;
        }
        state.is_tripped = true;
        break;

      case TriggerEventType::kBuildUnit:
        if (owner->JustBuiltUnit != unit()) {
          return false;
        }
        state.is_tripped = true;
        break;

      case TriggerEventType::kBuildInfantry:
        if (owner->JustBuiltInfantry != infantry()) {
          return false;
        }
        state.is_tripped = true;
        break;

      case TriggerEventType::kBuildAircraft:
        if (owner->JustBuiltAircraft != aircraft()) {
          return false;
        }
        state.is_tripped = true;
        break;

      case TriggerEventType::kBuildingsLost:
        if (std::cmp_less(owner->BuildingsLost, value())) {
          return false;
        }
        break;

      case TriggerEventType::kUnitsLost:
        if (std::cmp_less(owner->UnitsLost, value())) {
          return false;
        }
        break;

      case TriggerEventType::kNone:
      case TriggerEventType::kPlayerEntered:
      case TriggerEventType::kSpied:
      case TriggerEventType::kThieved:
      case TriggerEventType::kDiscovered:
      case TriggerEventType::kHouseDiscovered:
      case TriggerEventType::kAttacked:
      case TriggerEventType::kDestroyed:
      case TriggerEventType::kAny:
      case TriggerEventType::kUnitsDestroyed:
      case TriggerEventType::kBuildingsDestroyed:
      case TriggerEventType::kAllDestroyed:
      case TriggerEventType::kTime:
      case TriggerEventType::kMissionTimerExpired:
      case TriggerEventType::kEntersZone:
      case TriggerEventType::kCrossHorizontal:
      case TriggerEventType::kCrossVertical:
      case TriggerEventType::kGlobalSet:
      case TriggerEventType::kGlobalClear:
      case TriggerEventType::kLowPower:
      case TriggerEventType::kAllBridgesDestroyed:
      default:
        break;
    }
  }

  // Then the house the event names.
  const HouseClass* const event_house = HouseClass::As_Pointer(house());
  if (event_house != nullptr) {
    switch (Event) {
      case TriggerEventType::kLowPower:
        if (event_house->Power_Fraction() >= 1) {
          return false;
        }
        break;

      case TriggerEventType::kThieved:
        if (!event_house->IsThieved) {
          return false;
        }
        break;

      case TriggerEventType::kHouseDiscovered:
        if (!event_house->IsDiscovered) {
          return false;
        }
        break;

      case TriggerEventType::kBuildingsDestroyed:
        if (event_house->ActiveBScan) {
          return false;
        }
        break;

      // Vessels and aircraft do not count.
      case TriggerEventType::kUnitsDestroyed:
        if (event_house->ActiveUScan | event_house->ActiveIScan) {
          return false;
        }
        break;

      // Aircraft do not count.
      case TriggerEventType::kAllDestroyed:
        if (event_house->ActiveBScan | event_house->ActiveUScan |
            event_house->ActiveIScan | event_house->ActiveVScan) {
          return false;
        }
        break;

      case TriggerEventType::kNone:
      case TriggerEventType::kPlayerEntered:
      case TriggerEventType::kSpied:
      case TriggerEventType::kDiscovered:
      case TriggerEventType::kAttacked:
      case TriggerEventType::kDestroyed:
      case TriggerEventType::kAny:
      case TriggerEventType::kCredits:
      case TriggerEventType::kTime:
      case TriggerEventType::kMissionTimerExpired:
      case TriggerEventType::kBuildingsLost:
      case TriggerEventType::kUnitsLost:
      case TriggerEventType::kNoFactories:
      case TriggerEventType::kCivilianEvacuated:
      case TriggerEventType::kBuildBuilding:
      case TriggerEventType::kBuildUnit:
      case TriggerEventType::kBuildInfantry:
      case TriggerEventType::kBuildAircraft:
      case TriggerEventType::kLeavesMap:
      case TriggerEventType::kEntersZone:
      case TriggerEventType::kCrossHorizontal:
      case TriggerEventType::kCrossVertical:
      case TriggerEventType::kGlobalSet:
      case TriggerEventType::kGlobalClear:
      case TriggerEventType::kFakesDestroyed:
      case TriggerEventType::kAllBridgesDestroyed:
      case TriggerEventType::kBuildingExists:
      default:
        break;
    }
  }

  return true;
}

std::string TriggerEvent::ToIniEntry() const {
  return std::format("{},{},{}", std::to_underlying(Event),
                     TheObjectHeaps().team_type().PositionOf(Team), value());
}

void TriggerEvent::ReadIni(base::Tokenizer& tokens) {
  std::string_view token;
  switch (TheWorld().new_ini_format()) {
    default:
      Event = EventFromNumber(base::ParseIntegerOr<int>(tokens.Next(), 0));
      Team = TheObjectHeaps().team_type().SlotOrNull(
          base::ParseIntegerOr<int>(tokens.Next(), 0));
      set_value(base::ParseIntegerOr<int>(tokens.Next(), 0));
      break;

    case 1:
      token = tokens.Next();
      Event = TriggerEventType::kNone;
      if (!token.empty()) {
        Event = EventFromNumber(base::ParseIntegerOr<int>(token, 0));
      }

      token = tokens.Next();
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
      break;

    case 0:
      Event = EventFromNumber(base::ParseIntegerOr<int>(tokens.Next(), 0));

      tokens.Next();  // Old-format field, unused.
      tokens.Next();  // Old-format field, unused.

      Team = TeamType::FromName(tokens.Next());
      set_value(base::ParseIntegerOr<int>(tokens.Next(), 0));
      tokens.Next();  // Old-format field, unused.
      break;
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

TriggerEventType Event_From_Name(const std::string_view name) {
  if (!name.empty()) {
    for (const TriggerEventType i :
         magic_enum::enum_values<TriggerEventType>()) {
      if (absl::EqualsIgnoreCase(name, kEventNames.at(i))) {
        return i;
      }
    }
  }

  return TriggerEventType::kNone;
}

const char* EventName(TriggerEventType event) { return kEventNames.at(event); }

TriggerAttachment EventAttachment(TriggerEventType event) {
  TriggerAttachment attachment = TriggerAttachment::kNone;

  switch (event) {
    case TriggerEventType::kCrossHorizontal:
    case TriggerEventType::kCrossVertical:
    case TriggerEventType::kEntersZone:
    case TriggerEventType::kPlayerEntered:
    case TriggerEventType::kAny:
    case TriggerEventType::kDiscovered:
    case TriggerEventType::kNone:
      attachment = attachment | TriggerAttachment::kCell;
      break;

    case TriggerEventType::kSpied:
    case TriggerEventType::kThieved:
    case TriggerEventType::kHouseDiscovered:
    case TriggerEventType::kAttacked:
    case TriggerEventType::kDestroyed:
    case TriggerEventType::kUnitsDestroyed:
    case TriggerEventType::kBuildingsDestroyed:
    case TriggerEventType::kAllDestroyed:
    case TriggerEventType::kCredits:
    case TriggerEventType::kTime:
    case TriggerEventType::kMissionTimerExpired:
    case TriggerEventType::kBuildingsLost:
    case TriggerEventType::kUnitsLost:
    case TriggerEventType::kNoFactories:
    case TriggerEventType::kCivilianEvacuated:
    case TriggerEventType::kBuildBuilding:
    case TriggerEventType::kBuildUnit:
    case TriggerEventType::kBuildInfantry:
    case TriggerEventType::kBuildAircraft:
    case TriggerEventType::kLeavesMap:
    case TriggerEventType::kGlobalSet:
    case TriggerEventType::kGlobalClear:
    case TriggerEventType::kFakesDestroyed:
    case TriggerEventType::kLowPower:
    case TriggerEventType::kAllBridgesDestroyed:
    case TriggerEventType::kBuildingExists:
    default:
      break;
  }

  switch (event) {
    case TriggerEventType::kSpied:
    case TriggerEventType::kPlayerEntered:
    case TriggerEventType::kDiscovered:
    case TriggerEventType::kDestroyed:
    case TriggerEventType::kAttacked:
    case TriggerEventType::kAny:
    case TriggerEventType::kNone:
      attachment = attachment | TriggerAttachment::kObject;
      break;

    case TriggerEventType::kThieved:
    case TriggerEventType::kHouseDiscovered:
    case TriggerEventType::kUnitsDestroyed:
    case TriggerEventType::kBuildingsDestroyed:
    case TriggerEventType::kAllDestroyed:
    case TriggerEventType::kCredits:
    case TriggerEventType::kTime:
    case TriggerEventType::kMissionTimerExpired:
    case TriggerEventType::kBuildingsLost:
    case TriggerEventType::kUnitsLost:
    case TriggerEventType::kNoFactories:
    case TriggerEventType::kCivilianEvacuated:
    case TriggerEventType::kBuildBuilding:
    case TriggerEventType::kBuildUnit:
    case TriggerEventType::kBuildInfantry:
    case TriggerEventType::kBuildAircraft:
    case TriggerEventType::kLeavesMap:
    case TriggerEventType::kEntersZone:
    case TriggerEventType::kCrossHorizontal:
    case TriggerEventType::kCrossVertical:
    case TriggerEventType::kGlobalSet:
    case TriggerEventType::kGlobalClear:
    case TriggerEventType::kFakesDestroyed:
    case TriggerEventType::kLowPower:
    case TriggerEventType::kAllBridgesDestroyed:
    case TriggerEventType::kBuildingExists:
    default:
      break;
  }

  switch (event) {
    case TriggerEventType::kEntersZone:
    case TriggerEventType::kAny:
      attachment = attachment | TriggerAttachment::kMap;
      break;

    case TriggerEventType::kNone:
    case TriggerEventType::kPlayerEntered:
    case TriggerEventType::kSpied:
    case TriggerEventType::kThieved:
    case TriggerEventType::kDiscovered:
    case TriggerEventType::kHouseDiscovered:
    case TriggerEventType::kAttacked:
    case TriggerEventType::kDestroyed:
    case TriggerEventType::kUnitsDestroyed:
    case TriggerEventType::kBuildingsDestroyed:
    case TriggerEventType::kAllDestroyed:
    case TriggerEventType::kCredits:
    case TriggerEventType::kTime:
    case TriggerEventType::kMissionTimerExpired:
    case TriggerEventType::kBuildingsLost:
    case TriggerEventType::kUnitsLost:
    case TriggerEventType::kNoFactories:
    case TriggerEventType::kCivilianEvacuated:
    case TriggerEventType::kBuildBuilding:
    case TriggerEventType::kBuildUnit:
    case TriggerEventType::kBuildInfantry:
    case TriggerEventType::kBuildAircraft:
    case TriggerEventType::kLeavesMap:
    case TriggerEventType::kCrossHorizontal:
    case TriggerEventType::kCrossVertical:
    case TriggerEventType::kGlobalSet:
    case TriggerEventType::kGlobalClear:
    case TriggerEventType::kFakesDestroyed:
    case TriggerEventType::kLowPower:
    case TriggerEventType::kAllBridgesDestroyed:
    case TriggerEventType::kBuildingExists:
    default:
      break;
  }

  switch (event) {
    case TriggerEventType::kLowPower:
    case TriggerEventType::kCivilianEvacuated:
    case TriggerEventType::kBuildingExists:
    case TriggerEventType::kBuildBuilding:
    case TriggerEventType::kBuildUnit:
    case TriggerEventType::kBuildInfantry:
    case TriggerEventType::kBuildAircraft:
    case TriggerEventType::kNoFactories:
    case TriggerEventType::kBuildingsDestroyed:
    case TriggerEventType::kBuildingsLost:
    case TriggerEventType::kUnitsDestroyed:
    case TriggerEventType::kUnitsLost:
    case TriggerEventType::kAllDestroyed:
    case TriggerEventType::kHouseDiscovered:
    case TriggerEventType::kCredits:
    case TriggerEventType::kThieved:
    case TriggerEventType::kAny:
    case TriggerEventType::kFakesDestroyed:
      attachment = attachment | TriggerAttachment::kHouse;
      break;

    case TriggerEventType::kNone:
    case TriggerEventType::kPlayerEntered:
    case TriggerEventType::kSpied:
    case TriggerEventType::kDiscovered:
    case TriggerEventType::kAttacked:
    case TriggerEventType::kDestroyed:
    case TriggerEventType::kTime:
    case TriggerEventType::kMissionTimerExpired:
    case TriggerEventType::kLeavesMap:
    case TriggerEventType::kEntersZone:
    case TriggerEventType::kCrossHorizontal:
    case TriggerEventType::kCrossVertical:
    case TriggerEventType::kGlobalSet:
    case TriggerEventType::kGlobalClear:
    case TriggerEventType::kAllBridgesDestroyed:
    default:
      break;
  }

  switch (event) {
    case TriggerEventType::kTime:
    case TriggerEventType::kGlobalSet:
    case TriggerEventType::kGlobalClear:
    case TriggerEventType::kMissionTimerExpired:
    case TriggerEventType::kAny:
    case TriggerEventType::kAllBridgesDestroyed:
    case TriggerEventType::kLeavesMap:
      attachment = attachment | TriggerAttachment::kGeneral;
      break;

    case TriggerEventType::kNone:
    case TriggerEventType::kPlayerEntered:
    case TriggerEventType::kSpied:
    case TriggerEventType::kThieved:
    case TriggerEventType::kDiscovered:
    case TriggerEventType::kHouseDiscovered:
    case TriggerEventType::kAttacked:
    case TriggerEventType::kDestroyed:
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
    case TriggerEventType::kEntersZone:
    case TriggerEventType::kCrossHorizontal:
    case TriggerEventType::kCrossVertical:
    case TriggerEventType::kFakesDestroyed:
    case TriggerEventType::kLowPower:
    case TriggerEventType::kBuildingExists:
    default:
      break;
  }

  return attachment;
}

}  // namespace ra
