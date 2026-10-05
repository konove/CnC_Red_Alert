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
// (TriggerAction) and the kinds of action there are (TriggerActionType).

#ifndef CNC_RED_ALERT_RA_AI_TRIGGER_ACTION_H_
#define CNC_RED_ALERT_RA_AI_TRIGGER_ACTION_H_

#include <string>

#include "base/strings/tokenizer.h"
#include "ra/core/cell.h"
#include "ra/core/defines.h"
#include "ra/core/target.h"

class ArchiveReader;
class ArchiveWriter;

namespace ra {

class ObjectClass;
class TeamTypeClass;
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

// Returns the kind of value `action` takes in TriggerAction::data, which the
// scenario editor prompts for; NEED_NONE when it takes none. NEED_TEAM and
// NEED_TRIGGER mean the team_type or trigger_type member instead.
NeedType ActionNeed(TriggerActionType action);

// One action of a trigger type: the action and the data it needs. A trigger
// type holds two, and carries them out with Perform() when it springs.
struct TriggerAction {
  TriggerActionType type{TriggerActionType::kNone};  // Action to perform.

  TeamTypeClass* team_type = nullptr;  // Team type for this action (if needed).

  // Trigger type for this action (if needed).
  TriggerType* trigger_type = nullptr;

  // The action's value, read through the member ActionNeed() names. The INI
  // file and saved games store `value`; the one-byte members read its low byte,
  // which is what the shipped scenarios rely on (see trigger_action_test.cc).
  union {
    ThemeType theme;            // Musical theme.
    VocType sound;              // Sound effect.
    VoxType speech;             // Speech identifier.
    HousesType house;           // House to be affected.
    SpecialWeaponType special;  // Special weapon ability.
    QuarryType quarry;          // Preferred target for attack.
    VQType movie;               // The movie to play.
    bool flag;                  // Boolean value.
    int value;  // Waypoint, global, text ID or timer tenths of a
                // minute; -1 when unset.
  } data{.value = -1};

  // Trigger name read from a pre-version-2 INI, where triggers may name ones
  // that load later. TriggerType::ReadIni resolves it into trigger_type
  // once every trigger exists; empty otherwise. Not saved.
  std::string pending_trigger_name;

  // Clears team_type or trigger_type when `target` is the team type or trigger
  // type it points to; called when that type is being destroyed.
  void Detach(Target target);

  // Saved-game support; defined in ioobj.cc. The union travels through its
  // widest member.
  template <class Archive>
  void Serialize(Archive& ar);

  // Reads the action fields from the trigger entry `tokens` is parsing: the
  // action number, the team, the trigger and the value. The counterpart of
  // ToIniEntry().
  void ReadIni(base::Tokenizer& tokens);

  // Returns the action in the scenario INI's current format:
  // "action,team,trigger,value", the middle two as positions in their type
  // lists (-1 for none).
  [[nodiscard]] std::string ToIniEntry() const;

  // Carries out the action for the trigger that sprang. `house` is the
  // trigger's owner, which the house actions without a data.house act on.
  // `object` is what the trigger was attached to and sprang on, or nullptr.
  // `trigger_id` is the springing trigger's slot in the trigger heap, or -1 for
  // none. `cell` is the cell the trigger is attached to; cell 0 means none.
  // Returns false when the action could not be done (no reinforcements could
  // be created, nothing to destroy), so the trigger is not counted as used.
  bool Perform(HousesType house, ObjectClass* object, int trigger_id,
               Cell cell);
};

extern template void TriggerAction::Serialize<ArchiveWriter>(ArchiveWriter&);
extern template void TriggerAction::Serialize<ArchiveReader>(ArchiveReader&);

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_AI_TRIGGER_ACTION_H_
