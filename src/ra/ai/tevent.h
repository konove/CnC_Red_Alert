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

// The event half of a scenario trigger: what a trigger waits for
// (TEventType), an event as a trigger type describes it (TEventClass), the
// state each trigger keeps for it while it waits (TDEventClass), what an event
// can be attached to (AttachType), and the scenario editor's list entry for an
// event type (EventChoiceClass).

#ifndef CNC_RED_ALERT_RA_AI_TEVENT_H_
#define CNC_RED_ALERT_RA_AI_TEVENT_H_

#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>

#include "absl/strings/ascii.h"
#include "base/attributes.h"
#include "base/enum_array.h"
#include "base/enum_step.h"
#include "base/flags.h"
#include "base/strings/tokenizer.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/platform/timer.h"
#include "game/core/game_clock.h"
#include "ra/ai/team_type.h"
#include "ra/core/defines.h"
#include "ra/objects/object.h"

class ArchiveReader;
class ArchiveWriter;

namespace ra {

// What a trigger event waits for. Scenario INI files and saved games store the
// number, so the order is fixed. "The event's house" is the house the event
// names (TEventClass::house()); "the trigger's house" is the house of the
// trigger type the event belongs to.
enum class TEventType {
  TEVENT_NONE,  // Never happens; an unused second event.
  // An object of the event's house enters the cell or the object the trigger
  // is attached to.
  TEVENT_PLAYER_ENTERED,
  TEVENT_SPIED,  // A spy enters the building.
  // A thief has raided, or an engineer captured, a building of the event's
  // house.
  TEVENT_THIEVED,
  TEVENT_DISCOVERED,        // The player first sees the object.
  TEVENT_HOUSE_DISCOVERED,  // The player first sees the event's house.
  TEVENT_ATTACKED,          // The object is attacked.
  TEVENT_DESTROYED,         // The object is destroyed.
  // Happens on every check of the trigger, whatever caused it, and lets the
  // object events above through too.
  TEVENT_ANY,
  // The event's house has no unit and no infantry left (vessels and aircraft
  // do not count).
  TEVENT_UNITS_DESTROYED,
  TEVENT_BUILDINGS_DESTROYED,  // The event's house has no building left.
  // The event's house has no building, unit, infantry or vessel left
  // (aircraft do not count).
  TEVENT_ALL_DESTROYED,
  TEVENT_CREDITS,  // The trigger's house has at least value() credits.
  // value() tenths of a minute have passed since the trigger was made or
  // last reset.
  TEVENT_TIME,
  TEVENT_MISSION_TIMER_EXPIRED,  // The running mission timer reached zero.
  // The trigger's house has lost at least value() buildings.
  TEVENT_NBUILDINGS_DESTROYED,
  // The trigger's house has lost at least value() units.
  TEVENT_NUNITS_DESTROYED,
  // The trigger's house has no construction yard, war factory, barracks,
  // tent or airstrip left.
  TEVENT_NOFACTORIES,
  // The trigger's house has evacuated a civilian.
  TEVENT_EVAC_CIVILIAN,
  // The trigger's house has just built a structure() (unit(), infantry(),
  // aircraft()).
  TEVENT_BUILD,
  TEVENT_BUILD_UNIT,
  TEVENT_BUILD_INFANTRY,
  TEVENT_BUILD_AIRCRAFT,
  // An emptied team of the event's team type has left the map.
  TEVENT_LEAVES_MAP,
  // An object of the event's house enters the zone of the trigger's cell.
  TEVENT_ENTERS_ZONE,
  // An object of the event's house crosses the trigger's horizontal
  // (vertical) line.
  TEVENT_CROSS_HORIZONTAL,
  TEVENT_CROSS_VERTICAL,
  TEVENT_GLOBAL_SET,    // Scenario global value() is set.
  TEVENT_GLOBAL_CLEAR,  // Scenario global value() is clear.
  // The trigger's house has no fake construction yard or fake war factory
  // left.
  TEVENT_FAKES_DESTROYED,
  TEVENT_LOW_POWER,              // The event's house has less than full power.
  TEVENT_ALL_BRIDGES_DESTROYED,  // No bridge is left on the map.
  // The trigger's house has a working building of type structure().
  TEVENT_BUILDING_EXISTS,

