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
// (TriggerEventType), an event as a trigger type describes it (TriggerEvent),
// the state each trigger keeps for it while it waits (TriggerEventState), and
// what an event can be attached to (TriggerAttachment).

#ifndef CNC_RED_ALERT_RA_AI_TEVENT_H_
#define CNC_RED_ALERT_RA_AI_TEVENT_H_

#include <cstdint>
#include <string>

#include "base/attributes.h"
#include "base/flags.h"
#include "engine/platform/timer.h"
#include "game/core/game_clock.h"
#include "ra/core/defines.h"
#include "ra/core/target.h"

class ArchiveReader;
class ArchiveWriter;

namespace base {
class Tokenizer;
}  // namespace base

namespace ra {

class ObjectClass;
class TeamType;

// What a trigger event waits for. Scenario INI files and saved games store the
// number, so the order is fixed. "The event's house" is the house the event
// names (TriggerEvent::house()); "the trigger's house" is the house of the
// trigger type the event belongs to.
enum class TriggerEventType {
  kNone,  // Never happens; an unused second event.
  // An object of the event's house enters the cell or the object the trigger
  // is attached to.
  kPlayerEntered,
  kSpied,  // A spy enters the building.
  // A thief has raided, or an engineer captured, a building of the event's
  // house.
  kThieved,
  kDiscovered,       // The player first sees the object.
  kHouseDiscovered,  // The player first sees the event's house.
  kAttacked,         // The object is attacked.
  kDestroyed,        // The object is destroyed.
  // Happens on every check of the trigger, whatever caused it, and lets the
  // object events above through too.
  kAny,
  // The event's house has no unit and no infantry left (vessels and aircraft
  // do not count).
  kUnitsDestroyed,
  kBuildingsDestroyed,  // The event's house has no building left.
  // The event's house has no building, unit, infantry or vessel left
  // (aircraft do not count).
  kAllDestroyed,
  kCredits,  // The trigger's house has at least value() credits.
  // value() tenths of a minute have passed since the trigger was made or
  // last reset.
  kTime,
  kMissionTimerExpired,  // The running mission timer reached zero.
  // The trigger's house has lost at least value() buildings.
  kBuildingsLost,
  // The trigger's house has lost at least value() units.
  kUnitsLost,
  // The trigger's house has no construction yard, war factory, barracks,
  // tent or airstrip left.
  kNoFactories,
  // The trigger's house has evacuated a civilian.
  kCivilianEvacuated,
  // The trigger's house has just built a structure() (unit(), infantry(),
  // aircraft()).
  kBuildBuilding,
  kBuildUnit,
  kBuildInfantry,
  kBuildAircraft,
  // An emptied team of the event's team type has left the map.
  kLeavesMap,
  // An object of the event's house enters the zone of the trigger's cell.
  kEntersZone,
  // An object of the event's house crosses the trigger's horizontal
  // (vertical) line.
  kCrossHorizontal,
  kCrossVertical,
  kGlobalSet,    // Scenario global value() is set.
  kGlobalClear,  // Scenario global value() is clear.
  // The trigger's house has no fake construction yard or fake war factory
  // left.
  kFakesDestroyed,
  kLowPower,             // The event's house has less than full power.
  kAllBridgesDestroyed,  // No bridge is left on the map.
  // The trigger's house has a working building of type structure().
  kBuildingExists,
};

// Returns the kind of argument `event` takes, which the scenario editor
// prompts for; NeedType::kNone when it takes none. NeedType::kTeam means the
// event's team type, every other kind its data word.
NeedType EventNeed(TriggerEventType event);
// Returns the scenario editor's name for `event`.
const char* EventName(TriggerEventType event);

// The state one trigger keeps for one of its events, which a trigger type's
// TriggerEvent reads and writes; TriggerClass holds one for each of its two
// events.
struct TriggerEventState {
  // Set when an event that happens at a moment (a structure being built, an
  // object entering a cell, a forced spring) has happened, so the event keeps
  // counting on later checks until Reset() clears it. Without it a trigger
  // whose other event comes true later would miss the first.
  bool is_tripped = false;

  // Counts down to a TriggerEventType::kTime event; Reset() starts it.
  engine::platform::Countdown<game::FrameTickSource> timer;

  // Saved-game support.
  template <class Archive>
  void Serialize(Archive& ar) {
    ar(is_tripped, timer);
  }
};

// One of the two events of a trigger type: which event it is and its
// argument. Shared by every trigger of that type; the state of one trigger's
// wait is the TriggerEventState passed to Reset() and HasHappened().
//
// Example:
//   TriggerEvent event(TriggerEventType::kCredits);
//   event.set_value(5000);
//   TriggerEventState state;
//   event.Reset(state);
//   if (event.HasHappened(state, TriggerEventType::kAny, HOUSE_USSR, nullptr,
//                         false)) { ...
//   }
class TriggerEvent {
 public:
  TriggerEvent() = default;
  explicit TriggerEvent(TriggerEventType type) : type_(type) {}

  // Saved-game support; defined in ioobj.cc.
  template <class Archive>
  void Serialize(Archive& ar);

