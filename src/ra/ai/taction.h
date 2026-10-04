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
// (TActionClass), the kinds of action there are (TActionType), and the entries
// the scenario editor lists them with (ActionChoiceClass).

#ifndef CNC_RED_ALERT_RA_AI_TACTION_H_
#define CNC_RED_ALERT_RA_AI_TACTION_H_

#include <algorithm>
#include <string>
#include <string_view>

#include "absl/strings/ascii.h"
#include "base/enum_array.h"
#include "base/enum_step.h"
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
enum class TActionType {
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

  TACTION_COUNT,
};
using enum TActionType;

// Returns the action the scenario editor lists as `name`, ignoring case, or
// TACTION_NONE when there is none (or `name` is empty).
TActionType Action_From_Name(std::string_view name);

// Returns the scenario editor's name for `action`.
const char* Name_From_Action(TActionType action);

// Returns the kind of value `action` takes in TActionClass::Data, which the
// scenario editor prompts for; NEED_NONE when it takes none. NEED_TEAM and
// NEED_TRIGGER mean the Team or Trigger member instead.
NeedType Action_Needs(TActionType action);

// One action of a trigger type: the action and the data it needs. A trigger
// type holds two, and carries them out by calling them when it springs.
struct TActionClass {
  TActionType Action{TACTION_NONE};  // Action to perform.

  TeamTypeClass* Team = nullptr;  // Team type for this action (if needed).

  // Trigger type for this action (if needed).
  TriggerTypeClass* Trigger = nullptr;

  // The action's value, read through the member Action_Needs() names. The INI
  // file and saved games store Value; the one-byte members read its low byte,
  // which is what the shipped scenarios rely on (see taction_test.cc).
  union {
    ThemeType Theme;            // Musical theme.
    VocType Sound;              // Sound effect.
    VoxType Speech;             // Speech identifier.
    HousesType House;           // House to be affected.
    SpecialWeaponType Special;  // Special weapon ability.
    QuarryType Quarry;          // Preferred target for attack.
    VQType Movie;               // The movie to play.
    bool Bool;                  // Boolean value.
    int Value;  // Waypoint, global, text ID or timer tenths of a
                // minute; -1 when unset.
  } Data{};

  // Trigger name read from a pre-version-2 INI, where triggers may name ones
  // that load later. TriggerTypeClass::Read_INI resolves it into Trigger once
  // every trigger exists; empty otherwise. Not saved.
  std::string PendingTriggerName;

  TActionClass() {
    Data.Theme = THEME_NONE;
    Data.Value = -1;
  }

  // Clears Team or Trigger when `target` is the team type or trigger type it
  // points to; called when that type is being destroyed.
  void Detach(Target target);

  // Saved-game support; defined in ioobj.cc. The union travels through its
  // widest member.
  template <class Archive>
  void Serialize(Archive& ar);

  // Reads the action fields from the trigger entry `tokens` is parsing: the
  // action number, the team, the trigger and the value. The counterpart of
  // Build_INI_Entry().
  void Read_INI(base::Tokenizer& tokens);

  // Returns the action in the scenario INI's current format:
  // "action,team,trigger,value", the middle two as positions in their type
  // lists (-1 for none).
  [[nodiscard]] std::string Build_INI_Entry() const;

  // Carries out the action for the trigger that sprang. `house` is the
  // trigger's owner, which the house actions without a Data.House act on.
  // `object` is what the trigger was attached to and sprang on, or nullptr.
  // `id` is the springing trigger's slot in the trigger heap, or -1 for none.
  // `cell` is the cell the trigger is attached to; cell 0 means none.
  // Returns false when the action could not be done (no reinforcements could
  // be created, nothing to destroy), so the trigger is not counted as used.
  bool operator()(HousesType house, ObjectClass* object, int id, Cell cell);
};

// An action as an entry of the scenario editor's action drop lists, which
// sort by description and draw each entry through Draw_It().
class ActionChoiceClass {
 public:
  // Implicit: choice tables are brace lists of action types.
  // NOLINTNEXTLINE(*-explicit-constructor)
  ActionChoiceClass(TActionType event = TACTION_NONE) noexcept
      : Action(event) {}

  // Implicit: choices compare directly against their action type.
  // NOLINTNEXTLINE(*-explicit-constructor)
  operator TActionType() const { return Action; }
  bool operator==(const ActionChoiceClass& rvalue) const {
    return Action == rvalue.Action;
  }
  bool operator!=(const ActionChoiceClass& rvalue) const {
    return Action != rvalue.Action;
  }
  bool operator>(const ActionChoiceClass& rvalue) const {
    return rvalue < *this;
  }
  // Orders by description, ignoring case.
  bool operator<(const ActionChoiceClass& rvalue) const {
    return std::ranges::lexicographical_compare(
        std::string_view(Description()), std::string_view(rvalue.Description()),
        {}, absl::ascii_tolower, absl::ascii_tolower);
  }
  bool operator<=(const ActionChoiceClass& rvalue) const {
    return Action == rvalue.Action || *this < rvalue;
  }
  bool operator>=(const ActionChoiceClass& rvalue) const {
    return Action == rvalue.Action || rvalue < *this;
  }
  // The editor's name for the action.
  [[nodiscard]] const char* Description() const {
    return Name_From_Action(Action);
  }

  // Draws the description as a list box entry in the box at (`x`, `y`) of
  // `width` by `height` pixels, highlighted when `selected`. The index of the
  // entry in the list is not used.
  void Draw_It(PixelView& view, int index, int x, int y, int width, int height,
               bool selected, FontType font, TextPrintType flags) const;

  TActionType Action;
};

// The choice for every action, indexed by action.
extern base::EnumArray<TActionType, ActionChoiceClass,
                       static_cast<int>(TACTION_COUNT)>
    ActionChoices;

extern template void TActionClass::Serialize<ArchiveWriter>(ArchiveWriter&);
extern template void TActionClass::Serialize<ArchiveReader>(ArchiveReader&);

}  // namespace ra

template <>
inline constexpr bool base::kIsSteppableEnum<ra::TActionType> = true;

#endif  // CNC_RED_ALERT_RA_AI_TACTION_H_
