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
  TeamTypeClass* team_type = nullptr;   // nullptr for none.
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

// One action of a trigger type: the action and the data it needs. A trigger
// type holds two, and carries them out with Perform() when it springs.
class TriggerAction {
 public:
  TriggerAction() = default;

  // Makes the action `record` describes.
  explicit TriggerAction(const TriggerActionRecord& record)
      : type_(record.type),
        team_type_(record.team_type),
        trigger_type_(record.trigger_type),
        data_{.value = record.value} {}

  // Returns the action as a record; TriggerAction(record) makes an equal
  // action from it.
  [[nodiscard]] TriggerActionRecord ToRecord() const {
    return {.type = type_,
            .team_type = team_type_,
            .trigger_type = trigger_type_,
            .value = data_.value};
  }

  // The action to perform.
  [[nodiscard]] TriggerActionType type() const { return type_; }
  void set_type(const TriggerActionType type) { type_ = type; }

  // The team type the action is for; nullptr when it needs none or the
  // scenario named one that does not exist.
  [[nodiscard]] TeamTypeClass* team_type() const { return team_type_; }
  void set_team_type(TeamTypeClass* team_type) { team_type_ = team_type; }

  // The trigger type the action is for; nullptr when it needs none.
  [[nodiscard]] TriggerType* trigger_type() const { return trigger_type_; }
  void set_trigger_type(TriggerType* trigger_type) {
    trigger_type_ = trigger_type;
  }

  // The action's value, read through the accessor ActionNeed() names. They
  // share storage: the INI file and saved games store value(), and the others
  // read and write its low byte.
  [[nodiscard]] ThemeType theme() const { return data_.theme; }
  void set_theme(const ThemeType theme) { data_.theme = theme; }
  [[nodiscard]] VocType sound() const { return data_.sound; }
  void set_sound(const VocType sound) { data_.sound = sound; }
  [[nodiscard]] VoxType speech() const { return data_.speech; }
  void set_speech(const VoxType speech) { data_.speech = speech; }
  [[nodiscard]] HousesType house() const { return data_.house; }
  void set_house(const HousesType house) { data_.house = house; }
  [[nodiscard]] SpecialWeaponType special() const { return data_.special; }
  void set_special(const SpecialWeaponType special) { data_.special = special; }
  [[nodiscard]] QuarryType quarry() const { return data_.quarry; }
  void set_quarry(const QuarryType quarry) { data_.quarry = quarry; }
  [[nodiscard]] VQType movie() const { return data_.movie; }
  void set_movie(const VQType movie) { data_.movie = movie; }
  [[nodiscard]] bool flag() const { return data_.flag; }
  void set_flag(const bool flag) { data_.flag = flag; }
  // Waypoint, global, text ID or timer tenths of a minute; -1 when unset.
  [[nodiscard]] int value() const { return data_.value; }
  void set_value(const int value) { data_.value = value; }

  // Clears team_type or trigger_type when `target` is the team type or trigger
  // type it points to; called when that type is being destroyed.
  void Detach(Target target);

  // Saved-game support; defined in ioobj.cc. The action travels as its
  // record.
  template <class Archive>
  void Serialize(Archive& ar);

  // Carries out the action for the trigger that sprang. Returns false when
  // the action could not be done (no reinforcements could be created, nothing
  // to destroy), so the trigger is not counted as used.
  bool Perform(const TriggerActionContext& context);

 private:
  TriggerActionType type_{TriggerActionType::kNone};
  TeamTypeClass* team_type_ = nullptr;
  TriggerType* trigger_type_ = nullptr;

  // The action's value; type_ decides which member means something (see
  // ActionNeed()). The one-byte members alias the low byte of `value`, which
  // is what the shipped scenarios rely on (see trigger_action_test.cc).
  union {
    ThemeType theme;            // Musical theme.
    VocType sound;              // Sound effect.
    VoxType speech;             // Speech identifier.
    HousesType house;           // House to be affected.
    SpecialWeaponType special;  // Special weapon ability.
    QuarryType quarry;          // Preferred target for attack.
    VQType movie;               // The movie to play.
    bool flag;                  // Boolean value.
    int value;
  } data_{.value = -1};
};

extern template void TriggerActionRecord::Serialize<ArchiveWriter>(
    ArchiveWriter&);
extern template void TriggerActionRecord::Serialize<ArchiveReader>(
    ArchiveReader&);
extern template void TriggerAction::Serialize<ArchiveWriter>(ArchiveWriter&);
extern template void TriggerAction::Serialize<ArchiveReader>(ArchiveReader&);

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_AI_TRIGGER_ACTION_H_