  // Makes `state` wait for the event again: clears its tripped flag and, for a
  // TriggerEventType::kTime event, restarts its countdown. A trigger calls it
  // when it is made and after each spring of a persistent trigger, and the
  // scenario when a global the trigger's other event waits for changes.
  void Reset(TriggerEventState& state) const;
  // Returns whether the event has happened for the trigger whose state is
  // `state`. `cause` is what caused the check: the moment events (entering,
  // being attacked, crossing a line) only count when it is this event or
  // TriggerEventType::kAny, while the condition events (credits, globals, a
  // house's losses) are checked whatever it is. `trigger_house` is the
  // trigger's house and `object` the object that caused the check (for the
  // entering events, the one that entered); nullptr for none. `forced` trips
  // the event without a check, for a trigger another trigger forces.
  // Drops the event's reference to the team type `target`, which is being
  // destroyed. Does nothing for any other target.
  void Detach(Target target);
  bool HasHappened(TriggerEventState& state, TriggerEventType cause,
                   HousesType trigger_house, const ObjectClass* object,
                   bool forced) const;
  // Reads the event fields from the trigger entry `tokens` is parsing, in the
  // scenario's INI format: from format 2 on "event,team,value" with the team
  // as its heap slot; format 1 "event,team-or-value"; format 0
  // "event,-,-,team name,value,-". An event number that names no event reads
  // as TriggerEventType::kNone, and a team slot outside the heap as no team.
  void ReadIni(base::Tokenizer& tokens);
  // Returns the event as "event,team,value", the form ReadIni() reads from
  // format 2 on, with the team as its position in the team type list (-1 for
  // none).
  [[nodiscard]] std::string ToIniEntry() const;

  // The event's argument. Which accessor applies depends on type(): a building,
  // unit, infantry or aircraft type for the "built" events, a house for the
  // house events, and otherwise a number (credits, minutes, a global, a
  // count). All of them share one saved word.
  [[nodiscard]] StructType structure() const {
    return static_cast<StructType>(argument_);
  }
  void set_structure(const StructType structure) {
    argument_ = static_cast<int32_t>(structure);
  }
  [[nodiscard]] UnitType unit() const {
    return static_cast<UnitType>(argument_);
  }
  void set_unit(const UnitType unit) { argument_ = static_cast<int32_t>(unit); }
  [[nodiscard]] InfantryType infantry() const {
    return static_cast<InfantryType>(argument_);
  }
  void set_infantry(const InfantryType infantry) {
    argument_ = static_cast<int32_t>(infantry);
  }
  [[nodiscard]] AircraftType aircraft() const {
    return static_cast<AircraftType>(argument_);
  }
  void set_aircraft(const AircraftType aircraft) {
    argument_ = static_cast<int32_t>(aircraft);
  }
  // HousesType is one byte, the word's lowest. Storing one keeps the other
  // three, as the store into the union this replaced did, so the value a
  // [Trigs] entry saves does not change.
  [[nodiscard]] HousesType house() const {
    return static_cast<HousesType>(static_cast<int8_t>(argument_));
  }
  void set_house(const HousesType house) {
    argument_ =
        static_cast<int32_t>((static_cast<uint32_t>(argument_) & ~0xFFU) |
                             static_cast<uint8_t>(house));
  }
  [[nodiscard]] int32_t value() const { return argument_; }
  void set_value(const int32_t value) { argument_ = value; }

  // The event this is.
  [[nodiscard]] TriggerEventType type() const { return type_; }
  void set_type(const TriggerEventType type) { type_ = type; }

  // The team type a TriggerEventType::kLeavesMap event waits for; nullptr for
  // none.
  [[nodiscard]] TeamType* team_type() const { return team_type_; }
  void set_team_type(TeamType* const team_type) { team_type_ = team_type; }

 private:
  TriggerEventType type_ = TriggerEventType::kNone;
  TeamType* team_type_ = nullptr;
  int32_t argument_ = 0;
};

// What a trigger can be attached to, as bits: an event can go on several
// kinds of thing, and a trigger type allows whatever either of its events
// allows. The scenario puts a trigger with the map, general or house bit on
// the list that checks it; the editor places one with the cell or object bit
// on what the player clicks.
enum class CNC_FLAG_ENUM TriggerAttachment {
  kNone = 0x00,  // Attaches to nothing (an orphan trigger type).
  kCell = 0x01,  // A cell.
  // An object, usually a building or a vehicle.
  kObject = 0x02,
  // The whole map, checked whenever an object moves (the zone event).
  kMap = 0x04,
  // The trigger's house, checked every frame with the house's other triggers.
  kHouse = 0x08,
  // The game state, checked every frame (time, globals, the mission timer,
  // the bridges).
  kGeneral = 0x10,
};

// Returns what a trigger with `event` can be attached to.
TriggerAttachment EventAttachment(TriggerEventType event);

extern template void TriggerEvent::Serialize<ArchiveWriter>(ArchiveWriter&);
extern template void TriggerEvent::Serialize<ArchiveReader>(ArchiveReader&);

}  // namespace ra

template <>
inline constexpr bool base::kIsFlagEnum<ra::TriggerAttachment> = true;

#endif  // CNC_RED_ALERT_RA_AI_TEVENT_H_