  TEVENT_COUNT,
};
using enum TEventType;

// Returns the event whose editor name is `name`, ignoring case, or
// TEVENT_NONE when no event has it.
TEventType Event_From_Name(std::string_view name);
// Returns the kind of argument `event` takes, which the scenario editor
// prompts for; NeedType::kNone when it takes none. NeedType::kTeam means the
// event's team type, every other kind its data word.
NeedType Event_Needs(TEventType event);
// Returns the scenario editor's name for `event`.
const char* Name_From_Event(TEventType event);

// The state one trigger keeps for one of its events, which a trigger type's
// TEventClass reads and writes; TriggerClass holds one for each of its two
// events.
struct TDEventClass {
  // Set when an event that happens at a moment (a structure being built, an
  // object entering a cell, a forced spring) has happened, so the event keeps
  // counting on later checks until Reset() clears it. Without it a trigger
  // whose other event comes true later would miss the first.
  bool IsTripped : 1 = false;

  // Counts down to a TEVENT_TIME event; Reset() starts it.
  engine::platform::Countdown<game::FrameTickSource> EventTimer;

  // Saved-game support.
  template <class Archive>
  void Serialize(Archive& ar) {
    bool is_tripped = IsTripped;
    ar(is_tripped, EventTimer);
    IsTripped = is_tripped;
  }
};

// One of the two events of a trigger type: which event it is and its
// argument. Shared by every trigger of that type; the state of one trigger's
// wait is the TDEventClass passed to Reset() and operator().
//
// Example:
//   TEventClass event(TEVENT_CREDITS);
//   event.set_value(5000);
//   TDEventClass state;
//   event.Reset(state);
//   if (event(state, TEVENT_ANY, HOUSE_USSR, nullptr, false)) { ... }
class TEventClass {
 public:
  // The event this is.
  TEventType Event;

  // The team type a TEVENT_LEAVES_MAP event waits for; nullptr for none.
  TeamType* Team = nullptr;

  TEventClass() : Event(TEVENT_NONE) {}
  explicit TEventClass(TEventType event) : Event(event) {}

  // Saved-game support; defined in ioobj.cc.
  template <class Archive>
  void Serialize(Archive& ar);

  // Makes `td` wait for the event again: clears its tripped flag and, for a
  // TEVENT_TIME event, restarts its countdown. A trigger calls it when it is
  // made and after each spring of a persistent trigger, and the scenario when
  // a global the trigger's other event waits for changes.
  void Reset(TDEventClass& td) const;
  // Returns whether the event has happened for the trigger whose state is
  // `td`. `event` is what caused the check: the moment events (entering,
  // being attacked, crossing a line) only count when it is this event or
  // TEVENT_ANY, while the condition events (credits, globals, a house's
  // losses) are checked whatever it is. `house` is the trigger's house and
  // `object` the object that caused the check (for the entering events, the
  // one that entered); nullptr for none. `forced` trips the event without a
  // check, for a trigger another trigger forces.
  bool operator()(TDEventClass& td, TEventType event, HousesType house,
                  const ObjectClass* object, bool forced) const;
  // Reads the event fields from the trigger entry `tokens` is parsing, in the
  // scenario's INI format: from format 2 on "event,team,value" with the team
  // as its heap slot; format 1 "event,team-or-value"; format 0
  // "event,-,-,team name,value,-". An event number that names no event reads
  // as TEVENT_NONE.
  void Read_INI(base::Tokenizer& tokens);
  // Appends the event to `buffer` as "event,team,value", the form Read_INI()
  // reads from format 2 on, with the team as its position in the team type
  // list (-1 for none).
  void Build_INI_Entry(std::string& buffer) const;

