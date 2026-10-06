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

// The scenario editor's dialog for a trigger type: TriggerType::Edit() and the
// gadget layout and list entries it needs.

#include <algorithm>
#include <array>
#include <string_view>
#include <utility>

#include "absl/base/attributes.h"
#include "absl/strings/ascii.h"
#include "absl/strings/str_format.h"
#include "base/enum_array.h"
#include "base/numeric.h"
#include "base/strings/number_parse.h"
#include "base/strings/safe_string.h"
#include "engine/file/mix_archive.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/wwstd.h"
#include "engine/window/hardware_cursor.h"
#include "engine/window/keyboard.h"
#include "game/core/object_heap.h"
#include "game/ui/control.h"
#include "game/ui/gadget.h"
#include "game/ui/input.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/ai/teamtype.h"
#include "ra/ai/tevent.h"
#include "ra/ai/trigger_action.h"
#include "ra/ai/trigger_type.h"
#include "ra/ai/waypoint_name.h"
#include "ra/assets/audio.h"
#include "ra/assets/palettes.h"
#include "ra/assets/screen.h"
#include "ra/assets/theme.h"
#include "ra/core/const.h"
#include "ra/core/defines.h"
#include "ra/core/object_heaps.h"
#include "ra/data/text.h"
#include "ra/data/text_ids.h"
#include "ra/debug/debug.h"
#include "ra/objects/type.h"
#include "ra/startup/conquer.h"
#include "ra/ui/color_scheme.h"
#include "ra/ui/dialog.h"
#include "ra/ui/drop.h"
#include "ra/ui/edit.h"
#include "ra/ui/textbtn.h"

namespace ra {

using enum engine::window::KeyNumber;

namespace {

// An action as an entry of the scenario editor's action drop lists, which
// sort by description and draw each entry through Draw_It().
class ActionChoice {
 public:
  // Implicit: choice tables are brace lists of action types.
  // NOLINTNEXTLINE(*-explicit-constructor)
  ActionChoice(TriggerActionType action = TriggerActionType::kNone) noexcept
      : action_(action) {}

  // Implicit: choices compare directly against their action type.
  // NOLINTNEXTLINE(*-explicit-constructor)
  operator TriggerActionType() const { return action_; }
  // Orders by description, ignoring case; the editor sorts its lists with it.
  bool operator<(const ActionChoice& other) const {
    return std::ranges::lexicographical_compare(
        std::string_view(Description()), std::string_view(other.Description()),
        {}, absl::ascii_tolower, absl::ascii_tolower);
  }

  // The editor's name for the action.
  [[nodiscard]] const char* Description() const { return ActionName(action_); }

  // Draws the description as a list box entry in the box at (`x`, `y`) of
  // `width` by `height` pixels, highlighted when `selected`. The index of the
  // entry in the list is not used.
  void Draw_It(PixelView& view, int index, int x, int y, int width, int height,
               bool selected, FontType font, TextPrintType flags) const;

