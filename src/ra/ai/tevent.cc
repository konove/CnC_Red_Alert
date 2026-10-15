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
// TEVENT_NONE when it names none. The number indexes the name table and picks
// the check, which takes anything it does not list as having happened.
TEventType EventFromNumber(const int number) {
  const std::optional<TEventType> event =
      magic_enum::enum_cast<TEventType>(number);
  if (!event.has_value()) {
    DLOG(WARNING) << "EventFromNumber: " << number << " is no event";
    return TEVENT_NONE;
  }
  return *event;
}

}  // namespace

// The scenario editor's name for each event, which its lists show and sort by.
static base::EnumArray<TEventType, const char*> EventText = {
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

base::EnumArray<TEventType, EventChoiceClass> EventChoices = {{
    {TEVENT_NONE},
    {TEVENT_PLAYER_ENTERED},
    {TEVENT_SPIED},
    {TEVENT_THIEVED},
    {TEVENT_DISCOVERED},
    {TEVENT_HOUSE_DISCOVERED},
    {TEVENT_ATTACKED},
    {TEVENT_DESTROYED},
    {TEVENT_ANY},
    {TEVENT_UNITS_DESTROYED},
    {TEVENT_BUILDINGS_DESTROYED},
    {TEVENT_ALL_DESTROYED},
    {TEVENT_CREDITS},
    {TEVENT_TIME},
    {TEVENT_MISSION_TIMER_EXPIRED},
    {TEVENT_NBUILDINGS_DESTROYED},
    {TEVENT_NUNITS_DESTROYED},
    {TEVENT_NOFACTORIES},
    {TEVENT_EVAC_CIVILIAN},
    {TEVENT_BUILD},
    {TEVENT_BUILD_UNIT},
    {TEVENT_BUILD_INFANTRY},
    {TEVENT_BUILD_AIRCRAFT},
    {TEVENT_LEAVES_MAP},
    {TEVENT_ENTERS_ZONE},
    {TEVENT_CROSS_HORIZONTAL},
    {TEVENT_CROSS_VERTICAL},
    {TEVENT_GLOBAL_SET},
    {TEVENT_GLOBAL_CLEAR},
    {TEVENT_FAKES_DESTROYED},
    {TEVENT_LOW_POWER},
    {TEVENT_ALL_BRIDGES_DESTROYED},
    {TEVENT_BUILDING_EXISTS},
}};

void EventChoiceClass::Draw_It(PixelView& view, int /*unused*/, int x, int y,
                               int width, int height, bool selected,
                               FontType font, TextPrintType flags) const {
  // Tab stops in pixels; the event names hold no tabs.
  static constexpr std::array<int, 2> kTabStops = {13, 40};
  UiSkin::Draw_List_Entry(view, Description(), x, y, width, height, font, flags,
                          selected, kTabStops);
}

void TEventClass::Reset(TDEventClass& td) const {
  td.IsTripped = false;
  if (Event == TEVENT_TIME) {
    td.EventTimer.Set(value() * (kTicksPerMinute / 10));
  }
}

bool TEventClass::operator()(TDEventClass& td, TEventType event,
                             HousesType house, const ObjectClass* object,
                             bool forced) const {
  if (forced) {
    td.IsTripped = true;
  }

  if (td.IsTripped) {
    return true;
  }

  // The game-state events are true while their condition is, never tripped:
  // a global that is cleared again, or a timer that is reset, turns them
  // false.
  switch (Event) {
    case TEVENT_GLOBAL_SET:
      return base::At(TheScenario().GlobalFlags, value());

    case TEVENT_GLOBAL_CLEAR:
      return !base::At(TheScenario().GlobalFlags, value());

    case TEVENT_MISSION_TIMER_EXPIRED:
      return TheScenario().MissionTimer.IsRunning() &&
             TheScenario().MissionTimer.IsFinished();

    case TEVENT_TIME:
      return td.EventTimer.IsFinished();
    case TEventType::TEVENT_NONE:
    case TEventType::TEVENT_PLAYER_ENTERED:
    case TEventType::TEVENT_SPIED:
    case TEventType::TEVENT_THIEVED:
    case TEventType::TEVENT_DISCOVERED:
    case TEventType::TEVENT_HOUSE_DISCOVERED:
    case TEventType::TEVENT_ATTACKED:
    case TEventType::TEVENT_DESTROYED:
    case TEventType::TEVENT_ANY:
    case TEventType::TEVENT_UNITS_DESTROYED:
    case TEventType::TEVENT_BUILDINGS_DESTROYED:
    case TEventType::TEVENT_ALL_DESTROYED:
    case TEventType::TEVENT_CREDITS:
    case TEventType::TEVENT_NBUILDINGS_DESTROYED:
    case TEventType::TEVENT_NUNITS_DESTROYED:
    case TEventType::TEVENT_NOFACTORIES:
    case TEventType::TEVENT_EVAC_CIVILIAN:
    case TEventType::TEVENT_BUILD:
    case TEventType::TEVENT_BUILD_UNIT:
    case TEventType::TEVENT_BUILD_INFANTRY:
    case TEventType::TEVENT_BUILD_AIRCRAFT:
    case TEventType::TEVENT_LEAVES_MAP:
    case TEventType::TEVENT_ENTERS_ZONE:
    case TEventType::TEVENT_CROSS_HORIZONTAL:
    case TEventType::TEVENT_CROSS_VERTICAL:
    case TEventType::TEVENT_FAKES_DESTROYED:
    case TEventType::TEVENT_LOW_POWER:
    case TEventType::TEVENT_ALL_BRIDGES_DESTROYED:
    case TEventType::TEVENT_BUILDING_EXISTS:
    default:
      break;
  }

  // An unused event never happens.
  if (Event == TEVENT_NONE) {
    return false;
  }

  // The moment events are true merely because the check was made for them:
  // the caller springs TEVENT_ATTACKED from the code that does the attacking.
  // TEVENT_ANY lets them through as well, so a house's per-frame check of a
  // trigger counts them as having happened.
  if ((Event == TEVENT_ATTACKED || Event == TEVENT_DESTROYED ||
       Event == TEVENT_DISCOVERED || Event == TEVENT_SPIED ||
       Event == TEVENT_NONE || Event == TEVENT_CROSS_HORIZONTAL ||
       Event == TEVENT_CROSS_VERTICAL || Event == TEVENT_ENTERS_ZONE ||
       Event == TEVENT_PLAYER_ENTERED) &&
      (event != Event && event != TEVENT_ANY)) {
    return false;
  }

  // The entering events also need the object that entered to belong to the
  // event's house; the trigger's own house does not matter.
  if (Event == TEVENT_PLAYER_ENTERED || Event == TEVENT_CROSS_HORIZONTAL ||
      Event == TEVENT_CROSS_VERTICAL || Event == TEVENT_ENTERS_ZONE) {
    if (!object || object->Owner() != this->house()) {
      return false;
    }
    td.IsTripped = true;
    return true;
  }

  // Tripped, so the event stays true if a bridge is repaired.
  if (Event == TEVENT_ALL_BRIDGES_DESTROYED) {
    if (TheScenario().BridgeCount) {
      return false;
    }
    td.IsTripped = true;
    return true;
  }

  // The events below check a house's state, first the trigger's own house's.
  // An event whose house cannot be found is not checked and counts as having
  // happened.
  const HouseClass* hptr = HouseClass::As_Pointer(house);
  int index = 0;
  if (hptr != nullptr) {
    switch (Event) {
      // A team springs the logic triggers itself once its last member has
      // left the map, just before it deletes itself, so it is still in the
      // heap here.
      case TEVENT_LEAVES_MAP:
        for (index = 0; index < TheObjectHeaps().team().Count(); index++) {
          const TeamClass* ptr = TheObjectHeaps().team().ActiveAt(index);
          if (ptr->Class == Team && ptr->Is_Empty() && ptr->IsLeaveMap) {
            td.IsTripped = true;
            break;
          }
        }
        if (index == TheObjectHeaps().team().Count()) {
          return false;
        }
        break;

      // The editor calls this "Credits exceed (x100)", but the value is in
      // credits.
      case TEVENT_CREDITS:
        if (hptr->Available_Money() < value()) {
          return false;
        }
        break;

      case TEVENT_NOFACTORIES:
        if (hptr->BScan &
            (kStructFlagAirstrip | kStructFlagTent | kStructFlagWeap |
             kStructFlagBarracks | kStructFlagConst)) {
          return false;
        }
        break;

      // Only two of the five fakes count: a fake shipyard, sub pen or radar
      // dome left standing does not stop the event.
      case TEVENT_FAKES_DESTROYED:
        if (hptr->BScan & (kStructFlagFakeConst | kStructFlagFakeWeap)) {
          return false;
        }
        break;

      case TEVENT_EVAC_CIVILIAN:
        if (!hptr->IsCivEvacuated) {
          return false;
        }
        break;

      // A structure past the 64 the scan holds never exists.
      case TEVENT_BUILDING_EXISTS:
        if ((hptr->ActiveBScan & ScanBit(static_cast<int>(structure()))) == 0) {
          return false;
        }
        break;

      // The house only remembers what it built last, so the building
      // events are tripped to outlast the next thing it builds.
      case TEVENT_BUILD:
        if (hptr->JustBuiltStructure != structure()) {
          return false;
        }
        td.IsTripped = true;
        break;

      case TEVENT_BUILD_UNIT:
        if (hptr->JustBuiltUnit != unit()) {
          return false;
        }
        td.IsTripped = true;
        break;

      case TEVENT_BUILD_INFANTRY:
        if (hptr->JustBuiltInfantry != infantry()) {
          return false;
        }
        td.IsTripped = true;
        break;

      case TEVENT_BUILD_AIRCRAFT:
        if (hptr->JustBuiltAircraft != aircraft()) {
          return false;
        }
        td.IsTripped = true;
        break;

      case TEVENT_NBUILDINGS_DESTROYED:
        if (std::cmp_less(hptr->BuildingsLost, value())) {
          return false;
        }
        break;

      case TEVENT_NUNITS_DESTROYED:
        if (std::cmp_less(hptr->UnitsLost, value())) {
          return false;
        }
        break;

      case TEventType::TEVENT_NONE:
      case TEventType::TEVENT_PLAYER_ENTERED:
      case TEventType::TEVENT_SPIED:
      case TEventType::TEVENT_THIEVED:
      case TEventType::TEVENT_DISCOVERED:
      case TEventType::TEVENT_HOUSE_DISCOVERED:
      case TEventType::TEVENT_ATTACKED:
      case TEventType::TEVENT_DESTROYED:
      case TEventType::TEVENT_ANY:
      case TEventType::TEVENT_UNITS_DESTROYED:
      case TEventType::TEVENT_BUILDINGS_DESTROYED:
      case TEventType::TEVENT_ALL_DESTROYED:
      case TEventType::TEVENT_TIME:
      case TEventType::TEVENT_MISSION_TIMER_EXPIRED:
      case TEventType::TEVENT_ENTERS_ZONE:
      case TEventType::TEVENT_CROSS_HORIZONTAL:
      case TEventType::TEVENT_CROSS_VERTICAL:
      case TEventType::TEVENT_GLOBAL_SET:
      case TEventType::TEVENT_GLOBAL_CLEAR:
      case TEventType::TEVENT_LOW_POWER:
      case TEventType::TEVENT_ALL_BRIDGES_DESTROYED:
      default:
        break;
    }
  }

  // Then the house the event names.
  hptr = HouseClass::As_Pointer(this->house());
  if (hptr) {
    switch (Event) {
      case TEVENT_LOW_POWER:
        if (hptr->Power_Fraction() >= 1) {
          return false;
        }
        break;

      case TEVENT_THIEVED:
        if (!hptr->IsThieved) {
          return false;
        }
        break;

      case TEVENT_HOUSE_DISCOVERED:
        if (!hptr->IsDiscovered) {
          return false;
        }
        break;

      case TEVENT_BUILDINGS_DESTROYED:
        if (hptr->ActiveBScan) {
          return false;
        }
        break;

      // Vessels and aircraft do not count.
      case TEVENT_UNITS_DESTROYED:
        if (hptr->ActiveUScan | hptr->ActiveIScan) {
          return false;
        }
        break;

      // Aircraft do not count.
      case TEVENT_ALL_DESTROYED:
        if (hptr->ActiveBScan | hptr->ActiveUScan | hptr->ActiveIScan |
            hptr->ActiveVScan) {
          return false;
        }
        break;

      case TEventType::TEVENT_NONE:
      case TEventType::TEVENT_PLAYER_ENTERED:
      case TEventType::TEVENT_SPIED:
      case TEventType::TEVENT_DISCOVERED:
      case TEventType::TEVENT_ATTACKED:
      case TEventType::TEVENT_DESTROYED:
      case TEventType::TEVENT_ANY:
      case TEventType::TEVENT_CREDITS:
      case TEventType::TEVENT_TIME:
      case TEventType::TEVENT_MISSION_TIMER_EXPIRED:
      case TEventType::TEVENT_NBUILDINGS_DESTROYED:
      case TEventType::TEVENT_NUNITS_DESTROYED:
      case TEventType::TEVENT_NOFACTORIES:
      case TEventType::TEVENT_EVAC_CIVILIAN:
      case TEventType::TEVENT_BUILD:
      case TEventType::TEVENT_BUILD_UNIT:
      case TEventType::TEVENT_BUILD_INFANTRY:
      case TEventType::TEVENT_BUILD_AIRCRAFT:
      case TEventType::TEVENT_LEAVES_MAP:
      case TEventType::TEVENT_ENTERS_ZONE:
      case TEventType::TEVENT_CROSS_HORIZONTAL:
      case TEventType::TEVENT_CROSS_VERTICAL:
      case TEventType::TEVENT_GLOBAL_SET:
      case TEventType::TEVENT_GLOBAL_CLEAR:
      case TEventType::TEVENT_FAKES_DESTROYED:
      case TEventType::TEVENT_ALL_BRIDGES_DESTROYED:
      case TEventType::TEVENT_BUILDING_EXISTS:
      default:
        break;
    }
  }

  return true;
}

std::string TEventClass::Build_INI_Entry() const {
  return std::format("{},{},{}", std::to_underlying(Event),
                     TheObjectHeaps().team_type().PositionOf(Team), value());
}

void TEventClass::Read_INI(base::Tokenizer& tokens) {
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
      Event = TEVENT_NONE;
      if (!token.empty()) {
        Event = EventFromNumber(base::ParseIntegerOr<int>(token, 0));
      }

      token = tokens.Next();
      Team = nullptr;
      set_value(-1);
      if (!token.empty()) {
        if (Event_Needs(Event) == NeedType::kTeam) {
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

NeedType Event_Needs(TEventType event) {
  switch (event) {
    case TEVENT_THIEVED:
    case TEVENT_PLAYER_ENTERED:
    case TEVENT_CROSS_HORIZONTAL:
    case TEVENT_CROSS_VERTICAL:
    case TEVENT_ENTERS_ZONE:
    case TEVENT_HOUSE_DISCOVERED:
    case TEVENT_BUILDINGS_DESTROYED:
    case TEVENT_UNITS_DESTROYED:
    case TEVENT_ALL_DESTROYED:
    case TEVENT_LOW_POWER:
      return NeedType::kHouse;

    case TEVENT_NUNITS_DESTROYED:
    case TEVENT_NBUILDINGS_DESTROYED:
    case TEVENT_CREDITS:
    case TEVENT_TIME:
    case TEVENT_GLOBAL_SET:
    case TEVENT_GLOBAL_CLEAR:
      return NeedType::kNumber;

    case TEVENT_BUILDING_EXISTS:
    case TEVENT_BUILD:
      return NeedType::kStructure;

    case TEVENT_BUILD_UNIT:
      return NeedType::kUnit;

    case TEVENT_BUILD_INFANTRY:
      return NeedType::kInfantry;

    case TEVENT_BUILD_AIRCRAFT:
      return NeedType::kAircraft;

    case TEVENT_LEAVES_MAP:
      return NeedType::kTeam;

    case TEventType::TEVENT_NONE:
    case TEventType::TEVENT_SPIED:
    case TEventType::TEVENT_DISCOVERED:
    case TEventType::TEVENT_ATTACKED:
    case TEventType::TEVENT_DESTROYED:
    case TEventType::TEVENT_ANY:
    case TEventType::TEVENT_MISSION_TIMER_EXPIRED:
    case TEventType::TEVENT_NOFACTORIES:
    case TEventType::TEVENT_EVAC_CIVILIAN:
    case TEventType::TEVENT_FAKES_DESTROYED:
    case TEventType::TEVENT_ALL_BRIDGES_DESTROYED:
    default:
      break;
  }
  return NeedType::kNone;
}

TEventType Event_From_Name(const std::string_view name) {
  if (!name.empty()) {
    for (const TEventType i : magic_enum::enum_values<TEventType>()) {
      if (absl::EqualsIgnoreCase(name, EventText.at(i))) {
        return i;
      }
    }
  }

  return TEVENT_NONE;
}

const char* Name_From_Event(TEventType event) { return EventText.at(event); }

AttachType Attaches_To(TEventType event) {
  AttachType attach = ATTACH_NONE;

  switch (event) {
    case TEVENT_CROSS_HORIZONTAL:
    case TEVENT_CROSS_VERTICAL:
    case TEVENT_ENTERS_ZONE:
    case TEVENT_PLAYER_ENTERED:
    case TEVENT_ANY:
    case TEVENT_DISCOVERED:
    case TEVENT_NONE:
      attach = attach | ATTACH_CELL;
      break;

    case TEventType::TEVENT_SPIED:
    case TEventType::TEVENT_THIEVED:
    case TEventType::TEVENT_HOUSE_DISCOVERED:
    case TEventType::TEVENT_ATTACKED:
    case TEventType::TEVENT_DESTROYED:
    case TEventType::TEVENT_UNITS_DESTROYED:
    case TEventType::TEVENT_BUILDINGS_DESTROYED:
    case TEventType::TEVENT_ALL_DESTROYED:
    case TEventType::TEVENT_CREDITS:
    case TEventType::TEVENT_TIME:
    case TEventType::TEVENT_MISSION_TIMER_EXPIRED:
    case TEventType::TEVENT_NBUILDINGS_DESTROYED:
    case TEventType::TEVENT_NUNITS_DESTROYED:
    case TEventType::TEVENT_NOFACTORIES:
    case TEventType::TEVENT_EVAC_CIVILIAN:
    case TEventType::TEVENT_BUILD:
    case TEventType::TEVENT_BUILD_UNIT:
    case TEventType::TEVENT_BUILD_INFANTRY:
    case TEventType::TEVENT_BUILD_AIRCRAFT:
    case TEventType::TEVENT_LEAVES_MAP:
    case TEventType::TEVENT_GLOBAL_SET:
    case TEventType::TEVENT_GLOBAL_CLEAR:
    case TEventType::TEVENT_FAKES_DESTROYED:
    case TEventType::TEVENT_LOW_POWER:
    case TEventType::TEVENT_ALL_BRIDGES_DESTROYED:
    case TEventType::TEVENT_BUILDING_EXISTS:
    default:
      break;
  }

  switch (event) {
    case TEVENT_SPIED:
    case TEVENT_PLAYER_ENTERED:
    case TEVENT_DISCOVERED:
    case TEVENT_DESTROYED:
    case TEVENT_ATTACKED:
    case TEVENT_ANY:
    case TEVENT_NONE:
      attach = attach | ATTACH_OBJECT;
      break;

    case TEventType::TEVENT_THIEVED:
    case TEventType::TEVENT_HOUSE_DISCOVERED:
    case TEventType::TEVENT_UNITS_DESTROYED:
    case TEventType::TEVENT_BUILDINGS_DESTROYED:
    case TEventType::TEVENT_ALL_DESTROYED:
    case TEventType::TEVENT_CREDITS:
    case TEventType::TEVENT_TIME:
    case TEventType::TEVENT_MISSION_TIMER_EXPIRED:
    case TEventType::TEVENT_NBUILDINGS_DESTROYED:
    case TEventType::TEVENT_NUNITS_DESTROYED:
    case TEventType::TEVENT_NOFACTORIES:
    case TEventType::TEVENT_EVAC_CIVILIAN:
    case TEventType::TEVENT_BUILD:
    case TEventType::TEVENT_BUILD_UNIT:
    case TEventType::TEVENT_BUILD_INFANTRY:
    case TEventType::TEVENT_BUILD_AIRCRAFT:
    case TEventType::TEVENT_LEAVES_MAP:
    case TEventType::TEVENT_ENTERS_ZONE:
    case TEventType::TEVENT_CROSS_HORIZONTAL:
    case TEventType::TEVENT_CROSS_VERTICAL:
    case TEventType::TEVENT_GLOBAL_SET:
    case TEventType::TEVENT_GLOBAL_CLEAR:
    case TEventType::TEVENT_FAKES_DESTROYED:
    case TEventType::TEVENT_LOW_POWER:
    case TEventType::TEVENT_ALL_BRIDGES_DESTROYED:
    case TEventType::TEVENT_BUILDING_EXISTS:
    default:
      break;
  }

  switch (event) {
    case TEVENT_ENTERS_ZONE:
    case TEVENT_ANY:
      attach = attach | ATTACH_MAP;
      break;

    case TEventType::TEVENT_NONE:
    case TEventType::TEVENT_PLAYER_ENTERED:
    case TEventType::TEVENT_SPIED:
    case TEventType::TEVENT_THIEVED:
    case TEventType::TEVENT_DISCOVERED:
    case TEventType::TEVENT_HOUSE_DISCOVERED:
    case TEventType::TEVENT_ATTACKED:
    case TEventType::TEVENT_DESTROYED:
    case TEventType::TEVENT_UNITS_DESTROYED:
    case TEventType::TEVENT_BUILDINGS_DESTROYED:
    case TEventType::TEVENT_ALL_DESTROYED:
    case TEventType::TEVENT_CREDITS:
    case TEventType::TEVENT_TIME:
    case TEventType::TEVENT_MISSION_TIMER_EXPIRED:
    case TEventType::TEVENT_NBUILDINGS_DESTROYED:
    case TEventType::TEVENT_NUNITS_DESTROYED:
    case TEventType::TEVENT_NOFACTORIES:
    case TEventType::TEVENT_EVAC_CIVILIAN:
    case TEventType::TEVENT_BUILD:
    case TEventType::TEVENT_BUILD_UNIT:
    case TEventType::TEVENT_BUILD_INFANTRY:
    case TEventType::TEVENT_BUILD_AIRCRAFT:
    case TEventType::TEVENT_LEAVES_MAP:
    case TEventType::TEVENT_CROSS_HORIZONTAL:
    case TEventType::TEVENT_CROSS_VERTICAL:
    case TEventType::TEVENT_GLOBAL_SET:
    case TEventType::TEVENT_GLOBAL_CLEAR:
    case TEventType::TEVENT_FAKES_DESTROYED:
    case TEventType::TEVENT_LOW_POWER:
    case TEventType::TEVENT_ALL_BRIDGES_DESTROYED:
    case TEventType::TEVENT_BUILDING_EXISTS:
    default:
      break;
  }

  switch (event) {
    case TEVENT_LOW_POWER:
    case TEVENT_EVAC_CIVILIAN:
    case TEVENT_BUILDING_EXISTS:
    case TEVENT_BUILD:
    case TEVENT_BUILD_UNIT:
    case TEVENT_BUILD_INFANTRY:
    case TEVENT_BUILD_AIRCRAFT:
    case TEVENT_NOFACTORIES:
    case TEVENT_BUILDINGS_DESTROYED:
    case TEVENT_NBUILDINGS_DESTROYED:
    case TEVENT_UNITS_DESTROYED:
    case TEVENT_NUNITS_DESTROYED:
    case TEVENT_ALL_DESTROYED:
    case TEVENT_HOUSE_DISCOVERED:
    case TEVENT_CREDITS:
    case TEVENT_THIEVED:
    case TEVENT_ANY:
    case TEVENT_FAKES_DESTROYED:
      attach = attach | ATTACH_HOUSE;
      break;

    case TEventType::TEVENT_NONE:
    case TEventType::TEVENT_PLAYER_ENTERED:
    case TEventType::TEVENT_SPIED:
    case TEventType::TEVENT_DISCOVERED:
    case TEventType::TEVENT_ATTACKED:
    case TEventType::TEVENT_DESTROYED:
    case TEventType::TEVENT_TIME:
    case TEventType::TEVENT_MISSION_TIMER_EXPIRED:
    case TEventType::TEVENT_LEAVES_MAP:
    case TEventType::TEVENT_ENTERS_ZONE:
    case TEventType::TEVENT_CROSS_HORIZONTAL:
    case TEventType::TEVENT_CROSS_VERTICAL:
    case TEventType::TEVENT_GLOBAL_SET:
    case TEventType::TEVENT_GLOBAL_CLEAR:
    case TEventType::TEVENT_ALL_BRIDGES_DESTROYED:
    default:
      break;
  }

  switch (event) {
    case TEVENT_TIME:
    case TEVENT_GLOBAL_SET:
    case TEVENT_GLOBAL_CLEAR:
    case TEVENT_MISSION_TIMER_EXPIRED:
    case TEVENT_ANY:
    case TEVENT_ALL_BRIDGES_DESTROYED:
    case TEVENT_LEAVES_MAP:
      attach = attach | ATTACH_GENERAL;
      break;

    case TEventType::TEVENT_NONE:
    case TEventType::TEVENT_PLAYER_ENTERED:
    case TEventType::TEVENT_SPIED:
    case TEventType::TEVENT_THIEVED:
    case TEventType::TEVENT_DISCOVERED:
    case TEventType::TEVENT_HOUSE_DISCOVERED:
    case TEventType::TEVENT_ATTACKED:
    case TEventType::TEVENT_DESTROYED:
    case TEventType::TEVENT_UNITS_DESTROYED:
    case TEventType::TEVENT_BUILDINGS_DESTROYED:
    case TEventType::TEVENT_ALL_DESTROYED:
    case TEventType::TEVENT_CREDITS:
    case TEventType::TEVENT_NBUILDINGS_DESTROYED:
    case TEventType::TEVENT_NUNITS_DESTROYED:
    case TEventType::TEVENT_NOFACTORIES:
    case TEventType::TEVENT_EVAC_CIVILIAN:
    case TEventType::TEVENT_BUILD:
    case TEventType::TEVENT_BUILD_UNIT:
    case TEventType::TEVENT_BUILD_INFANTRY:
    case TEventType::TEVENT_BUILD_AIRCRAFT:
    case TEventType::TEVENT_ENTERS_ZONE:
    case TEventType::TEVENT_CROSS_HORIZONTAL:
    case TEventType::TEVENT_CROSS_VERTICAL:
    case TEventType::TEVENT_FAKES_DESTROYED:
    case TEventType::TEVENT_LOW_POWER:
    case TEventType::TEVENT_BUILDING_EXISTS:
    default:
      break;
  }

  return attach;
}

}  // namespace ra