  // The event's argument. Which accessor applies depends on Event: a building,
  // unit, infantry or aircraft type for the "built" events, a house for the
  // house events, and otherwise a number (credits, minutes, a global, a
  // count). All of them share one saved word.
  [[nodiscard]] StructType structure() const {
    return static_cast<StructType>(data_);
  }
  void set_structure(const StructType structure) {
    data_ = static_cast<int32_t>(structure);
  }
  [[nodiscard]] UnitType unit() const { return static_cast<UnitType>(data_); }
  void set_unit(const UnitType unit) { data_ = static_cast<int32_t>(unit); }
  [[nodiscard]] InfantryType infantry() const {
    return static_cast<InfantryType>(data_);
  }
  void set_infantry(const InfantryType infantry) {
    data_ = static_cast<int32_t>(infantry);
  }
  [[nodiscard]] AircraftType aircraft() const {
    return static_cast<AircraftType>(data_);
  }
  void set_aircraft(const AircraftType aircraft) {
    data_ = static_cast<int32_t>(aircraft);
  }
  // HousesType is one byte, the word's lowest. Storing one keeps the other
  // three, as the store into the union this replaced did, so the value a
  // [Trigs] entry saves does not change.
  [[nodiscard]] HousesType house() const {
    return static_cast<HousesType>(static_cast<int8_t>(data_));
  }
  void set_house(const HousesType house) {
    data_ = static_cast<int32_t>((static_cast<uint32_t>(data_) & ~0xFFU) |
                                 static_cast<uint8_t>(house));
  }
  [[nodiscard]] int32_t value() const { return data_; }
  void set_value(const int32_t value) { data_ = value; }

 private:
  int32_t data_ = 0;
};

// What a trigger can be attached to, as bits: an event can go on several
// kinds of thing, and a trigger type allows whatever either of its events
// allows. The scenario puts a trigger with the map, general or house bit on
// the list that checks it; the editor places one with the cell or object bit
// on what the player clicks.
enum class CNC_FLAG_ENUM AttachType {
  ATTACH_NONE = 0x00,  // Attaches to nothing (an orphan trigger type).
  ATTACH_CELL = 0x01,  // A cell.
  // An object, usually a building or a vehicle.
  ATTACH_OBJECT = 0x02,
  // The whole map, checked whenever an object moves (the zone event).
  ATTACH_MAP = 0x04,
  // The trigger's house, checked every frame with the house's other triggers.
  ATTACH_HOUSE = 0x08,
  // The game state, checked every frame (time, globals, the mission timer,
  // the bridges).
  ATTACH_GENERAL = 0x10,
  ATTACH_TEAM = 0x20  // A team.
};
using enum AttachType;

// Returns what a trigger with `event` can be attached to.
AttachType Attaches_To(TEventType event);

// An event type as an entry of the scenario editor's event lists, which shows
// its name and sorts by it.
class EventChoiceClass {
 public:
  // Implicit, so the choice table can be a brace list of event types.
  // NOLINTNEXTLINE(*-explicit-constructor)
  EventChoiceClass(TEventType event = TEVENT_NONE) noexcept : Event(event) {}

  // Implicit, so an entry compares directly against an event type.
  // NOLINTNEXTLINE(*-explicit-constructor)
  operator TEventType() const { return Event; }
  bool operator==(const EventChoiceClass& rvalue) const {
    return Event == rvalue.Event;
  }
  bool operator!=(const EventChoiceClass& rvalue) const {
    return Event != rvalue.Event;
  }
  bool operator>(const EventChoiceClass& rvalue) const {
    return rvalue < *this;
  }
  // Orders by description, ignoring case.
  bool operator<(const EventChoiceClass& rvalue) const {
    return std::ranges::lexicographical_compare(
        std::string_view(Description()), std::string_view(rvalue.Description()),
        {}, absl::ascii_tolower, absl::ascii_tolower);
  }
  bool operator<=(const EventChoiceClass& rvalue) const {
    return Event == rvalue.Event || *this < rvalue;
  }
  bool operator>=(const EventChoiceClass& rvalue) const {
    return Event == rvalue.Event || rvalue < *this;
  }
  // The event's name in the editor.
  [[nodiscard]] const char* Description() const {
    return Name_From_Event(Event);
  }
  // Draws the event's name as a list box line; `index` is unused.
  void Draw_It(PixelView& view, int index, int x, int y, int width, int height,
               bool selected, FontType font, TextPrintType flags) const;

  TEventType Event;
};

// One entry per event type, indexed by it, for the editor's event lists.
extern base::EnumArray<TEventType, EventChoiceClass,
                       static_cast<int>(TEVENT_COUNT)>
    EventChoices;

extern template void TEventClass::Serialize<ArchiveWriter>(ArchiveWriter&);
extern template void TEventClass::Serialize<ArchiveReader>(ArchiveReader&);

}  // namespace ra

template <>
inline constexpr bool base::kIsSteppableEnum<ra::TEventType> = true;
template <>
inline constexpr bool base::kIsFlagEnum<ra::AttachType> = true;

#endif  // CNC_RED_ALERT_RA_AI_TEVENT_H_
