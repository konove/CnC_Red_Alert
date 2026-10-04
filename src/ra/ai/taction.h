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

#ifndef CNC_RED_ALERT_RA_AI_TACTION_H_
#define CNC_RED_ALERT_RA_AI_TACTION_H_

#include <algorithm>
#include <string>
#include <string_view>

#include "absl/strings/ascii.h"
#include "base/enum_array.h"
#include "base/strings/tokenizer.h"
#include "engine/gfx/pixel_buffer.h"
#include "ra/ai/teamtype.h"
#include "ra/core/defines.h"
#include "ra/objects/object.h"

class ArchiveReader;
class ArchiveWriter;

namespace ra {

// What a trigger action does. Scenario INI files and saved games store the
// number, so the order is fixed.
enum class TriggerActionType {
  TACTION_NONE,

  TACTION_WIN,               // player wins!
  TACTION_LOSE,              // player loses.
  TACTION_BEGIN_PRODUCTION,  // computer begins factory production.
  TACTION_CREATE_TEAM,       // computer creates a certain type of team
  TACTION_DESTROY_TEAM,
  TACTION_ALL_HUNT,  // all enemy units go into hunt mode (teams destroyed).
  TACTION_REINFORCEMENTS,   // The team type arrives as reinforcements, for
                            // the house the team type names.
  TACTION_DZ,               // Deploy drop zone smoke.
  TACTION_FIRE_SALE,        // Sell all buildings and go on rampage.
  TACTION_PLAY_MOVIE,       // Play movie (temporarily suspend game).
  TACTION_TEXT_TRIGGER,     // Triggers a text message display.
  TACTION_DESTROY_TRIGGER,  // Destroy specified trigger.
  TACTION_AUTOCREATE,       // Computer to autocreate teams.
  TACTION_WINLOSE,          // Unused ("~don't use~" in the editor); was win
                            // if captured, lose if destroyed.
  TACTION_ALLOWWIN,         // The house cannot win until this has sprung.

  TACTION_REVEAL_ALL,     // Reveal the entire map.
  TACTION_REVEAL_SOME,    // Reveal map around cell #.
  TACTION_REVEAL_ZONE,    // Reveal all of specified zone.
  TACTION_PLAY_SOUND,     // Play sound effect.
  TACTION_PLAY_MUSIC,     // Play musical score.
  TACTION_PLAY_SPEECH,    // Play EVA speech.
  TACTION_FORCE_TRIGGER,  // Force trigger to activate.
  TACTION_START_TIMER,    // Start mission timer.
  TACTION_STOP_TIMER,     // Stop mission timer.
  TACTION_ADD_TIMER,      // Increase mission timer time.
  TACTION_SUB_TIMER,      // Decrease mission timer time.
  TACTION_SET_TIMER,      // Set and start the mission timer.
  TACTION_SET_GLOBAL,     // Set global variable.
  TACTION_CLEAR_GLOBAL,   // Clear global variable.
  TACTION_BASE_BUILDING,  // Automated base building.
  TACTION_CREEP_SHADOW,   // Shadow grows back one 'step'.

  TACTION_DESTROY_OBJECT,  // Destroys every object this trigger is attached to.
  TACTION_1_SPECIAL,       // Add a one-time special weapon ability to house.
  TACTION_FULL_SPECIAL,    // Add a repeating special weapon ability to house.

  TACTION_PREFERRED_TARGET,  // Designates preferred target for house.
  TACTION_LAUNCH_NUKES,      // Launch fake nuclear missiles from all silos
};
using enum TriggerActionType;

// Returns the scenario editor's name for `action`.
const char* ActionName(TriggerActionType action);

// Returns the kind of value `action` takes in TriggerAction::data, which the
// scenario editor prompts for; NEED_NONE when it takes none. NEED_TEAM and
// NEED_TRIGGER mean the team_type or trigger_type member instead.
NeedType ActionNeed(TriggerActionType action);

// One action of a trigger type: the action and the data it needs. A trigger
// type holds two, and carries them out with Perform() when it springs.
struct TriggerAction {
  TriggerActionType type{TACTION_NONE};  // Action to perform.

  TeamTypeClass* team_type = nullptr;  // Team type for this action (if needed).

  // Trigger type for this action (if needed).
  TriggerTypeClass* trigger_type = nullptr;

  // The action's value, read through the member ActionNeed() names. The INI
  // file and saved games store `value`; the one-byte members read its low byte,
  // which is what the shipped scenarios rely on (see taction_test.cc).
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
  // that load later. TriggerTypeClass::Read_INI resolves it into trigger_type
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

#endif  // CNC_RED_ALERT_RA_AI_TACTION_H_
