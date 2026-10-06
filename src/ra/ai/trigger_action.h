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

// The action half of a scenario trigger: what happens when the trigger springs
// (TriggerAction), the kinds of action there are (TriggerActionType), and an
// action as the scenario INI, a saved game and the editor store it
// (TriggerActionRecord).

#ifndef CNC_RED_ALERT_RA_AI_TRIGGER_ACTION_H_
#define CNC_RED_ALERT_RA_AI_TRIGGER_ACTION_H_

#include <memory>
#include <string>

#include "base/strings/tokenizer.h"
#include "ra/core/cell.h"
#include "ra/core/defines.h"
#include "ra/core/target.h"

class ArchiveReader;
class ArchiveWriter;

namespace ra {

class ObjectClass;
class TeamType;
class TriggerType;

// What a trigger action does. Scenario INI files and saved games store the
// number, so the order is fixed.
enum class TriggerActionType {
  kNone,

  kWin,              // player wins!
  kLose,             // player loses.
  kBeginProduction,  // computer begins factory production.
  kCreateTeam,       // computer creates a certain type of team
  kDestroyTeam,
  kAllHunt,         // all enemy units go into hunt mode (teams destroyed).
  kReinforcements,  // The team type arrives as reinforcements, for
                    // the house the team type names.
  kDropZoneFlare,   // Deploy drop zone smoke.
  kFireSale,        // Sell all buildings and go on rampage.
  kPlayMovie,       // Play movie (temporarily suspend game).
  kTextTrigger,     // Triggers a text message display.
  kDestroyTrigger,  // Destroy specified trigger.
  kAutocreate,      // Computer to autocreate teams.
  kWinLose,         // Unused ("~don't use~" in the editor); was win
                    // if captured, lose if destroyed.
  kAllowWin,        // The house cannot win until this has sprung.

  kRevealAll,     // Reveal the entire map.
  kRevealSome,    // Reveal map around cell #.
  kRevealZone,    // Reveal all of specified zone.
  kPlaySound,     // Play sound effect.
  kPlayMusic,     // Play musical score.
  kPlaySpeech,    // Play EVA speech.
  kForceTrigger,  // Force trigger to activate.
  kStartTimer,    // Start mission timer.
  kStopTimer,     // Stop mission timer.
  kAddTimer,      // Increase mission timer time.
  kSubTimer,      // Decrease mission timer time.
  kSetTimer,      // Set and start the mission timer.
  kSetGlobal,     // Set global variable.
  kClearGlobal,   // Clear global variable.
  kBaseBuilding,  // Automated base building.
  kCreepShadow,   // Shadow grows back one 'step'.

  kDestroyObject,  // Destroys every object this trigger is attached to.
  kOneSpecial,     // Add a one-time special weapon ability to house.
  kFullSpecial,    // Add a repeating special weapon ability to house.

  kPreferredTarget,  // Designates preferred target for house.
  kLaunchNukes,      // Launch fake nuclear missiles from all silos
};

// Returns the scenario editor's name for `action`.
const char* ActionName(TriggerActionType action);

// Returns the kind of value `action` takes in TriggerActionRecord::value, which
// the scenario editor prompts for; NEED_NONE when it takes none. NEED_TEAM and
// NEED_TRIGGER mean the record's team_type or trigger_type instead.
NeedType ActionNeed(TriggerActionType action);

// What an action is told when its trigger springs.
struct TriggerActionContext {
  // The trigger's owner, which the house actions that name no house of their
  // own act on.
  HousesType house = HOUSE_NONE;
  // What the trigger was attached to and sprang on; nullptr for none.
  ObjectClass* object = nullptr;
  // The springing trigger's slot in the trigger heap; -1 for none.
  int trigger_id = -1;
  // The cell the trigger is attached to; cell 0 for none.
  Cell cell{};
};

// An action as the scenario INI, a saved game and the editor see it: the same
// four fields whatever the action is. ActionNeed() says which of the last three
// `type` uses.
struct TriggerActionRecord {
  TriggerActionType type = TriggerActionType::kNone;
  TeamType* team_type = nullptr;        // nullptr for none.
  TriggerType* trigger_type = nullptr;  // nullptr for none.
  // The house, special weapon, movie, sound, theme, speech, quarry, flag,
  // waypoint or number the action takes, as its number; -1 when unset.
  int value = -1;

  friend bool operator==(const TriggerActionRecord&,
                         const TriggerActionRecord&) = default;

  // Saved-game support; defined in ioobj.cc. The two types travel as their
  // heap slots.
  template <class Archive>
  void Serialize(Archive& ar);
};

// Reads a record from the trigger entry `tokens` is parsing: the action
// number, the team, the trigger and the value. The counterpart of
// ToIniEntry(). Every team type and trigger type of the scenario must exist
// already: INI formats 0 and 1 name them, and the later formats give a heap
// slot. A name nothing has, or a slot the scenario never filled, reads as
// none.
[[nodiscard]] TriggerActionRecord ReadTriggerActionRecord(
    base::Tokenizer& tokens);

// Returns `record` in the scenario INI's current format:
// "action,team,trigger,value", the middle two as positions in their type
// lists (-1 for none).
[[nodiscard]] std::string ToIniEntry(const TriggerActionRecord& record);

// One action of a trigger type, which the trigger type carries out with
// Perform() when it springs. Each kind of action is a class of its own in
// trigger_action.cc; MakeTriggerAction() makes them.
//
// Example:
//   std::unique_ptr<TriggerAction> action = MakeTriggerAction(
//       {.type = TriggerActionType::kSetTimer, .value = 20});
//   action->Perform({.house = HOUSE_USSR});
class TriggerAction {
 public:
  TriggerAction(const TriggerAction&) = delete;
  TriggerAction& operator=(const TriggerAction&) = delete;
  TriggerAction(TriggerAction&&) = delete;
  TriggerAction& operator=(TriggerAction&&) = delete;
  virtual ~TriggerAction() = default;

  // The action this is.
  [[nodiscard]] virtual TriggerActionType type() const = 0;

  // Carries out the action for the trigger that sprang. Returns false when
  // the action could not be done (no reinforcements could be created, nothing
  // to destroy), so the trigger is not counted as used.
  virtual bool Perform(const TriggerActionContext& context) = 0;

  // Drops the action's reference to the team type or trigger type `target`,
  // which is being destroyed. Does nothing for an action that refers to
  // neither.
  virtual void Detach(Target target);

  // Returns the action as a record; MakeTriggerAction() makes an equal action
  // from it. A field the action does not use is at its default, and the value
  // is the number of what the action acts on: 1 for the nuke, where a shipped
  // scenario may have said -255.
  [[nodiscard]] virtual TriggerActionRecord ToRecord() const = 0;

 protected:
  TriggerAction() = default;
};

// Returns the action `record` describes; never null. A team, trigger or value
// the action's type does not use is ignored. The house, special weapon, movie,
// speech and flag actions take the low byte of the value, which is where the
// shipped scenarios keep it (-255 is the nuke, movie -170 is movie 86), and a
// sound action takes the low byte when the value is no sound and the byte is
// one. A type number that is no action makes an action that does nothing.
[[nodiscard]] std::unique_ptr<TriggerAction> MakeTriggerAction(
    const TriggerActionRecord& record);

extern template void TriggerActionRecord::Serialize<ArchiveWriter>(
    ArchiveWriter&);
extern template void TriggerActionRecord::Serialize<ArchiveReader>(
    ArchiveReader&);

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_AI_TRIGGER_ACTION_H_