 private:
  TriggerActionType action_;
};

void ActionChoice::Draw_It(PixelView& view, int /*unused*/, int x, int y,
                           int width, int height, bool selected, FontType font,
                           TextPrintType flags) const {
  RemapControlType* scheme = Get_Color_Scheme();
  // Tab stops in pixels; the action names hold no tabs, so they only matter to
  // the shared print call.
  static constexpr std::array<int, 2> kTabStops = {13, 40};
  if (font == FontType::k6PointGradient || font == FontType::kEditor) {
    if (selected) {
      flags = flags | TPF_BRIGHT_COLOR;
      view.FillRect(x, y, x + width - 1, y + height - 1, scheme->Shadow);
    } else if (!base::Any(flags & TPF_USE_GRAD_PAL)) {
      flags = flags | TPF_MEDIUM_COLOR;
    }
  } else {
    scheme = &ThePalettes().color_remaps().at(selected ? PCOLOR_DIALOG_BLUE
                                                       : PCOLOR_GREY);
  }
  Conquer_Clip_Text_Print(view, Description(), x, y, scheme, kTBlack, font,
                          flags, width, kTabStops);
}

// Returns the name selected in `list`, or an empty view when the list has no
// items: Current_Item() is null then, which a string_view cannot hold.
std::string_view SelectedName(
    DropListClass& list ABSL_ATTRIBUTE_LIFETIME_BOUND) {
  const char* const name = list.Current_Item();
  return name != nullptr ? name : std::string_view();
}

}  // namespace

namespace {

// Layout and gadget IDs of the trigger editor dialog, in pixels. Each event
// and each action has one data field position;
// the gadget that suits what the chosen event or action needs is put there.

// Dialog position and dimensions.
constexpr int kDialogW = 320 + 100;
constexpr int kDialogH = 200 + 20;
constexpr int kDialogX = 0;
constexpr int kDialogY = 0;

// Event drop list coordinates and dimensions; the second sits below the
// first.
constexpr int kEvent1X = kDialogX + 45;
constexpr int kEvent1Y = kDialogY + 65;
constexpr int kEvent2X = kEvent1X;
constexpr int kEvent2Y = kEvent1Y + 22;
constexpr int kListWidth = 160;
constexpr int kListHeight = 8 * 5;

// Event data field coordinates and dimensions, to the right of each list.
constexpr int kEvent1DataX = kEvent1X + kListWidth + 20;
constexpr int kEvent1DataY = kEvent1Y;
constexpr int kEvent2DataX = kEvent1DataX;
constexpr int kEvent2DataY = kEvent2Y;

constexpr int kDataWidth = 95;
constexpr int kDataHeight = 8 * 5;

// Action drop list coordinates; the lists share the event lists' size.
constexpr int kAction1X = kEvent1X;
constexpr int kAction1Y = kDialogY + 120;
constexpr int kAction2X = kEvent1X;
constexpr int kAction2Y = kAction1Y + 22;

// Action data field coordinates.
constexpr int kAction1DataX = kAction1X + kListWidth + 20;
constexpr int kAction1DataY = kAction1Y;
constexpr int kAction2DataX = kAction1DataX;
constexpr int kAction2DataY = kAction2Y;

// Text buffer sizes, each counting the terminating NUL.
constexpr int kNumberSize = 10;  // Text length for general data entry fields.
constexpr int kChoiceSize =
    35;  // Maximum size of event or action description text.
constexpr int kWaypointSize = 3;  // Text length maximum for waypoint entry.
constexpr int kNameSize = 10;     // Team name text entry field length.
constexpr int kDescriptionSize =
    35;  // Maximum length of object full name description.

// Gadget IDs. "Primary" is the first event or action, "secondary" the second.
constexpr int kEvent1List = 100;
constexpr int kEvent2List = 101;
constexpr int kAction1List = 102;
constexpr int kAction2List = 103;
constexpr int kNameEdit = 104;
constexpr int kAction1Speech = 105;
constexpr int kAction2Speech = 106;
constexpr int kAction1Theme = 107;
constexpr int kAction2Theme = 108;
constexpr int kAction1Movie = 109;
constexpr int kAction2Movie = 110;
constexpr int kAction1Sound = 111;
constexpr int kAction2Sound = 112;
constexpr int kAction1Special = 113;
constexpr int kAction2Special = 114;
constexpr int kAction1Waypoint = 117;
constexpr int kAction2Waypoint = 118;
constexpr int kEvent1House = 119;
constexpr int kEvent2House = 120;
constexpr int kAction1House = 121;
constexpr int kAction2House = 122;
constexpr int kAction1Bool = 123;
constexpr int kAction2Bool = 124;
constexpr int kEvent1Number = 125;
constexpr int kEvent2Number = 126;
constexpr int kAction1Number = 127;
constexpr int kAction2Number = 128;
constexpr int kEvent1Building = 129;
constexpr int kEvent2Building = 130;
constexpr int kEvent1Infantry = 131;
constexpr int kEvent2Infantry = 132;
constexpr int kEvent1Aircraft = 133;
constexpr int kEvent2Aircraft = 134;
constexpr int kEvent1Unit = 135;
constexpr int kEvent2Unit = 136;
constexpr int kEvent1Team = 137;
constexpr int kEvent2Team = 138;
constexpr int kAction1Team = 139;
constexpr int kAction2Team = 140;
constexpr int kAction1Trigger = 141;
constexpr int kAction2Trigger = 142;
constexpr int kOwnerList = 143;        // house ownership for this trigger.
constexpr int kPersistenceList = 144;  // Persistence of this trigger.
constexpr int kButtonOk = 145;           // Ok button - save and exit.
constexpr int kButtonCancel = 146;       // Cancel button - just exit.
constexpr int kActionControlButton = 147;  // Multiple action control button.
constexpr int kEventControlButton = 148;   // Multiple event control button.

}  // namespace

bool TriggerType::Edit() {
  RemapControlType* scheme = Get_Color_Scheme();

  // Head of the dialog's gadget list. Only the gadgets linked into it are
  // drawn and receive input; the redraw pass below relinks the data fields to
  // match the chosen events and actions.
  game::ControlClass* gadgets = nullptr;

  // The two event lists, each offering every event, sorted by name.
  char event1_text[kChoiceSize] = "";
  TDropListClass<EventChoiceClass*> event1_list(
      kEvent1List, event1_text, sizeof(event1_text), FontType::kEditor,
      TPF_NOSHADOW, kEvent1X, kEvent1Y, kListWidth, kListHeight,
      MixArchive::RetrieveData("EBTN-UP.SHP"),
      MixArchive::RetrieveData("EBTN-DN.SHP"));
  char event2_text[kChoiceSize] = "";
  TDropListClass<EventChoiceClass*> event2_list(
      kEvent2List, event2_text, sizeof(event2_text), FontType::kEditor,
      TPF_NOSHADOW, kEvent2X, kEvent2Y, kListWidth, kListHeight,
      MixArchive::RetrieveData("EBTN-UP.SHP"),
      MixArchive::RetrieveData("EBTN-DN.SHP"));
  for (TEventType event = TEVENT_NONE; event < TEVENT_COUNT; event++) {
    event1_list.Add_Item(&EventChoices.at(event));
    event2_list.Add_Item(&EventChoices.at(event));
  }

  const auto by_description = [](const auto* left, const auto* right) {
    return *left < *right;
  };
  event1_list.Sort(by_description);
  event2_list.Sort(by_description);

  event1_list.Set_Selected_Index(&EventChoices.at(event1.Event));
  event2_list.Set_Selected_Index(&EventChoices.at(event2.Event));

  // The dialog shows and edits the actions as records; OK makes the actions
  // from them.
  TriggerActionRecord action1_record = action1->ToRecord();
  TriggerActionRecord action2_record = action2->ToRecord();

  // The two action lists, likewise. The lists hold pointers to the choices,
  // so the choices are declared first and outlive them.
  base::EnumArray<TriggerActionType, ActionChoice> action_choices{};
  char action1_text[kChoiceSize] = "";
  TDropListClass<ActionChoice*> action1_list(
      kAction1List, action1_text, sizeof(action1_text), FontType::kEditor,
      TPF_NOSHADOW, kAction1X, kAction1Y, kListWidth, kListHeight,
      MixArchive::RetrieveData("EBTN-UP.SHP"),
      MixArchive::RetrieveData("EBTN-DN.SHP"));
  char action2_text[kChoiceSize] = "";
  TDropListClass<ActionChoice*> action2_list(
      kAction2List, action2_text, sizeof(action2_text), FontType::kEditor,
      TPF_NOSHADOW, kAction2X, kAction2Y, kListWidth, kListHeight,
      MixArchive::RetrieveData("EBTN-UP.SHP"),
      MixArchive::RetrieveData("EBTN-DN.SHP"));
  for (const TriggerActionType action :
       magic_enum::enum_values<TriggerActionType>()) {
    action_choices.at(action) = action;
    action1_list.Add_Item(&action_choices.at(action));
    action2_list.Add_Item(&action_choices.at(action));
  }

  action1_list.Sort(by_description);
  action2_list.Sort(by_description);

  action1_list.Set_Selected_Index(&action_choices.at(action1_record.type));
  action2_list.Set_Selected_Index(&action_choices.at(action2_record.type));

  // Waypoint fields, one per action; no event takes a waypoint. A waypoint
  // number shows as a
  // letter, 'A' for 0 to 'Z' for 25, and from 26 on as two letters, "AA" for
  // 26 to "ZZ" for 701.
  char action1_waypoint_text[kWaypointSize] = "A";
  EditClass action1_waypoint_edit(
      kAction1Waypoint, action1_waypoint_text, sizeof(action1_waypoint_text),
      FontType::kEditor, TPF_NOSHADOW, kAction1DataX, kAction1DataY, kDataWidth,
      9, EditClass::kAlpha);
  if (ActionNeed(action1_record.type) == NEED_WAYPOINT) {
    base::SafeCopy(action1_waypoint_text,
                   WaypointLetters(action1_record.value));
  }

  char action2_waypoint_text[kWaypointSize] = "A";
  EditClass action2_waypoint_edit(
      kAction2Waypoint, action2_waypoint_text, sizeof(action2_waypoint_text),
      FontType::kEditor, TPF_NOSHADOW, kAction2DataX, kAction2DataY, kDataWidth,
      9, EditClass::kAlpha);
  if (ActionNeed(action2_record.type) == NEED_WAYPOINT) {
    base::SafeCopy(action2_waypoint_text,
                   WaypointLetters(action2_record.value));
  }

  // Number fields, one per event and action.
  char event1_number_text[kNumberSize] = "";
  EditClass event1_number_edit(kEvent1Number, event1_number_text,
                               sizeof(event1_number_text), FontType::kEditor,
                               TPF_NOSHADOW, kEvent1DataX, kEvent1DataY,
                               kDataWidth, 9, EditClass::kNumeric);
  if (const NeedType need = Event_Needs(event1.Event);
      need == NEED_TIME || need == NEED_NUMBER) {
    absl::SNPrintF(event1_number_text, sizeof(event1_number_text), "%d",
                   static_cast<int>(event1.Data.Value));
  }

  char event2_number_text[kNumberSize] = "";
  EditClass event2_number_edit(kEvent2Number, event2_number_text,
                               sizeof(event2_number_text), FontType::kEditor,
                               TPF_NOSHADOW, kEvent2DataX, kEvent2DataY,
                               kDataWidth, 9, EditClass::kNumeric);
  if (const NeedType need = Event_Needs(event2.Event);
      need == NEED_TIME || need == NEED_NUMBER) {
    absl::SNPrintF(event2_number_text, sizeof(event2_number_text), "%d",
                   static_cast<int>(event2.Data.Value));
  }

  char action1_number_text[kNumberSize] = "";
  EditClass action1_number_edit(kAction1Number, action1_number_text,
                                sizeof(action1_number_text), FontType::kEditor,
                                TPF_NOSHADOW, kAction1DataX, kAction1DataY,
                                kDataWidth, 9, EditClass::kNumeric);
  if (ActionNeed(action1_record.type) == NEED_NUMBER) {
    absl::SNPrintF(action1_number_text, sizeof(action1_number_text), "%d",
                   action1_record.value);
  }

  char action2_number_text[kNumberSize] = "";
  EditClass action2_number_edit(kAction2Number, action2_number_text,
                                sizeof(action2_number_text), FontType::kEditor,
                                TPF_NOSHADOW, kAction2DataX, kAction2DataY,
                                kDataWidth, 9, EditClass::kNumeric);
  if (ActionNeed(action2_record.type) == NEED_NUMBER) {
    absl::SNPrintF(action2_number_text, sizeof(action2_number_text), "%d",
                   action2_record.value);
  }

  // Team type lists, one per event and action, by INI name.
  char event1_team_text[kNameSize] = "";
  DropListClass event1_team_list(
      kEvent1Team, event1_team_text, sizeof(event1_team_text),
      FontType::kEditor, TPF_NOSHADOW, kEvent1DataX, kEvent1DataY, kDataWidth,
      kDataHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
      MixArchive::RetrieveData("EBTN-DN.SHP"));
  char event2_team_text[kNameSize] = "";
  DropListClass event2_team_list(
      kEvent2Team, event2_team_text, sizeof(event2_team_text),
      FontType::kEditor, TPF_NOSHADOW, kEvent2DataX, kEvent2DataY, kDataWidth,
      kDataHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
      MixArchive::RetrieveData("EBTN-DN.SHP"));
  char action1_team_text[kNameSize] = "";
  DropListClass action1_team_list(
      kAction1Team, action1_team_text, sizeof(action1_team_text),
      FontType::kEditor, TPF_NOSHADOW, kAction1DataX, kAction1DataY, kDataWidth,
      kDataHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
      MixArchive::RetrieveData("EBTN-DN.SHP"));
  char action2_team_text[kNameSize] = "";
  DropListClass action2_team_list(
      kAction2Team, action2_team_text, sizeof(action2_team_text),
      FontType::kEditor, TPF_NOSHADOW, kAction2DataX, kAction2DataY, kDataWidth,
      kDataHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
      MixArchive::RetrieveData("EBTN-DN.SHP"));

  for (int index = 0; index < TheObjectHeaps().team_type().Count(); index++) {
    event1_team_list.Add_Item(
        TheObjectHeaps().team_type().ActiveAt(index)->IniName);
    event2_team_list.Add_Item(
        TheObjectHeaps().team_type().ActiveAt(index)->IniName);
    action1_team_list.Add_Item(
        TheObjectHeaps().team_type().ActiveAt(index)->IniName);
    action2_team_list.Add_Item(
        TheObjectHeaps().team_type().ActiveAt(index)->IniName);
  }

  if (event1.Team != nullptr) {
    event1_team_list.Set_Selected_Index(event1.Team->IniName);
  } else {
    event1_team_list.Set_Selected_Index(0);
  }
  if (event2.Team != nullptr) {
    event2_team_list.Set_Selected_Index(event2.Team->IniName);
  } else {
    event2_team_list.Set_Selected_Index(0);
  }
  if (action1_record.team_type != nullptr) {
    action1_team_list.Set_Selected_Index(action1_record.team_type->IniName);
  } else {
    action1_team_list.Set_Selected_Index(0);
  }
  if (action2_record.team_type != nullptr) {
    action2_team_list.Set_Selected_Index(action2_record.team_type->IniName);
  } else {
    action2_team_list.Set_Selected_Index(0);
  }

  // Trigger type lists for the actions, by INI name. They include this
  // trigger type itself.
  char action1_trigger_text[kNameSize] = "";
  DropListClass action1_trigger_list(
      kAction1Trigger, action1_trigger_text, sizeof(action1_trigger_text),
      FontType::kEditor, TPF_NOSHADOW, kAction1DataX, kAction1DataY, kDataWidth,
      kDataHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
      MixArchive::RetrieveData("EBTN-DN.SHP"));
  char action2_trigger_text[kNameSize] = "";
  DropListClass action2_trigger_list(
      kAction2Trigger, action2_trigger_text, sizeof(action2_trigger_text),
      FontType::kEditor, TPF_NOSHADOW, kAction2DataX, kAction2DataY, kDataWidth,
      kDataHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
      MixArchive::RetrieveData("EBTN-DN.SHP"));

  for (int index = 0; index < TheObjectHeaps().trigger_type().Count();
       index++) {
    action1_trigger_list.Add_Item(
        TheObjectHeaps().trigger_type().ActiveAt(index)->IniName);
    action2_trigger_list.Add_Item(
        TheObjectHeaps().trigger_type().ActiveAt(index)->IniName);
  }

  if (action1_record.trigger_type != nullptr) {
    action1_trigger_list.Set_Selected_Index(
        action1_record.trigger_type->IniName);
  } else {
    action1_trigger_list.Set_Selected_Index(0);
  }
  if (action2_record.trigger_type != nullptr) {
    action2_trigger_list.Set_Selected_Index(
        action2_record.trigger_type->IniName);
  } else {
    action2_trigger_list.Set_Selected_Index(0);
  }

  // On/off lists for the actions.
  char action1_bool_text[kNameSize] = "";
  DropListClass action1_bool_list(
      kAction1Bool, action1_bool_text, sizeof(action1_bool_text),
      FontType::kEditor, TPF_NOSHADOW, kAction1DataX, kAction1DataY, kDataWidth,
      kDataHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
      MixArchive::RetrieveData("EBTN-DN.SHP"));
  char action2_bool_text[kNameSize] = "";
  DropListClass action2_bool_list(
      kAction2Bool, action2_bool_text, sizeof(action2_bool_text),
      FontType::kEditor, TPF_NOSHADOW, kAction2DataX, kAction2DataY, kDataWidth,
      kDataHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
      MixArchive::RetrieveData("EBTN-DN.SHP"));

  action1_bool_list.Add_Item("OFF");
  action1_bool_list.Add_Item("ON");
  action2_bool_list.Add_Item("OFF");
  action2_bool_list.Add_Item("ON");

  action1_bool_list.Set_Selected_Index(
      ActionNeed(action1_record.type) == NEED_BOOL && action1_record.value != 0
          ? 1
          : 0);
  action2_bool_list.Set_Selected_Index(
      ActionNeed(action2_record.type) == NEED_BOOL && action2_record.value != 0
          ? 1
          : 0);

  // Musical theme lists for the actions; the list index is the ThemeType.
  char action1_theme_text[kDescriptionSize] = "";
  DropListClass action1_theme_list(
      kAction1Theme, action1_theme_text, sizeof(action1_theme_text),
      FontType::kEditor, TPF_NOSHADOW, kAction1DataX, kAction1DataY, kDataWidth,
      kDataHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
      MixArchive::RetrieveData("EBTN-DN.SHP"));
  char action2_theme_text[kDescriptionSize] = "";
  DropListClass action2_theme_list(
      kAction2Theme, action2_theme_text, sizeof(action2_theme_text),
      FontType::kEditor, TPF_NOSHADOW, kAction2DataX, kAction2DataY, kDataWidth,
      kDataHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
      MixArchive::RetrieveData("EBTN-DN.SHP"));

  for (const ThemeType theme : magic_enum::enum_values<ThemeType>()) {
    action1_theme_list.Add_Item(ThemeClass::Full_Name(theme));
    action2_theme_list.Add_Item(ThemeClass::Full_Name(theme));
  }

  if (ActionNeed(action1_record.type) == NEED_THEME) {
    action1_theme_list.Set_Selected_Index(action1_record.value);
  } else {
    action1_theme_list.Set_Selected_Index(0);
  }
  if (ActionNeed(action2_record.type) == NEED_THEME) {
    action2_theme_list.Set_Selected_Index(action2_record.value);
  } else {
    action2_theme_list.Set_Selected_Index(0);
  }

  // Movie lists for the actions; the list index is the VQType.
  char action1_movie_text[kDescriptionSize] = "";
  DropListClass action1_movie_list(
      kAction1Movie, action1_movie_text, sizeof(action1_movie_text),
      FontType::kEditor, TPF_NOSHADOW, kAction1DataX, kAction1DataY, kDataWidth,
      kDataHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
      MixArchive::RetrieveData("EBTN-DN.SHP"));
  char action2_movie_text[kDescriptionSize] = "";
  DropListClass action2_movie_list(
      kAction2Movie, action2_movie_text, sizeof(action2_movie_text),
      FontType::kEditor, TPF_NOSHADOW, kAction2DataX, kAction2DataY, kDataWidth,
      kDataHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
      MixArchive::RetrieveData("EBTN-DN.SHP"));

  for (const VQType movie : magic_enum::enum_values<VQType>()) {
    action1_movie_list.Add_Item(VQName.at(movie));
    action2_movie_list.Add_Item(VQName.at(movie));
  }

  if (ActionNeed(action1_record.type) == NEED_MOVIE) {
    action1_movie_list.Set_Selected_Index(action1_record.value);
  } else {
    action1_movie_list.Set_Selected_Index(0);
  }
  if (ActionNeed(action2_record.type) == NEED_MOVIE) {
    action2_movie_list.Set_Selected_Index(action2_record.value);
  } else {
    action2_movie_list.Set_Selected_Index(0);
  }

  // Sound effect lists for the actions; the list index is the VocType.
  char action1_sound_text[kDescriptionSize] = "";
  DropListClass action1_sound_list(
      kAction1Sound, action1_sound_text, sizeof(action1_sound_text),
      FontType::kEditor, TPF_NOSHADOW, kAction1DataX, kAction1DataY, kDataWidth,
      kDataHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
      MixArchive::RetrieveData("EBTN-DN.SHP"));
  char action2_sound_text[kDescriptionSize] = "";
  DropListClass action2_sound_list(
      kAction2Sound, action2_sound_text, sizeof(action2_sound_text),
      FontType::kEditor, TPF_NOSHADOW, kAction2DataX, kAction2DataY, kDataWidth,
      kDataHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
      MixArchive::RetrieveData("EBTN-DN.SHP"));

  for (const VocType sound : magic_enum::enum_values<VocType>()) {
    action1_sound_list.Add_Item(VocName(sound));
    action2_sound_list.Add_Item(VocName(sound));
  }

  if (ActionNeed(action1_record.type) == NEED_SOUND) {
    action1_sound_list.Set_Selected_Index(action1_record.value);
  } else {
    action1_sound_list.Set_Selected_Index(0);
  }
  if (ActionNeed(action2_record.type) == NEED_SOUND) {
    action2_sound_list.Set_Selected_Index(action2_record.value);
  } else {
    action2_sound_list.Set_Selected_Index(0);
  }

  // Speech lists for the actions; the list index is the VoxType.
  char action1_speech_text[kDescriptionSize] = "";
  DropListClass action1_speech_list(
      kAction1Speech, action1_speech_text, sizeof(action1_speech_text),
      FontType::kEditor, TPF_NOSHADOW, kAction1DataX, kAction1DataY, kDataWidth,
      kDataHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
      MixArchive::RetrieveData("EBTN-DN.SHP"));
  char action2_speech_text[kDescriptionSize] = "";
  DropListClass action2_speech_list(
      kAction2Speech, action2_speech_text, sizeof(action2_speech_text),
      FontType::kEditor, TPF_NOSHADOW, kAction2DataX, kAction2DataY, kDataWidth,
      kDataHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
      MixArchive::RetrieveData("EBTN-DN.SHP"));

  for (const VoxType speech : magic_enum::enum_values<VoxType>()) {
    action1_speech_list.Add_Item(VoxName(speech));
    action2_speech_list.Add_Item(VoxName(speech));
  }

  if (ActionNeed(action1_record.type) == NEED_SPEECH) {
    action1_speech_list.Set_Selected_Index(action1_record.value);
  } else {
    action1_speech_list.Set_Selected_Index(0);
  }
  if (ActionNeed(action2_record.type) == NEED_SPEECH) {
    action2_speech_list.Set_Selected_Index(action2_record.value);
  } else {
    action2_speech_list.Set_Selected_Index(0);
  }

  // Building type lists for the events; the list index is the StructType.
  char event1_building_text[kDescriptionSize] = "";
  DropListClass event1_building_list(
      kEvent1Building, event1_building_text, sizeof(event1_building_text),
      FontType::kEditor, TPF_NOSHADOW, kEvent1DataX, kEvent1DataY, kDataWidth,
      kDataHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
      MixArchive::RetrieveData("EBTN-DN.SHP"));
  char event2_building_text[kDescriptionSize] = "";
  DropListClass event2_building_list(
      kEvent2Building, event2_building_text, sizeof(event2_building_text),
      FontType::kEditor, TPF_NOSHADOW, kEvent2DataX, kEvent2DataY, kDataWidth,
      kDataHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
      MixArchive::RetrieveData("EBTN-DN.SHP"));

  for (const StructType building : magic_enum::enum_values<StructType>()) {
    event1_building_list.Add_Item(
        TextString(BuildingTypeClass::As_Reference(building).Full_Name()));
    event2_building_list.Add_Item(
        TextString(BuildingTypeClass::As_Reference(building).Full_Name()));
  }

  if (Event_Needs(event1.Event) == NEED_STRUCTURE) {
    event1_building_list.Set_Selected_Index(
        static_cast<int>(event1.Data.Structure));
  } else {
    event1_building_list.Set_Selected_Index(0);
  }
  if (Event_Needs(event2.Event) == NEED_STRUCTURE) {
    event2_building_list.Set_Selected_Index(
        static_cast<int>(event2.Data.Structure));
  } else {
    event2_building_list.Set_Selected_Index(0);
  }

  // Infantry type lists for the events; the list index is the InfantryType.
  char event1_infantry_text[kDescriptionSize] = "";
  DropListClass event1_infantry_list(
      kEvent1Infantry, event1_infantry_text, sizeof(event1_infantry_text),
      FontType::kEditor, TPF_NOSHADOW, kEvent1DataX, kEvent1DataY, kDataWidth,
      kDataHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
      MixArchive::RetrieveData("EBTN-DN.SHP"));
  char event2_infantry_text[kDescriptionSize] = "";
  DropListClass event2_infantry_list(
      kEvent2Infantry, event2_infantry_text, sizeof(event2_infantry_text),
      FontType::kEditor, TPF_NOSHADOW, kEvent2DataX, kEvent2DataY, kDataWidth,
      kDataHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
      MixArchive::RetrieveData("EBTN-DN.SHP"));

  for (const InfantryType infantry : magic_enum::enum_values<InfantryType>()) {
    event1_infantry_list.Add_Item(
        TextString(InfantryTypeClass::As_Reference(infantry).Full_Name()));
    event2_infantry_list.Add_Item(
        TextString(InfantryTypeClass::As_Reference(infantry).Full_Name()));
  }

  if (Event_Needs(event1.Event) == NEED_INFANTRY) {
    event1_infantry_list.Set_Selected_Index(
        static_cast<int>(event1.Data.Infantry));
  } else {
    event1_infantry_list.Set_Selected_Index(0);
  }
  if (Event_Needs(event2.Event) == NEED_INFANTRY) {
    event2_infantry_list.Set_Selected_Index(
        static_cast<int>(event2.Data.Infantry));
  } else {
    event2_infantry_list.Set_Selected_Index(0);
  }

  // Aircraft type lists for the events; the list index is the AircraftType.
  char event1_aircraft_text[kDescriptionSize] = "";
  DropListClass event1_aircraft_list(
      kEvent1Aircraft, event1_aircraft_text, sizeof(event1_aircraft_text),
      FontType::kEditor, TPF_NOSHADOW, kEvent1DataX, kEvent1DataY, kDataWidth,
      kDataHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
      MixArchive::RetrieveData("EBTN-DN.SHP"));
  char event2_aircraft_text[kDescriptionSize] = "";
  DropListClass event2_aircraft_list(
      kEvent2Aircraft, event2_aircraft_text, sizeof(event2_aircraft_text),
      FontType::kEditor, TPF_NOSHADOW, kEvent2DataX, kEvent2DataY, kDataWidth,
      kDataHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
      MixArchive::RetrieveData("EBTN-DN.SHP"));

  for (const AircraftType aircraft : magic_enum::enum_values<AircraftType>()) {
    event1_aircraft_list.Add_Item(
        TextString(AircraftTypeClass::As_Reference(aircraft).Full_Name()));
    event2_aircraft_list.Add_Item(
        TextString(AircraftTypeClass::As_Reference(aircraft).Full_Name()));
  }

  if (Event_Needs(event1.Event) == NEED_AIRCRAFT) {
    event1_aircraft_list.Set_Selected_Index(
        static_cast<int>(event1.Data.Aircraft));
  } else {
    event1_aircraft_list.Set_Selected_Index(0);
  }
  if (Event_Needs(event2.Event) == NEED_AIRCRAFT) {
    event2_aircraft_list.Set_Selected_Index(
        static_cast<int>(event2.Data.Aircraft));
  } else {
    event2_aircraft_list.Set_Selected_Index(0);
  }

  // Unit type lists for the events; the list index is the UnitType.
  char event1_unit_text[kDescriptionSize] = "";
  DropListClass event1_unit_list(
      kEvent1Unit, event1_unit_text, sizeof(event1_unit_text),
      FontType::kEditor, TPF_NOSHADOW, kEvent1DataX, kEvent1DataY, kDataWidth,
      kDataHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
      MixArchive::RetrieveData("EBTN-DN.SHP"));
  char event2_unit_text[kDescriptionSize] = "";
  DropListClass event2_unit_list(
      kEvent2Unit, event2_unit_text, sizeof(event2_unit_text),
      FontType::kEditor, TPF_NOSHADOW, kEvent2DataX, kEvent2DataY, kDataWidth,
      kDataHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
      MixArchive::RetrieveData("EBTN-DN.SHP"));

  for (const UnitType unit : magic_enum::enum_values<UnitType>()) {
    event1_unit_list.Add_Item(
        TextString(UnitTypeClass::As_Reference(unit).Full_Name()));
    event2_unit_list.Add_Item(
        TextString(UnitTypeClass::As_Reference(unit).Full_Name()));
  }

  if (Event_Needs(event1.Event) == NEED_UNIT) {
    event1_unit_list.Set_Selected_Index(static_cast<int>(event1.Data.Unit));
  } else {
    event1_unit_list.Set_Selected_Index(0);
  }
  if (Event_Needs(event2.Event) == NEED_UNIT) {
    event2_unit_list.Set_Selected_Index(static_cast<int>(event2.Data.Unit));
  } else {
    event2_unit_list.Set_Selected_Index(0);
  }

  // House lists, one per event and action; the list index is the HousesType.
  char event1_house_text[kDescriptionSize] = "";
  DropListClass event1_house_list(
      kEvent1House, event1_house_text, sizeof(event1_house_text),
      FontType::kEditor, TPF_NOSHADOW, kEvent1DataX, kEvent1DataY, kDataWidth,
      kDataHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
      MixArchive::RetrieveData("EBTN-DN.SHP"));
  char event2_house_text[kDescriptionSize] = "";
  DropListClass event2_house_list(
      kEvent2House, event2_house_text, sizeof(event2_house_text),
      FontType::kEditor, TPF_NOSHADOW, kEvent2DataX, kEvent2DataY, kDataWidth,
      kDataHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
      MixArchive::RetrieveData("EBTN-DN.SHP"));
  char action1_house_text[kDescriptionSize] = "";
  DropListClass action1_house_list(
      kAction1House, action1_house_text, sizeof(action1_house_text),
      FontType::kEditor, TPF_NOSHADOW, kAction1DataX, kAction1DataY, kDataWidth,
      kDataHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
      MixArchive::RetrieveData("EBTN-DN.SHP"));
  char action2_house_text[kDescriptionSize] = "";
  DropListClass action2_house_list(
      kAction2House, action2_house_text, sizeof(action2_house_text),
      FontType::kEditor, TPF_NOSHADOW, kAction2DataX, kAction2DataY, kDataWidth,
      kDataHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
      MixArchive::RetrieveData("EBTN-DN.SHP"));

  for (const HousesType house_type : magic_enum::enum_values<HousesType>()) {
    event1_house_list.Add_Item(
        HouseTypeClass::As_Reference(house_type).IniName);
    event2_house_list.Add_Item(
        HouseTypeClass::As_Reference(house_type).IniName);
    action1_house_list.Add_Item(
        HouseTypeClass::As_Reference(house_type).IniName);
    action2_house_list.Add_Item(
        HouseTypeClass::As_Reference(house_type).IniName);
  }

  if (Event_Needs(event1.Event) == NEED_HOUSE) {
    event1_house_list.Set_Selected_Index(static_cast<int>(event1.Data.House));
  } else {
    event1_house_list.Set_Selected_Index(0);
  }
  if (Event_Needs(event2.Event) == NEED_HOUSE) {
    event2_house_list.Set_Selected_Index(static_cast<int>(event2.Data.House));
  } else {
    event2_house_list.Set_Selected_Index(0);
  }
  if (ActionNeed(action1_record.type) == NEED_HOUSE) {
    action1_house_list.Set_Selected_Index(action1_record.value);
  } else {
    action1_house_list.Set_Selected_Index(0);
  }
  if (ActionNeed(action2_record.type) == NEED_HOUSE) {
    action2_house_list.Set_Selected_Index(action2_record.value);
  } else {
    action2_house_list.Set_Selected_Index(0);
  }

  // Special weapon lists for the actions; the list index is the
  // SpecialWeaponType. An action that needs something else holds another
  // member of the data union, which may not be a valid weapon.
  char action1_special_text[kDescriptionSize] = "";
  DropListClass action1_special_list(
      kAction1Special, action1_special_text, sizeof(action1_special_text),
      FontType::kEditor, TPF_NOSHADOW, kAction1DataX, kAction1DataY, kDataWidth,
      kDataHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
      MixArchive::RetrieveData("EBTN-DN.SHP"));
  char action2_special_text[kDescriptionSize] = "";
  DropListClass action2_special_list(
      kAction2Special, action2_special_text, sizeof(action2_special_text),
      FontType::kEditor, TPF_NOSHADOW, kAction2DataX, kAction2DataY, kDataWidth,
      kDataHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
      MixArchive::RetrieveData("EBTN-DN.SHP"));
  for (const SpecialWeaponType weapon :
       magic_enum::enum_values<SpecialWeaponType>()) {
    action1_special_list.Add_Item(SpecialWeaponName.at(weapon));
    action2_special_list.Add_Item(SpecialWeaponName.at(weapon));
  }
  if (ActionNeed(action1_record.type) == NEED_SPECIAL &&
      magic_enum::enum_contains(
          static_cast<SpecialWeaponType>(action1_record.value))) {
    action1_special_list.Set_Selected_Index(action1_record.value);
  } else {
    action1_special_list.Set_Selected_Index(0);
  }
  if (ActionNeed(action2_record.type) == NEED_SPECIAL &&
      magic_enum::enum_contains(
          static_cast<SpecialWeaponType>(action2_record.value))) {
    action2_special_list.Set_Selected_Index(action2_record.value);
  } else {
    action2_special_list.Set_Selected_Index(0);
  }

  // Quarry (preferred target) lists for the actions; the list index is the
  // QuarryType. They share the special weapon lists' gadget IDs, which is
  // harmless: nothing looks either up by ID, and only one of the two is linked
  // in at a time.
  char action1_quarry_text[kDescriptionSize] = "";
  DropListClass action1_quarry_list(
      kAction1Special, action1_quarry_text, sizeof(action1_quarry_text),
      FontType::kEditor, TPF_NOSHADOW, kAction1DataX, kAction1DataY, kDataWidth,
      kDataHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
      MixArchive::RetrieveData("EBTN-DN.SHP"));
  char action2_quarry_text[kDescriptionSize] = "";
  DropListClass action2_quarry_list(
      kAction2Special, action2_quarry_text, sizeof(action2_quarry_text),
      FontType::kEditor, TPF_NOSHADOW, kAction2DataX, kAction2DataY, kDataWidth,
      kDataHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
      MixArchive::RetrieveData("EBTN-DN.SHP"));
  for (const QuarryType quarry : magic_enum::enum_values<QuarryType>()) {
    action1_quarry_list.Add_Item(QuarryName.at(quarry));
    action2_quarry_list.Add_Item(QuarryName.at(quarry));
  }
  if (magic_enum::enum_contains(
          static_cast<QuarryType>(action1_record.value))) {
    action1_quarry_list.Set_Selected_Index(action1_record.value);
  } else {
    action1_quarry_list.Set_Selected_Index(0);
  }
  if (magic_enum::enum_contains(
          static_cast<QuarryType>(action2_record.value))) {
    action2_quarry_list.Set_Selected_Index(action2_record.value);
  } else {
    action2_quarry_list.Set_Selected_Index(0);
  }

  // The trigger name: four characters, which is all Description() shows.
  char name_text[5] = "";
  EditClass name_edit(kNameEdit, name_text, sizeof(name_text),
                      FontType::kEditor, TPF_NOSHADOW, kDialogX + 40,
                      kDialogY + 30, 40, 9, EditClass::kAlphanumeric);
  base::SafeCopy(name_text, IniName);  // Name

  // The house that owns the trigger. The list cannot show "no house", so an
  // unset house becomes HOUSE_GOOD, even if the dialog is then cancelled.
  char owner_text[kDescriptionSize] = "";
  DropListClass owner_list(kOwnerList, owner_text, sizeof(owner_text),
                           FontType::kEditor, TPF_NOSHADOW,
                           name_edit.X + name_edit.Width + 20, name_edit.Y, 95,
                           8 * 5, MixArchive::RetrieveData("EBTN-UP.SHP"),
                           MixArchive::RetrieveData("EBTN-DN.SHP"));
  for (const HousesType house_type : magic_enum::enum_values<HousesType>()) {
    owner_list.Add_Item(HouseTypeClass::As_Reference(house_type).IniName);
  }
  if (house == HOUSE_NONE) {
    house = HOUSE_GOOD;
  }
  owner_list.Set_Selected_Index(static_cast<int>(house));

  // The persistence list; the list index is the Persistence, so the names
  // must stay in the enum's order.
  char persistence_text[kDescriptionSize] = "";
  static constexpr std::array<const char*, 3> kPersistenceNames = {
      "Volatile", "Semi-persistent", "Persistent"};
  DropListClass persistence_list(
      kPersistenceList, persistence_text, sizeof(persistence_text),
      FontType::kEditor, TPF_NOSHADOW, owner_list.X + owner_list.Width + 20,
      owner_list.Y, 105, 8 * 5, MixArchive::RetrieveData("EBTN-UP.SHP"),
      MixArchive::RetrieveData("EBTN-DN.SHP"));
  for (const char* const name : kPersistenceNames) {
    persistence_list.Add_Item(name);
  }
  persistence_list.Set_Selected_Index(static_cast<int>(persistence));

  // This button cycles through the four MultiStyle values for the events;
  // `event_style` is the one showing.
  MultiStyle event_style = event_control;
  TextButtonClass event_control_button(
      kEventControlButton, TXT_TRIGGER_JUST_EVENT, kEButtonFont, kEButtonFlags,
      event1_list.X, event1_list.Y + 11, 100, 9);

  // This button toggles the second action. Linked events always have one, so
  // the button is hidden then and `has_second_action` is not consulted.
  bool has_second_action = action_control != MULTI_ONLY;
  TextButtonClass action_control_button(
      kActionControlButton, TXT_TRIGGER_JUST_ACTION, kEButtonFont,
      kEButtonFlags, action1_list.X, action1_list.Y + 11, 100, 9);

  // Create the ubiquitous OK and Cancel buttons.
  TextButtonClass ok_button(kButtonOk, TXT_OK, kEButtonFont, kEButtonFlags,
                            kDialogX + 35, kDialogY + kDialogH - 30, 45, 9);
  TextButtonClass cancel_button(kButtonCancel, TXT_CANCEL, kEButtonFont,
                                kEButtonFlags, kDialogX + kDialogW - 80,
                                kDialogY + kDialogH - 30, 45, 9);

  PixelView& view = TheScreen().visible_view();

  // Link in the gadgets that are always shown. The second event and action
  // lists and all the data fields are added by the redraw pass.
  gadgets = &ok_button;
  cancel_button.Add_Tail(*gadgets);
  event1_list.Add_Tail(*gadgets);
  action1_list.Add_Tail(*gadgets);
  event_control_button.Add_Tail(*gadgets);
  action_control_button.Add_Tail(*gadgets);
  name_edit.Add_Tail(*gadgets);
  persistence_list.Add_Tail(*gadgets);
  owner_list.Add_Tail(*gadgets);

  // Main Processing Loop
  bool redraw = true;
  bool running = true;
  while (running) {
    // Service the real-time systems while the dialog has the main loop.
    ServiceRealTime();

    // Redraw the dialog and relink its gadgets whenever a choice may have
    // changed what it shows.
    if (redraw) {
      game::TheMouse().Hide();
      Dialog_Box(kDialogX, kDialogY, kDialogW, kDialogH);
      Draw_Caption(view, TXT_TRIGGER_EDITOR, kDialogX, kDialogY, kDialogW);

      // Draw the captions
      Fancy_Text_Print(view, "Trigger Event:", event1_list.X, event1_list.Y - 7,
                       scheme, kTBlack, FontType::kEditor, TPF_NOSHADOW);
      Fancy_Text_Print(view, "Action to Perform:", action1_list.X,
                       action1_list.Y - 7, scheme, kTBlack, FontType::kEditor,
                       TPF_NOSHADOW);
      Fancy_Text_Print(view, "House:", owner_list.X, owner_list.Y - 7, scheme,
                       kTBlack, FontType::kEditor, TPF_NOSHADOW);
      Fancy_Text_Print(view, "Name:", name_edit.X, name_edit.Y - 7, scheme,
                       kTBlack, FontType::kEditor, TPF_NOSHADOW);
      Fancy_Text_Print(view, "Persistence:", persistence_list.X,
                       persistence_list.Y - 7, scheme, kTBlack,
                       FontType::kEditor, TPF_NOSHADOW);

      // For linked events, draw a bracket from each event list to the action
      // list it sets off.
      if (event_style == MULTI_LINKED) {
        view.DrawLine(event1_list.X - 1, event1_list.Y + 3, event1_list.X - 4,
                      event1_list.Y + 3, kWhite);
        view.DrawLine(event1_list.X - 4, event1_list.Y + 3, action1_list.X - 4,
                      action1_list.Y + 3, kWhite);
        view.DrawLine(action1_list.X - 1, action1_list.Y + 3,
                      action1_list.X - 4, action1_list.Y + 3, kWhite);

        view.DrawLine(event2_list.X - 1, event2_list.Y + 3, event2_list.X - 10,
                      event2_list.Y + 3, kWhite);
        view.DrawLine(event2_list.X - 10, event2_list.Y + 3,
                      action2_list.X - 10, action2_list.Y + 3, kWhite);
        view.DrawLine(action2_list.X - 1, action2_list.Y + 3,
                      action2_list.X - 10, action2_list.Y + 3, kWhite);
      }

      // Show the second event list unless there is only one event.
      event2_list.Remove();
      switch (event_style) {
        case MULTI_ONLY:
          event_control_button.Set_Text(TXT_TRIGGER_JUST_EVENT);
          break;

        case MULTI_AND:
          event_control_button.Set_Text(TXT_TRIGGER_AND);
          event2_list.Add(*gadgets);
          break;

        case MULTI_OR:
          event_control_button.Set_Text(TXT_TRIGGER_OR);
          event2_list.Add(*gadgets);
          break;

        case MULTI_LINKED:
          event_control_button.Set_Text(TXT_TRIGGER_LINKED);
          event2_list.Add(*gadgets);
          break;
        default:
          break;
      }

      // Show the data field the first event needs, if any.
      event1_house_list.Remove();
      event1_number_edit.Remove();
      event1_building_list.Remove();
      event1_infantry_list.Remove();
      event1_aircraft_list.Remove();
      event1_unit_list.Remove();
      event1_team_list.Remove();
      switch (Event_Needs(*event1_list.Current_Item())) {
        case NEED_HOUSE:
          event1_house_list.Add(*gadgets);
          break;

        case NEED_TEAM:
          event1_team_list.Add(*gadgets);
          break;


        case NEED_NUMBER:
          event1_number_edit.Add(*gadgets);
          break;

        case NEED_STRUCTURE:
          event1_building_list.Add(*gadgets);
          break;

        case NEED_INFANTRY:
          event1_infantry_list.Add(*gadgets);
          break;

        case NEED_AIRCRAFT:
          event1_aircraft_list.Add(*gadgets);
          break;

        case NEED_UNIT:
          event1_unit_list.Add(*gadgets);
          break;

        case NeedType::NEED_NONE:
        case NeedType::NEED_WAYPOINT:
        case NeedType::NEED_TIME:
        case NeedType::NEED_THEME:
        case NeedType::NEED_MOVIE:
        case NeedType::NEED_SOUND:
        case NeedType::NEED_SPEECH:
        case NeedType::NEED_TRIGGER:
        case NeedType::NEED_QUARRY:
        case NeedType::NEED_FORMATION:
        case NeedType::NEED_BOOL:
        case NeedType::NEED_SPECIAL:
        case NeedType::NEED_MISSION:
        case NeedType::NEED_HEX_NUMBER:
        default:
          break;
      }

      // The same for the second event, when its list is showing.
      event2_house_list.Remove();
      event2_number_edit.Remove();
      event2_building_list.Remove();
      event2_infantry_list.Remove();
      event2_aircraft_list.Remove();
      event2_unit_list.Remove();
      event2_team_list.Remove();
      if (gadgets->Extract_Gadget(kEvent2List)) {
        switch (Event_Needs(*event2_list.Current_Item())) {
          case NEED_HOUSE:
            event2_house_list.Add(*gadgets);
            break;

          case NEED_TEAM:
            event2_team_list.Add(*gadgets);
            break;


          case NEED_NUMBER:
            event2_number_edit.Add(*gadgets);
            break;

          case NEED_STRUCTURE:
            event2_building_list.Add(*gadgets);
            break;

          case NEED_INFANTRY:
            event2_infantry_list.Add(*gadgets);
            break;

          case NEED_AIRCRAFT:
            event2_aircraft_list.Add(*gadgets);
            break;

          case NEED_UNIT:
            event2_unit_list.Add(*gadgets);
            break;

          case NeedType::NEED_NONE:
          case NeedType::NEED_WAYPOINT:
          case NeedType::NEED_TIME:
          case NeedType::NEED_THEME:
          case NeedType::NEED_MOVIE:
          case NeedType::NEED_SOUND:
          case NeedType::NEED_SPEECH:
          case NeedType::NEED_TRIGGER:
          case NeedType::NEED_QUARRY:
          case NeedType::NEED_FORMATION:
          case NeedType::NEED_BOOL:
          case NeedType::NEED_SPECIAL:
          case NeedType::NEED_MISSION:
          case NeedType::NEED_HEX_NUMBER:
          default:
            break;
        }
      }

      // Show the second action list for linked events, or else the button
      // that toggles it and the list when it is toggled on.
      action_control_button.Remove();
      action2_list.Remove();
      if (event_style == MULTI_LINKED) {
        action2_list.Add(*gadgets);
      } else {
        action_control_button.Add(*gadgets);
        if (has_second_action) {
          action_control_button.Set_Text(TXT_TRIGGER_AND);
          action2_list.Add(*gadgets);
        } else {
          action_control_button.Set_Text(TXT_TRIGGER_JUST_ACTION);
        }
      }

      // Show the data field the first action needs, if any.
      action1_quarry_list.Remove();
      action1_special_list.Remove();
      action1_house_list.Remove();
      action1_bool_list.Remove();
      action1_trigger_list.Remove();
      action1_waypoint_edit.Remove();
      action1_number_edit.Remove();
      action1_team_list.Remove();
      action1_theme_list.Remove();
      action1_sound_list.Remove();
      action1_movie_list.Remove();
      action1_speech_list.Remove();
      switch (ActionNeed(*action1_list.Current_Item())) {
        case NEED_MOVIE:
          action1_movie_list.Add(*gadgets);
          break;

        case NEED_SPECIAL:
          action1_special_list.Add(*gadgets);
          break;

        case NEED_HOUSE:
          action1_house_list.Add(*gadgets);
          break;

        case NEED_BOOL:
          action1_bool_list.Add(*gadgets);
          break;

        case NEED_TRIGGER:
          action1_trigger_list.Add(*gadgets);
          break;

        case NEED_TEAM:
          action1_team_list.Add(*gadgets);
          break;

        case NEED_NUMBER:
          action1_number_edit.Add(*gadgets);
          break;

        case NEED_WAYPOINT:
          action1_waypoint_edit.Add(*gadgets);
          break;

        case NEED_THEME:
          action1_theme_list.Add(*gadgets);
          break;

        case NEED_SOUND:
          action1_sound_list.Add(*gadgets);
          break;

        case NEED_SPEECH:
          action1_speech_list.Add(*gadgets);
          break;

        case NEED_QUARRY:
          action1_quarry_list.Add(*gadgets);
          break;
        case NeedType::NEED_NONE:
        case NeedType::NEED_INFANTRY:
        case NeedType::NEED_UNIT:
        case NeedType::NEED_AIRCRAFT:
        case NeedType::NEED_STRUCTURE:
        case NeedType::NEED_TIME:
        case NeedType::NEED_FORMATION:
        case NeedType::NEED_MISSION:
        case NeedType::NEED_HEX_NUMBER:
        default:
          break;
      }

      // The same for the second action, when its list is showing.
      action2_quarry_list.Remove();
      action2_special_list.Remove();
      action2_house_list.Remove();
      action2_bool_list.Remove();
      action2_trigger_list.Remove();
      action2_waypoint_edit.Remove();
      action2_number_edit.Remove();
      action2_team_list.Remove();
      action2_theme_list.Remove();
      action2_sound_list.Remove();
      action2_movie_list.Remove();
      action2_speech_list.Remove();
      if (gadgets->Extract_Gadget(kAction2List)) {
        switch (ActionNeed(*action2_list.Current_Item())) {
          case NEED_MOVIE:
            action2_movie_list.Add(*gadgets);
            break;

          case NEED_SPECIAL:
            action2_special_list.Add(*gadgets);
            break;

          case NEED_HOUSE:
            action2_house_list.Add(*gadgets);
            break;

          case NEED_BOOL:
            action2_bool_list.Add(*gadgets);
            break;

          case NEED_TRIGGER:
            action2_trigger_list.Add(*gadgets);
            break;

          case NEED_TEAM:
            action2_team_list.Add(*gadgets);
            break;

          case NEED_NUMBER:
            action2_number_edit.Add(*gadgets);
            break;

          case NEED_WAYPOINT:
            action2_waypoint_edit.Add(*gadgets);
            break;

          case NEED_THEME:
            action2_theme_list.Add(*gadgets);
            break;

          case NEED_SOUND:
            action2_sound_list.Add(*gadgets);
            break;

          case NEED_SPEECH:
            action2_speech_list.Add(*gadgets);
            break;

          case NEED_QUARRY:
            action2_quarry_list.Add(*gadgets);
            break;
          case NeedType::NEED_NONE:
          case NeedType::NEED_INFANTRY:
          case NeedType::NEED_UNIT:
          case NeedType::NEED_AIRCRAFT:
          case NeedType::NEED_STRUCTURE:
          case NeedType::NEED_TIME:
          case NeedType::NEED_FORMATION:
          case NeedType::NEED_MISSION:
          case NeedType::NEED_HEX_NUMBER:
          default:
            break;
        }
      }

      // Collapse any dropped down list: the dialog was just redrawn over its
      // open part.
      action1_special_list.Collapse();
      action2_special_list.Collapse();
      action1_quarry_list.Collapse();
      action2_quarry_list.Collapse();
      event1_house_list.Collapse();
      event2_house_list.Collapse();
      action1_house_list.Collapse();
      action2_house_list.Collapse();
      event1_team_list.Collapse();
      event2_team_list.Collapse();
      action1_team_list.Collapse();
      action2_team_list.Collapse();
      event1_building_list.Collapse();
      event2_building_list.Collapse();
      event1_unit_list.Collapse();
      event2_unit_list.Collapse();
      event1_infantry_list.Collapse();
      event2_infantry_list.Collapse();
      event1_aircraft_list.Collapse();
      event2_aircraft_list.Collapse();
      action1_trigger_list.Collapse();
      action2_trigger_list.Collapse();
      action1_list.Collapse();
      action2_list.Collapse();
      event1_list.Collapse();
      event2_list.Collapse();
      owner_list.Collapse();
      persistence_list.Collapse();
      action1_bool_list.Collapse();
      action2_bool_list.Collapse();
      action1_theme_list.Collapse();
      action2_theme_list.Collapse();
      action1_sound_list.Collapse();
      action2_sound_list.Collapse();
      action1_movie_list.Collapse();
      action2_movie_list.Collapse();
      action1_speech_list.Collapse();
      action2_speech_list.Collapse();
      gadgets->Flag_List_To_Redraw();
      game::TheMouse().Show();
      redraw = false;
    }

    const engine::window::KeyNumber input = gadgets->Input(view).key;

    switch (static_cast<int>(input)) {
      case engine::window::ButtonKey(kEventControlButton):
        event_style = static_cast<MultiStyle>(
            (std::to_underlying(event_style) + 1) %
            static_cast<int>(magic_enum::enum_count<MultiStyle>()));
        redraw = true;
        break;

      case engine::window::ButtonKey(kActionControlButton):
        has_second_action = !has_second_action;
        redraw = true;
        break;

      case engine::window::ButtonKey(kAction1Speech):
        Speak(VoxType(action1_speech_list.Current_Index()));
        redraw = true;
        break;

      case engine::window::ButtonKey(kAction2Speech):
        Speak(VoxType(action2_speech_list.Current_Index()));
        redraw = true;
        break;

      case engine::window::ButtonKey(kAction1Sound):
        PlaySoundEffect(VocType(action1_sound_list.Current_Index()));
        redraw = true;
        break;

      case engine::window::ButtonKey(kAction2Sound):
        PlaySoundEffect(VocType(action2_sound_list.Current_Index()));
        redraw = true;
        break;

      // Store the dialog's values in the trigger type. Each event and action
      // takes only the field it needs; the rest of its data is left alone.
      case KN_RETURN:
      case engine::window::ButtonKey(kButtonOk):
        house = HousesType(owner_list.Current_Index());
        persistence = Persistence(persistence_list.Current_Index());
        if (std::string_view(name_text).empty()) {
          Set_Name("____");
        } else {
          Set_Name(name_text);
        }

        // The first event and its data.
        event_control = event_style;
        event1.Event = *event1_list.Current_Item();
        switch (Event_Needs(event1.Event)) {
          case NEED_HOUSE:
            event1.Data.House = HousesType(event1_house_list.Current_Index());
            break;

          case NEED_NUMBER:
            event1.Data.Value =
                base::ParseIntegerOr<int>(event1_number_edit.Get_Text(), 0);
            break;

          case NEED_STRUCTURE:
            event1.Data.Structure =
                StructType(event1_building_list.Current_Index());
            break;

          case NEED_UNIT:
            event1.Data.Unit = UnitType(event1_unit_list.Current_Index());
            break;

          case NEED_INFANTRY:
            event1.Data.Infantry =
                InfantryType(event1_infantry_list.Current_Index());
            break;

          case NEED_AIRCRAFT:
            event1.Data.Aircraft =
                AircraftType(event1_aircraft_list.Current_Index());
            break;


          case NEED_TEAM:
            event1.Team = TeamType::FromName(SelectedName(event1_team_list));
            break;
          case NeedType::NEED_NONE:
          case NeedType::NEED_WAYPOINT:
          case NeedType::NEED_TIME:
          case NeedType::NEED_THEME:
          case NeedType::NEED_MOVIE:
          case NeedType::NEED_SOUND:
          case NeedType::NEED_SPEECH:
          case NeedType::NEED_TRIGGER:
          case NeedType::NEED_QUARRY:
          case NeedType::NEED_FORMATION:
          case NeedType::NEED_BOOL:
          case NeedType::NEED_SPECIAL:
          case NeedType::NEED_MISSION:
          case NeedType::NEED_HEX_NUMBER:
          default:
            break;
        }

        // The second event and its data. They are stored even when only one
        // event is in use.
        event2.Event = *event2_list.Current_Item();
        switch (Event_Needs(event2.Event)) {
          case NEED_HOUSE:
            event2.Data.House = HousesType(event2_house_list.Current_Index());
            break;

          case NEED_NUMBER:
            event2.Data.Value =
                base::ParseIntegerOr<int>(event2_number_edit.Get_Text(), 0);
            break;

          case NEED_STRUCTURE:
            event2.Data.Structure =
                StructType(event2_building_list.Current_Index());
            break;

          case NEED_UNIT:
            event2.Data.Unit = UnitType(event2_unit_list.Current_Index());
            break;

          case NEED_INFANTRY:
            event2.Data.Infantry =
                InfantryType(event2_infantry_list.Current_Index());
            break;

          case NEED_AIRCRAFT:
            event2.Data.Aircraft =
                AircraftType(event2_aircraft_list.Current_Index());
            break;


          case NEED_TEAM:
            event2.Team = TeamType::FromName(SelectedName(event2_team_list));
            break;
          case NeedType::NEED_NONE:
          case NeedType::NEED_WAYPOINT:
          case NeedType::NEED_TIME:
          case NeedType::NEED_THEME:
          case NeedType::NEED_MOVIE:
          case NeedType::NEED_SOUND:
          case NeedType::NEED_SPEECH:
          case NeedType::NEED_TRIGGER:
          case NeedType::NEED_QUARRY:
          case NeedType::NEED_FORMATION:
          case NeedType::NEED_BOOL:
          case NeedType::NEED_SPECIAL:
          case NeedType::NEED_MISSION:
          case NeedType::NEED_HEX_NUMBER:
          default:
            break;
        }

        // The first action and its data. `has_second_action` converts to
        // MULTI_ONLY or MULTI_AND.
        action_control = MultiStyle(has_second_action);
        action1_record.type = *action1_list.Current_Item();
        switch (ActionNeed(action1_record.type)) {
          case NEED_SPECIAL:
            action1_record.value = action1_special_list.Current_Index();
            break;

          case NEED_HOUSE:
            action1_record.value = action1_house_list.Current_Index();
            break;

          case NEED_TRIGGER:
            action1_record.trigger_type =
                TriggerType::FromName(SelectedName(action1_trigger_list));
            break;

          case NEED_TEAM:
            action1_record.team_type =
                TeamType::FromName(SelectedName(action1_team_list));
            break;

          case NEED_NUMBER:
            action1_record.value =
                base::ParseIntegerOr<int>(action1_number_edit.Get_Text(), 0);
            break;

          case NEED_WAYPOINT:
            action1_record.value = WaypointFromLetters(action1_waypoint_text);
            break;

          case NEED_BOOL:
            action1_record.value =
                action1_bool_list.Current_Index() != 0 ? 1 : 0;
            break;

          case NEED_THEME:
            action1_record.value = action1_theme_list.Current_Index();
            break;

          case NEED_SOUND:
            action1_record.value = action1_sound_list.Current_Index();
            break;

          case NEED_MOVIE:
            action1_record.value = action1_movie_list.Current_Index();
            break;

          case NEED_SPEECH:
            action1_record.value = action1_speech_list.Current_Index();
            break;

          case NEED_QUARRY:
            action1_record.value = action1_quarry_list.Current_Index();
            break;
          case NeedType::NEED_NONE:
          case NeedType::NEED_INFANTRY:
          case NeedType::NEED_UNIT:
          case NeedType::NEED_AIRCRAFT:
          case NeedType::NEED_STRUCTURE:
          case NeedType::NEED_TIME:
          case NeedType::NEED_FORMATION:
          case NeedType::NEED_MISSION:
          case NeedType::NEED_HEX_NUMBER:
          default:
            break;
        }

        // The second action and its data, stored even when it is not in use.
        action2_record.type = *action2_list.Current_Item();
        switch (ActionNeed(action2_record.type)) {
          case NEED_SPECIAL:
            action2_record.value = action2_special_list.Current_Index();
            break;

          case NEED_HOUSE:
            action2_record.value = action2_house_list.Current_Index();
            break;

          case NEED_TRIGGER:
            action2_record.trigger_type =
                TriggerType::FromName(SelectedName(action2_trigger_list));
            break;

          case NEED_TEAM:
            action2_record.team_type =
                TeamType::FromName(SelectedName(action2_team_list));
            break;

          case NEED_NUMBER:
            action2_record.value =
                base::ParseIntegerOr<int>(action2_number_edit.Get_Text(), 0);
            break;

          case NEED_WAYPOINT:
            action2_record.value = WaypointFromLetters(action2_waypoint_text);
            break;

          case NEED_BOOL:
            action2_record.value =
                action2_bool_list.Current_Index() != 0 ? 1 : 0;
            break;

          case NEED_THEME:
            action2_record.value = action2_theme_list.Current_Index();
            break;

          case NEED_MOVIE:
            action2_record.value = action2_movie_list.Current_Index();
            break;

          case NEED_SOUND:
            action2_record.value = action2_sound_list.Current_Index();
            break;

          case NEED_SPEECH:
            action2_record.value = action2_speech_list.Current_Index();
            break;

          case NEED_QUARRY:
            action2_record.value = action2_quarry_list.Current_Index();
            break;
          case NeedType::NEED_NONE:
          case NeedType::NEED_INFANTRY:
          case NeedType::NEED_UNIT:
          case NeedType::NEED_AIRCRAFT:
          case NeedType::NEED_STRUCTURE:
          case NeedType::NEED_TIME:
          case NeedType::NEED_FORMATION:
          case NeedType::NEED_MISSION:
          case NeedType::NEED_HEX_NUMBER:
          default:
            break;
        }
        action1 = MakeTriggerAction(action1_record);
        action2 = MakeTriggerAction(action2_record);
        return true;

      case KN_ESC:
      case engine::window::ButtonKey(kButtonCancel):
        running = false;
        [[fallthrough]];

      // Any gadget may have changed what the dialog should show, so redraw
      // whenever the input came from one: those carry the button bit.
      default:
        if (input & KN_BUTTON) {
          redraw = true;
        }
        break;
    }
  }
  return false;
}

}  // namespace ra
