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

#include "ra/ai/trigtype.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <format>
#include <string>
#include <string_view>
#include <utility>

#include "absl/base/attributes.h"
#include "absl/strings/ascii.h"
#include "absl/strings/match.h"
#include "absl/strings/str_format.h"
#include "base/enum_array.h"
#include "base/numeric.h"
#include "base/pooled.h"
#include "base/strings/number_parse.h"
#include "base/strings/safe_string.h"
#include "base/strings/tokenizer.h"
#include "engine/file/ini_file.h"
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
#include "ra/assets/audio.h"
#include "ra/assets/palettes.h"
#include "ra/assets/screen.h"
#include "ra/assets/theme.h"
#include "ra/core/config.h"
#include "ra/core/const.h"
#include "ra/core/defines.h"
#include "ra/core/inline.h"
#include "ra/core/object_heaps.h"
#include "ra/data/text_ids.h"
#include "ra/debug/debug.h"
#include "ra/map/world.h"
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
  ActionChoice(TriggerActionType action = TACTION_NONE) noexcept
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

// One choice per action, in enum order. The editor's drop lists hold pointers
// to these, so they need storage that outlives the dialog.
base::EnumArray<TriggerActionType, ActionChoice> action_choices = {
    {{TACTION_NONE},
     {TACTION_WIN},
     {TACTION_LOSE},
     {TACTION_BEGIN_PRODUCTION},
     {TACTION_CREATE_TEAM},
     {TACTION_DESTROY_TEAM},
     {TACTION_ALL_HUNT},
     {TACTION_REINFORCEMENTS},
     {TACTION_DZ},
     {TACTION_FIRE_SALE},
     {TACTION_PLAY_MOVIE},
     {TACTION_TEXT_TRIGGER},
     {TACTION_DESTROY_TRIGGER},
     {TACTION_AUTOCREATE},
     {TACTION_WINLOSE},
     {TACTION_ALLOWWIN},
     {TACTION_REVEAL_ALL},
     {TACTION_REVEAL_SOME},
     {TACTION_REVEAL_ZONE},
     {TACTION_PLAY_SOUND},
     {TACTION_PLAY_MUSIC},
     {TACTION_PLAY_SPEECH},
     {TACTION_FORCE_TRIGGER},
     {TACTION_START_TIMER},
     {TACTION_STOP_TIMER},
     {TACTION_ADD_TIMER},
     {TACTION_SUB_TIMER},
     {TACTION_SET_TIMER},
     {TACTION_SET_GLOBAL},
     {TACTION_CLEAR_GLOBAL},
     {TACTION_BASE_BUILDING},
     {TACTION_CREEP_SHADOW},
     {TACTION_DESTROY_OBJECT},
     {TACTION_1_SPECIAL},
     {TACTION_FULL_SPECIAL},
     {TACTION_PREFERRED_TARGET},
     {TACTION_LAUNCH_NUKES}}};

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

TriggerTypeClass::TriggerTypeClass()
    : AbstractTypeClass(RTTI_TRIGGERTYPE,
                        TheObjectHeaps().trigger_type().SlotIndexOf(this),
                        TXT_NONE, "x") {}

TriggerTypeClass::~TriggerTypeClass() = default;

void* TriggerTypeClass::operator new(std::size_t /*unused*/) {
  void* ptr = TheObjectHeaps().trigger_type().Allocate();
  if (ptr) {
    static_cast<TriggerTypeClass*>(ptr)->IsActive = true;
  }

  return ptr;
}

void TriggerTypeClass::operator delete(void* ptr) {
  if (ptr) {
    static_cast<TriggerTypeClass*>(ptr)->IsActive = false;
  }
  TheObjectHeaps().trigger_type().Free(static_cast<TriggerTypeClass*>(ptr));
}

void TriggerTypeClass::Detach(Target target, bool /*unused*/) {
  // TODO: Event1.Team and Event2.Team are not cleared when `target` is their
  // team type, so they dangle once the editor deletes it.
  Action1.Detach(target);
  Action2.Detach(target);
}

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
constexpr int kE1X = kDialogX + 45;
constexpr int kE1Y = kDialogY + 65;
constexpr int kE2X = kE1X;
constexpr int kE2Y = kE1Y + 22;
constexpr int kEWidth = 160;
constexpr int kEHeight = 8 * 5;

// Event data field coordinates and dimensions, to the right of each list.
constexpr int kEd1X = kE1X + kEWidth + 20;
constexpr int kEd1Y = kE1Y;
constexpr int kEd2X = kEd1X;
constexpr int kEd2Y = kE2Y;

constexpr int kEdWidth = 95;
constexpr int kEdHeight = 8 * 5;

// Action drop list coordinates; the lists share the event lists' size.
constexpr int kA1X = kE1X;
constexpr int kA1Y = kDialogY + 120;
constexpr int kA2X = kE1X;
constexpr int kA2Y = kA1Y + 22;

// Action data field coordinates.
constexpr int kAd1X = kA1X + kEWidth + 20;
constexpr int kAd1Y = kA1Y;
constexpr int kAd2X = kAd1X;
constexpr int kAd2Y = kA2Y;

// Text buffer sizes, each counting the terminating NUL.
constexpr int kGeneralSize = 10;  // Text length for general data entry fields.
constexpr int kEntrySize =
    35;  // Maximum size of event or action description text.
constexpr int kWaypointSize = 3;  // Text length maximum for waypoint entry.
constexpr int kTeamSize = 10;     // Team name text entry field length.
constexpr int kDescSize =
    35;  // Maximum length of object full name description.

// Gadget IDs. "Primary" is the first event or action, "secondary" the second.
constexpr int kEventList = 100;      // Primary event list.
constexpr int kEventList2 = 101;     // Secondary event list.
constexpr int kActionList = 102;     // Primary action list.
constexpr int kActionList2 = 103;    // Secondary action list.
constexpr int kNameEdit = 104;       // Trigger name edit field.
constexpr int kDataSpeech1 = 105;    // Primary action speech.
constexpr int kDataSpeech2 = 106;    // Secondary action speech.
constexpr int kDataTheme1 = 107;     // Primary action theme.
constexpr int kDataTheme2 = 108;     // Secondary action theme.
constexpr int kDataMovie1 = 109;     // Primary action movie.
constexpr int kDataMovie2 = 110;     // Secondary action movie.
constexpr int kDataSound1 = 111;     // Primary action sound effect.
constexpr int kDataSound2 = 112;     // Secondary action sound effect.
constexpr int kDataSpecial1 = 113;   // Primary action special weapon.
constexpr int kDataSpecial2 = 114;   // Secondary action special weapon.
constexpr int kDataEdit = 115;       // Primary event waypoint data field.
constexpr int kDataEdit2 = 116;      // Secondary event waypoint data field.
constexpr int kDataEdit3 = 117;      // Primary action waypoint data field.
constexpr int kDataEdit4 = 118;      // Secondary action waypoint data field.
constexpr int kDataHtype1 = 119;     // Primary event house choice list.
constexpr int kDataHtype2 = 120;     // Secondary event house choice list.
constexpr int kDataHtype3 = 121;     // Primary action house choice list.
constexpr int kDataHtype4 = 122;     // Secondary action house choice list.
constexpr int kDataBooltype1 = 123;  // Primary action boolean data list.
constexpr int kDataBooltype2 = 124;  // Secondary action boolean data list.
constexpr int kDataGeneral1 = 125;   // Primary event general data field.
constexpr int kDataGeneral2 = 126;   // Secondary event general data field.
constexpr int kDataGeneral3 = 127;   // Primary action general data field.
constexpr int kDataGeneral4 = 128;   // Secondary action general data field.
constexpr int kDataBtype1 = 129;     // Primary event building type list.
constexpr int kDataBtype2 = 130;     // Secondary event building type list.
constexpr int kDataItype1 = 131;     // Primary event infantry type list.
constexpr int kDataItype2 = 132;     // Secondary event infantry type list.
constexpr int kDataAtype1 = 133;     // Primary event aircraft type list.
constexpr int kDataAtype2 = 134;     // Secondary event aircraft type list.
constexpr int kDataUtype1 = 135;     // Primary event unit type list.
constexpr int kDataUtype2 = 136;     // Secondary event unit type list.
constexpr int kDataTtype1 = 137;     // Primary event team type entry list.
constexpr int kDataTtype2 = 138;     // Secondary event team type entry list.
constexpr int kDataTtype3 = 139;     // Primary action team type entry list.
constexpr int kDataTtype4 = 140;     // Secondary action team type entry list.
constexpr int kDataTrtype1 = 141;    // Primary action trigger list.
constexpr int kDataTrtype2 = 142;    // Secondary action trigger list.
constexpr int kButtonHouse = 143;    // House ownership for this trigger.
constexpr int kButtonPersistance = 144;  // Persistence of this trigger.
constexpr int kButtonOk = 145;           // Ok button - save and exit.
constexpr int kKbuttoncancel = 146;      // Cancel button - just exit.
constexpr int kButtonAction = 147;       // Multiple action control button.
constexpr int kButtonEvent = 148;        // Multiple event control button.

bool TriggerTypeClass::Edit() {

  RemapControlType* scheme = Get_Color_Scheme();

  // Head of the dialog's gadget list. Only the gadgets linked into it are
  // drawn and receive input; the redraw pass below relinks the data fields to
  // match the chosen events and actions.
  game::ControlClass* commands = nullptr;

  // The two event lists, each offering every event, sorted by name.
  char eventtext[kEntrySize] = "";
  TDropListClass<EventChoiceClass*> event1list(
      kEventList, eventtext, sizeof(eventtext), FontType::kEditor, TPF_NOSHADOW,
      kE1X, kE1Y, kEWidth, kEHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
      MixArchive::RetrieveData("EBTN-DN.SHP"));
  char event2text[kEntrySize] = "";
  TDropListClass<EventChoiceClass*> event2list(
      kEventList2, event2text, sizeof(event2text), FontType::kEditor,
      TPF_NOSHADOW, kE2X, kE2Y, kEWidth, kEHeight,
      MixArchive::RetrieveData("EBTN-UP.SHP"),
      MixArchive::RetrieveData("EBTN-DN.SHP"));
  for (TEventType event = TEVENT_NONE; event < TEVENT_COUNT; event++) {
    event1list.Add_Item(&EventChoices.at(event));
    event2list.Add_Item(&EventChoices.at(event));
  }

  const auto by_description = [](const auto* left, const auto* right) {
    return *left < *right;
  };
  event1list.Sort(by_description);
  event2list.Sort(by_description);

  event1list.Set_Selected_Index(&EventChoices.at(Event1.Event));
  event2list.Set_Selected_Index(&EventChoices.at(Event2.Event));

  // The two action lists, likewise.
  char actiontext[kEntrySize] = "";
  TDropListClass<ActionChoice*> action1list(
      kActionList, actiontext, sizeof(actiontext), FontType::kEditor,
      TPF_NOSHADOW, kA1X, kA1Y, kEWidth, kEHeight,
      MixArchive::RetrieveData("EBTN-UP.SHP"),
      MixArchive::RetrieveData("EBTN-DN.SHP"));
  char action2text[kEntrySize] = "";
  TDropListClass<ActionChoice*> action2list(
      kActionList2, action2text, sizeof(action2text), FontType::kEditor,
      TPF_NOSHADOW, kA2X, kA2Y, kEWidth, kEHeight,
      MixArchive::RetrieveData("EBTN-UP.SHP"),
      MixArchive::RetrieveData("EBTN-DN.SHP"));
  for (const TriggerActionType action :
       magic_enum::enum_values<TriggerActionType>()) {
    action1list.Add_Item(&action_choices.at(action));
    action2list.Add_Item(&action_choices.at(action));
  }

  action1list.Sort(by_description);
  action2list.Sort(by_description);

  action1list.Set_Selected_Index(&action_choices.at(Action1.type));
  action2list.Set_Selected_Index(&action_choices.at(Action2.type));

  // Waypoint fields, one per event and action. A waypoint number shows as a
  // letter, 'A' for 0 to 'Z' for 25, and from 26 on as two letters, "AA" for
  // 26 to "ZZ" for 701.
  char way1[kWaypointSize] = "A";
  EditClass way1data(kDataEdit, way1, sizeof(way1), FontType::kEditor,
                     TPF_NOSHADOW, kEd1X, kEd1Y, kEdWidth, 9,
                     EditClass::kAlpha);
  if (Event_Needs(Event1.Event) == NEED_WAYPOINT) {
    if (Event1.Data.Value < 26) {
      absl::SNPrintF(way1, sizeof(way1), "%c",
                     static_cast<int>(Event1.Data.Value) + 'A');
    } else {
      absl::SNPrintF(way1, sizeof(way1), "%c%c",
                     (Event1.Data.Value / 26) + 'A' - 1,
                     (Event1.Data.Value % 26) + 'A');
    }
  }

  char way2[kWaypointSize] = "A";
  EditClass way2data(kDataEdit2, way2, sizeof(way2), FontType::kEditor,
                     TPF_NOSHADOW, kEd2X, kEd2Y, kEdWidth, 9,
                     EditClass::kAlpha);
  if (Event_Needs(Event2.Event) == NEED_WAYPOINT) {
    if (Event2.Data.Value < 26) {
      absl::SNPrintF(way2, sizeof(way2), "%c",
                     static_cast<int>(Event2.Data.Value) + 'A');
    } else {
      absl::SNPrintF(way2, sizeof(way2), "%c%c",
                     (Event2.Data.Value / 26) + 'A' - 1,
                     (Event2.Data.Value % 26) + 'A');
    }
  }

  char way3[kWaypointSize] = "A";
  EditClass way3data(kDataEdit3, way3, sizeof(way3), FontType::kEditor,
                     TPF_NOSHADOW, kAd1X, kAd1Y, kEdWidth, 9,
                     EditClass::kAlpha);
  if (ActionNeed(Action1.type) == NEED_WAYPOINT) {
    if (Action1.data.value < 26) {
      absl::SNPrintF(way3, sizeof(way3), "%c", Action1.data.value + 'A');
    } else {
      absl::SNPrintF(way3, sizeof(way3), "%c%c",
                     (Action1.data.value / 26) + 'A' - 1,
                     (Action1.data.value % 26) + 'A');
    }
  }

  char way4[kWaypointSize] = "A";
  EditClass way4data(kDataEdit4, way4, sizeof(way4), FontType::kEditor,
                     TPF_NOSHADOW, kAd2X, kAd2Y, kEdWidth, 9,
                     EditClass::kAlpha);
  if (ActionNeed(Action2.type) == NEED_WAYPOINT) {
    if (Action2.data.value < 26) {
      absl::SNPrintF(way4, sizeof(way4), "%c", Action2.data.value + 'A');
    } else {
      absl::SNPrintF(way4, sizeof(way4), "%c%c",
                     (Action2.data.value / 26) + 'A' - 1,
                     (Action2.data.value % 26) + 'A');
    }
  }

  // Number fields, one per event and action.
  char databuf1[kGeneralSize] = "";
  EditClass event1data(kDataGeneral1, databuf1, sizeof(databuf1),
                       FontType::kEditor, TPF_NOSHADOW, kEd1X, kEd1Y, kEdWidth,
                       9, EditClass::kNumeric);
  switch (Event_Needs(Event1.Event)) {
    case NEED_TIME:
    case NEED_NUMBER:
      absl::SNPrintF(databuf1, sizeof(databuf1), "%d",
                     static_cast<int>(Event1.Data.Value));
      break;
    case NeedType::NEED_NONE:
    case NeedType::NEED_THEME:
    case NeedType::NEED_MOVIE:
    case NeedType::NEED_SOUND:
    case NeedType::NEED_SPEECH:
    case NeedType::NEED_INFANTRY:
    case NeedType::NEED_UNIT:
    case NeedType::NEED_AIRCRAFT:
    case NeedType::NEED_STRUCTURE:
    case NeedType::NEED_WAYPOINT:
    case NeedType::NEED_TRIGGER:
    case NeedType::NEED_TEAM:
    case NeedType::NEED_HOUSE:
    case NeedType::NEED_QUARRY:
    case NeedType::NEED_FORMATION:
    case NeedType::NEED_BOOL:
    case NeedType::NEED_SPECIAL:
    case NeedType::NEED_MISSION:
    case NeedType::NEED_HEX_NUMBER:
    default:
      break;
  }

  char databuf2[kGeneralSize] = "";
  EditClass event2data(kDataGeneral2, databuf2, sizeof(databuf2),
                       FontType::kEditor, TPF_NOSHADOW, kEd2X, kEd2Y, kEdWidth,
                       9, EditClass::kNumeric);
  switch (Event_Needs(Event2.Event)) {
    case NEED_TIME:
    case NEED_NUMBER:
      absl::SNPrintF(databuf2, sizeof(databuf2), "%d",
                     static_cast<int>(Event2.Data.Value));
      break;
    case NeedType::NEED_NONE:
    case NeedType::NEED_THEME:
    case NeedType::NEED_MOVIE:
    case NeedType::NEED_SOUND:
    case NeedType::NEED_SPEECH:
    case NeedType::NEED_INFANTRY:
    case NeedType::NEED_UNIT:
    case NeedType::NEED_AIRCRAFT:
    case NeedType::NEED_STRUCTURE:
    case NeedType::NEED_WAYPOINT:
    case NeedType::NEED_TRIGGER:
    case NeedType::NEED_TEAM:
    case NeedType::NEED_HOUSE:
    case NeedType::NEED_QUARRY:
    case NeedType::NEED_FORMATION:
    case NeedType::NEED_BOOL:
    case NeedType::NEED_SPECIAL:
    case NeedType::NEED_MISSION:
    case NeedType::NEED_HEX_NUMBER:
    default:
      break;
  }

  char actionbuf1[kGeneralSize] = "";
  EditClass action1data(kDataGeneral3, actionbuf1, sizeof(actionbuf1),
                        FontType::kEditor, TPF_NOSHADOW, kAd1X, kAd1Y, kEdWidth,
                        9, EditClass::kNumeric);
  if (ActionNeed(Action1.type) == NEED_NUMBER) {
    absl::SNPrintF(actionbuf1, sizeof(actionbuf1), "%d", Action1.data.value);
  }

  char actionbuf2[kGeneralSize] = "";
  EditClass action2data(kDataGeneral4, actionbuf2, sizeof(actionbuf2),
                        FontType::kEditor, TPF_NOSHADOW, kAd2X, kAd2Y, kEdWidth,
                        9, EditClass::kNumeric);
  if (ActionNeed(Action2.type) == NEED_NUMBER) {
    absl::SNPrintF(actionbuf2, sizeof(actionbuf2), "%d", Action2.data.value);
  }

  // Team type lists, one per event and action, by INI name.
  char tbuf1[kTeamSize] = "";
  DropListClass ttype1list(kDataTtype1, tbuf1, sizeof(tbuf1), FontType::kEditor,
                           TPF_NOSHADOW, kEd1X, kEd1Y, kEdWidth, kEdHeight,
                           MixArchive::RetrieveData("EBTN-UP.SHP"),
                           MixArchive::RetrieveData("EBTN-DN.SHP"));
  char tbuf2[kTeamSize] = "";
  DropListClass ttype2list(kDataTtype2, tbuf2, sizeof(tbuf2), FontType::kEditor,
                           TPF_NOSHADOW, kEd2X, kEd2Y, kEdWidth, kEdHeight,
                           MixArchive::RetrieveData("EBTN-UP.SHP"),
                           MixArchive::RetrieveData("EBTN-DN.SHP"));
  char tbuf3[kTeamSize] = "";
  DropListClass ttype3list(kDataTtype3, tbuf3, sizeof(tbuf3), FontType::kEditor,
                           TPF_NOSHADOW, kAd1X, kAd1Y, kEdWidth, kEdHeight,
                           MixArchive::RetrieveData("EBTN-UP.SHP"),
                           MixArchive::RetrieveData("EBTN-DN.SHP"));
  char tbuf4[kTeamSize] = "";
  DropListClass ttype4list(kDataTtype4, tbuf4, sizeof(tbuf4), FontType::kEditor,
                           TPF_NOSHADOW, kAd2X, kAd2Y, kEdWidth, kEdHeight,
                           MixArchive::RetrieveData("EBTN-UP.SHP"),
                           MixArchive::RetrieveData("EBTN-DN.SHP"));

  for (int index = 0; index < TheObjectHeaps().team_type().Count(); index++) {
    ttype1list.Add_Item(TheObjectHeaps().team_type().ActiveAt(index)->IniName);
    ttype2list.Add_Item(TheObjectHeaps().team_type().ActiveAt(index)->IniName);
    ttype3list.Add_Item(TheObjectHeaps().team_type().ActiveAt(index)->IniName);
    ttype4list.Add_Item(TheObjectHeaps().team_type().ActiveAt(index)->IniName);
  }

  if (Event1.Team != nullptr) {
    ttype1list.Set_Selected_Index(Event1.Team->IniName);
  } else {
    ttype1list.Set_Selected_Index(0);
  }
  if (Event2.Team != nullptr) {
    ttype2list.Set_Selected_Index(Event2.Team->IniName);
  } else {
    ttype2list.Set_Selected_Index(0);
  }
  if (Action1.team_type != nullptr) {
    ttype3list.Set_Selected_Index(Action1.team_type->IniName);
  } else {
    ttype3list.Set_Selected_Index(0);
  }
  if (Action2.team_type != nullptr) {
    ttype4list.Set_Selected_Index(Action2.team_type->IniName);
  } else {
    ttype4list.Set_Selected_Index(0);
  }

  // Trigger type lists for the actions, by INI name. They include this
  // trigger type itself.
  char trbuf1[kTeamSize] = "";
  DropListClass trtype1list(kDataTrtype1, trbuf1, sizeof(trbuf1),
                            FontType::kEditor, TPF_NOSHADOW, kAd1X, kAd1Y,
                            kEdWidth, kEdHeight,
                            MixArchive::RetrieveData("EBTN-UP.SHP"),
                            MixArchive::RetrieveData("EBTN-DN.SHP"));
  char trbuf2[kTeamSize] = "";
  DropListClass trtype2list(kDataTrtype2, trbuf2, sizeof(trbuf2),
                            FontType::kEditor, TPF_NOSHADOW, kAd2X, kAd2Y,
                            kEdWidth, kEdHeight,
                            MixArchive::RetrieveData("EBTN-UP.SHP"),
                            MixArchive::RetrieveData("EBTN-DN.SHP"));

  for (int index = 0; index < TheObjectHeaps().trigger_type().Count();
       index++) {
    trtype1list.Add_Item(
        TheObjectHeaps().trigger_type().ActiveAt(index)->IniName);
    trtype2list.Add_Item(
        TheObjectHeaps().trigger_type().ActiveAt(index)->IniName);
  }

  if (Action1.trigger_type != nullptr) {
    trtype1list.Set_Selected_Index(Action1.trigger_type->IniName);
  } else {
    trtype1list.Set_Selected_Index(0);
  }
  if (Action2.trigger_type != nullptr) {
    trtype2list.Set_Selected_Index(Action2.trigger_type->IniName);
  } else {
    trtype2list.Set_Selected_Index(0);
  }

  // On/off lists for the actions.
  char boolbuf1[kTeamSize] = "";
  DropListClass booltype1list(kDataBooltype1, boolbuf1, sizeof(boolbuf1),
                              FontType::kEditor, TPF_NOSHADOW, kAd1X, kAd1Y,
                              kEdWidth, kEdHeight,
                              MixArchive::RetrieveData("EBTN-UP.SHP"),
                              MixArchive::RetrieveData("EBTN-DN.SHP"));
  char boolbuf2[kTeamSize] = "";
  DropListClass booltype2list(kDataBooltype2, boolbuf2, sizeof(boolbuf2),
                              FontType::kEditor, TPF_NOSHADOW, kAd2X, kAd2Y,
                              kEdWidth, kEdHeight,
                              MixArchive::RetrieveData("EBTN-UP.SHP"),
                              MixArchive::RetrieveData("EBTN-DN.SHP"));

  booltype1list.Add_Item("OFF");
  booltype1list.Add_Item("ON");
  booltype2list.Add_Item("OFF");
  booltype2list.Add_Item("ON");

  booltype1list.Set_Selected_Index(Action1.data.flag ? 1 : 0);
  booltype2list.Set_Selected_Index(Action2.data.flag ? 1 : 0);

  // Musical theme lists for the actions; the list index is the ThemeType.
  char themebuf1[kDescSize] = "";
  DropListClass themetype1list(kDataTheme1, themebuf1, sizeof(themebuf1),
                               FontType::kEditor, TPF_NOSHADOW, kAd1X, kAd1Y,
                               kEdWidth, kEdHeight,
                               MixArchive::RetrieveData("EBTN-UP.SHP"),
                               MixArchive::RetrieveData("EBTN-DN.SHP"));
  char themebuf2[kDescSize] = "";
  DropListClass themetype2list(kDataTheme2, themebuf2, sizeof(themebuf2),
                               FontType::kEditor, TPF_NOSHADOW, kAd2X, kAd2Y,
                               kEdWidth, kEdHeight,
                               MixArchive::RetrieveData("EBTN-UP.SHP"),
                               MixArchive::RetrieveData("EBTN-DN.SHP"));

  for (const ThemeType theme : magic_enum::enum_values<ThemeType>()) {
    themetype1list.Add_Item(ThemeClass::Full_Name(theme));
    themetype2list.Add_Item(ThemeClass::Full_Name(theme));
  }

  if (ActionNeed(Action1.type) == NEED_THEME) {
    themetype1list.Set_Selected_Index(static_cast<int>(Action1.data.theme));
  } else {
    themetype1list.Set_Selected_Index(0);
  }
  if (ActionNeed(Action2.type) == NEED_THEME) {
    themetype2list.Set_Selected_Index(static_cast<int>(Action2.data.theme));
  } else {
    themetype2list.Set_Selected_Index(0);
  }

  // Movie lists for the actions; the list index is the VQType.
  char moviebuf1[kDescSize] = "";
  DropListClass movietype1list(kDataMovie1, moviebuf1, sizeof(moviebuf1),
                               FontType::kEditor, TPF_NOSHADOW, kAd1X, kAd1Y,
                               kEdWidth, kEdHeight,
                               MixArchive::RetrieveData("EBTN-UP.SHP"),
                               MixArchive::RetrieveData("EBTN-DN.SHP"));
  char moviebuf2[kDescSize] = "";
  DropListClass movietype2list(kDataMovie2, moviebuf2, sizeof(moviebuf2),
                               FontType::kEditor, TPF_NOSHADOW, kAd2X, kAd2Y,
                               kEdWidth, kEdHeight,
                               MixArchive::RetrieveData("EBTN-UP.SHP"),
                               MixArchive::RetrieveData("EBTN-DN.SHP"));

  for (const VQType movie : magic_enum::enum_values<VQType>()) {
    movietype1list.Add_Item(VQName.at(movie));
    movietype2list.Add_Item(VQName.at(movie));
  }

  if (ActionNeed(Action1.type) == NEED_MOVIE) {
    movietype1list.Set_Selected_Index(static_cast<int>(Action1.data.movie));
  } else {
    movietype1list.Set_Selected_Index(0);
  }
  if (ActionNeed(Action2.type) == NEED_MOVIE) {
    movietype2list.Set_Selected_Index(static_cast<int>(Action2.data.movie));
  } else {
    movietype2list.Set_Selected_Index(0);
  }

  // Sound effect lists for the actions; the list index is the VocType.
  char soundbuf1[kDescSize] = "";
  DropListClass soundtype1list(kDataSound1, soundbuf1, sizeof(soundbuf1),
                               FontType::kEditor, TPF_NOSHADOW, kAd1X, kAd1Y,
                               kEdWidth, kEdHeight,
                               MixArchive::RetrieveData("EBTN-UP.SHP"),
                               MixArchive::RetrieveData("EBTN-DN.SHP"));
  char soundbuf2[kDescSize] = "";
  DropListClass soundtype2list(kDataSound2, soundbuf2, sizeof(soundbuf2),
                               FontType::kEditor, TPF_NOSHADOW, kAd2X, kAd2Y,
                               kEdWidth, kEdHeight,
                               MixArchive::RetrieveData("EBTN-UP.SHP"),
                               MixArchive::RetrieveData("EBTN-DN.SHP"));

  for (const VocType sound : magic_enum::enum_values<VocType>()) {
    soundtype1list.Add_Item(VocName(sound));
    soundtype2list.Add_Item(VocName(sound));
  }

  if (ActionNeed(Action1.type) == NEED_SOUND) {
    soundtype1list.Set_Selected_Index(static_cast<int>(Action1.data.sound));
  } else {
    soundtype1list.Set_Selected_Index(0);
  }
  if (ActionNeed(Action2.type) == NEED_SOUND) {
    soundtype2list.Set_Selected_Index(static_cast<int>(Action2.data.sound));
  } else {
    soundtype2list.Set_Selected_Index(0);
  }

  // Speech lists for the actions; the list index is the VoxType.
  char speechbuf1[kDescSize] = "";
  DropListClass speechtype1list(kDataSpeech1, speechbuf1, sizeof(speechbuf1),
                                FontType::kEditor, TPF_NOSHADOW, kAd1X, kAd1Y,
                                kEdWidth, kEdHeight,
                                MixArchive::RetrieveData("EBTN-UP.SHP"),
                                MixArchive::RetrieveData("EBTN-DN.SHP"));
  char speechbuf2[kDescSize] = "";
  DropListClass speechtype2list(kDataSpeech2, speechbuf2, sizeof(speechbuf2),
                                FontType::kEditor, TPF_NOSHADOW, kAd2X, kAd2Y,
                                kEdWidth, kEdHeight,
                                MixArchive::RetrieveData("EBTN-UP.SHP"),
                                MixArchive::RetrieveData("EBTN-DN.SHP"));

  for (const VoxType speech : magic_enum::enum_values<VoxType>()) {
    speechtype1list.Add_Item(VoxName(speech));
    speechtype2list.Add_Item(VoxName(speech));
  }

  if (ActionNeed(Action1.type) == NEED_SPEECH) {
    speechtype1list.Set_Selected_Index(static_cast<int>(Action1.data.speech));
  } else {
    speechtype1list.Set_Selected_Index(0);
  }
  if (ActionNeed(Action2.type) == NEED_SPEECH) {
    speechtype2list.Set_Selected_Index(static_cast<int>(Action2.data.speech));
  } else {
    speechtype2list.Set_Selected_Index(0);
  }

  // Building type lists for the events; the list index is the StructType.
  char bbuf1[kDescSize] = "";
  DropListClass btype1list(kDataBtype1, bbuf1, sizeof(bbuf1), FontType::kEditor,
                           TPF_NOSHADOW, kEd1X, kEd1Y, kEdWidth, kEdHeight,
                           MixArchive::RetrieveData("EBTN-UP.SHP"),
                           MixArchive::RetrieveData("EBTN-DN.SHP"));
  char bbuf2[kDescSize] = "";
  DropListClass btype2list(kDataBtype2, bbuf2, sizeof(bbuf2), FontType::kEditor,
                           TPF_NOSHADOW, kEd2X, kEd2Y, kEdWidth, kEdHeight,
                           MixArchive::RetrieveData("EBTN-UP.SHP"),
                           MixArchive::RetrieveData("EBTN-DN.SHP"));

  for (const StructType ss : magic_enum::enum_values<StructType>()) {
    btype1list.Add_Item(
        Text_String(BuildingTypeClass::As_Reference(ss).Full_Name()));
    btype2list.Add_Item(
        Text_String(BuildingTypeClass::As_Reference(ss).Full_Name()));
  }

  if (Event_Needs(Event1.Event) == NEED_STRUCTURE) {
    btype1list.Set_Selected_Index(static_cast<int>(Event1.Data.Structure));
  } else {
    btype1list.Set_Selected_Index(0);
  }
  if (Event_Needs(Event2.Event) == NEED_STRUCTURE) {
    btype2list.Set_Selected_Index(static_cast<int>(Event2.Data.Structure));
  } else {
    btype2list.Set_Selected_Index(0);
  }

  // Infantry type lists for the events; the list index is the InfantryType.
  char ibuf1[kDescSize] = "";
  DropListClass itype1list(kDataItype1, ibuf1, sizeof(ibuf1), FontType::kEditor,
                           TPF_NOSHADOW, kEd1X, kEd1Y, kEdWidth, kEdHeight,
                           MixArchive::RetrieveData("EBTN-UP.SHP"),
                           MixArchive::RetrieveData("EBTN-DN.SHP"));
  char ibuf2[kDescSize] = "";
  DropListClass itype2list(kDataItype2, ibuf2, sizeof(ibuf2), FontType::kEditor,
                           TPF_NOSHADOW, kEd2X, kEd2Y, kEdWidth, kEdHeight,
                           MixArchive::RetrieveData("EBTN-UP.SHP"),
                           MixArchive::RetrieveData("EBTN-DN.SHP"));

  for (const InfantryType ii : magic_enum::enum_values<InfantryType>()) {
    itype1list.Add_Item(
        Text_String(InfantryTypeClass::As_Reference(ii).Full_Name()));
    itype2list.Add_Item(
        Text_String(InfantryTypeClass::As_Reference(ii).Full_Name()));
  }

  if (Event_Needs(Event1.Event) == NEED_INFANTRY) {
    itype1list.Set_Selected_Index(static_cast<int>(Event1.Data.Infantry));
  } else {
    itype1list.Set_Selected_Index(0);
  }
  if (Event_Needs(Event2.Event) == NEED_INFANTRY) {
    itype2list.Set_Selected_Index(static_cast<int>(Event2.Data.Infantry));
  } else {
    itype2list.Set_Selected_Index(0);
  }

  // Aircraft type lists for the events; the list index is the AircraftType.
  char abuf1[kDescSize] = "";
  DropListClass atype1list(kDataAtype1, abuf1, sizeof(abuf1), FontType::kEditor,
                           TPF_NOSHADOW, kEd1X, kEd1Y, kEdWidth, kEdHeight,
                           MixArchive::RetrieveData("EBTN-UP.SHP"),
                           MixArchive::RetrieveData("EBTN-DN.SHP"));
  char abuf2[kDescSize] = "";
  DropListClass atype2list(kDataAtype2, abuf2, sizeof(abuf2), FontType::kEditor,
                           TPF_NOSHADOW, kEd2X, kEd2Y, kEdWidth, kEdHeight,
                           MixArchive::RetrieveData("EBTN-UP.SHP"),
                           MixArchive::RetrieveData("EBTN-DN.SHP"));

  for (const AircraftType aa : magic_enum::enum_values<AircraftType>()) {
    atype1list.Add_Item(
        Text_String(AircraftTypeClass::As_Reference(aa).Full_Name()));
    atype2list.Add_Item(
        Text_String(AircraftTypeClass::As_Reference(aa).Full_Name()));
  }

  if (Event_Needs(Event1.Event) == NEED_AIRCRAFT) {
    atype1list.Set_Selected_Index(static_cast<int>(Event1.Data.Aircraft));
  } else {
    atype1list.Set_Selected_Index(0);
  }
  if (Event_Needs(Event2.Event) == NEED_AIRCRAFT) {
    atype2list.Set_Selected_Index(static_cast<int>(Event2.Data.Aircraft));
  } else {
    atype2list.Set_Selected_Index(0);
  }

  // Unit type lists for the events; the list index is the UnitType.
  char ubuf1[kDescSize] = "";
  DropListClass utype1list(kDataUtype1, ubuf1, sizeof(ubuf1), FontType::kEditor,
                           TPF_NOSHADOW, kEd1X, kEd1Y, kEdWidth, kEdHeight,
                           MixArchive::RetrieveData("EBTN-UP.SHP"),
                           MixArchive::RetrieveData("EBTN-DN.SHP"));
  char ubuf2[kDescSize] = "";
  DropListClass utype2list(kDataUtype2, ubuf2, sizeof(ubuf2), FontType::kEditor,
                           TPF_NOSHADOW, kEd2X, kEd2Y, kEdWidth, kEdHeight,
                           MixArchive::RetrieveData("EBTN-UP.SHP"),
                           MixArchive::RetrieveData("EBTN-DN.SHP"));

  for (const UnitType uu : magic_enum::enum_values<UnitType>()) {
    utype1list.Add_Item(
        Text_String(UnitTypeClass::As_Reference(uu).Full_Name()));
    utype2list.Add_Item(
        Text_String(UnitTypeClass::As_Reference(uu).Full_Name()));
  }

  if (Event_Needs(Event1.Event) == NEED_UNIT) {
    utype1list.Set_Selected_Index(static_cast<int>(Event1.Data.Unit));
  } else {
    utype1list.Set_Selected_Index(0);
  }
  if (Event_Needs(Event2.Event) == NEED_UNIT) {
    utype2list.Set_Selected_Index(static_cast<int>(Event2.Data.Unit));
  } else {
    utype2list.Set_Selected_Index(0);
  }

  // House lists, one per event and action; the list index is the HousesType.
  char housebuf1[kDescSize] = "";
  DropListClass htype1list(kDataHtype1, housebuf1, sizeof(housebuf1),
                           FontType::kEditor, TPF_NOSHADOW, kEd1X, kEd1Y,
                           kEdWidth, kEdHeight,
                           MixArchive::RetrieveData("EBTN-UP.SHP"),
                           MixArchive::RetrieveData("EBTN-DN.SHP"));
  char housebuf2[kDescSize] = "";
  DropListClass htype2list(kDataHtype2, housebuf2, sizeof(housebuf2),
                           FontType::kEditor, TPF_NOSHADOW, kEd2X, kEd2Y,
                           kEdWidth, kEdHeight,
                           MixArchive::RetrieveData("EBTN-UP.SHP"),
                           MixArchive::RetrieveData("EBTN-DN.SHP"));
  char housebuf3[kDescSize] = "";
  DropListClass htype3list(kDataHtype3, housebuf3, sizeof(housebuf3),
                           FontType::kEditor, TPF_NOSHADOW, kAd1X, kAd1Y,
                           kEdWidth, kEdHeight,
                           MixArchive::RetrieveData("EBTN-UP.SHP"),
                           MixArchive::RetrieveData("EBTN-DN.SHP"));
  char housebuf4[kDescSize] = "";
  DropListClass htype4list(kDataHtype4, housebuf4, sizeof(housebuf4),
                           FontType::kEditor, TPF_NOSHADOW, kAd2X, kAd2Y,
                           kEdWidth, kEdHeight,
                           MixArchive::RetrieveData("EBTN-UP.SHP"),
                           MixArchive::RetrieveData("EBTN-DN.SHP"));

  for (const HousesType hh : magic_enum::enum_values<HousesType>()) {
    htype1list.Add_Item(HouseTypeClass::As_Reference(hh).IniName);
    htype2list.Add_Item(HouseTypeClass::As_Reference(hh).IniName);
    htype3list.Add_Item(HouseTypeClass::As_Reference(hh).IniName);
    htype4list.Add_Item(HouseTypeClass::As_Reference(hh).IniName);
  }

  if (Event_Needs(Event1.Event) == NEED_HOUSE) {
    htype1list.Set_Selected_Index(static_cast<int>(Event1.Data.House));
  } else {
    htype1list.Set_Selected_Index(0);
  }
  if (Event_Needs(Event2.Event) == NEED_HOUSE) {
    htype2list.Set_Selected_Index(static_cast<int>(Event2.Data.House));
  } else {
    htype2list.Set_Selected_Index(0);
  }
  if (ActionNeed(Action1.type) == NEED_HOUSE) {
    htype3list.Set_Selected_Index(static_cast<int>(Action1.data.house));
  } else {
    htype3list.Set_Selected_Index(0);
  }
  if (ActionNeed(Action2.type) == NEED_HOUSE) {
    htype4list.Set_Selected_Index(static_cast<int>(Action2.data.house));
  } else {
    htype4list.Set_Selected_Index(0);
  }

  // Special weapon lists for the actions; the list index is the
  // SpecialWeaponType. An action that needs something else holds another
  // member of the data union, which may not be a valid weapon.
  char special1[kDescSize] = "";
  DropListClass spc1(kDataSpecial1, special1, sizeof(special1),
                     FontType::kEditor, TPF_NOSHADOW, kAd1X, kAd1Y, kEdWidth,
                     kEdHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
                     MixArchive::RetrieveData("EBTN-DN.SHP"));
  char special2[kDescSize] = "";
  DropListClass spc2(kDataSpecial2, special2, sizeof(special2),
                     FontType::kEditor, TPF_NOSHADOW, kAd2X, kAd2Y, kEdWidth,
                     kEdHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
                     MixArchive::RetrieveData("EBTN-DN.SHP"));
  for (const SpecialWeaponType spec :
       magic_enum::enum_values<SpecialWeaponType>()) {
    spc1.Add_Item(SpecialWeaponName.at(spec));
    spc2.Add_Item(SpecialWeaponName.at(spec));
  }
  if (magic_enum::enum_contains(Action1.data.special)) {
    spc1.Set_Selected_Index(static_cast<int>(Action1.data.special));
  } else {
    spc1.Set_Selected_Index(0);
  }
  if (magic_enum::enum_contains(Action2.data.special)) {
    spc2.Set_Selected_Index(static_cast<int>(Action2.data.special));
  } else {
    spc2.Set_Selected_Index(0);
  }

  // Quarry (preferred target) lists for the actions; the list index is the
  // QuarryType. They share the special weapon lists' gadget IDs, which is
  // harmless: nothing looks either up by ID, and only one of the two is linked
  // in at a time.
  char quarry1[kDescSize] = "";
  DropListClass qlist1(kDataSpecial1, quarry1, sizeof(quarry1),
                       FontType::kEditor, TPF_NOSHADOW, kAd1X, kAd1Y, kEdWidth,
                       kEdHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
                       MixArchive::RetrieveData("EBTN-DN.SHP"));
  char quarry2[kDescSize] = "";
  DropListClass qlist2(kDataSpecial2, quarry2, sizeof(quarry2),
                       FontType::kEditor, TPF_NOSHADOW, kAd2X, kAd2Y, kEdWidth,
                       kEdHeight, MixArchive::RetrieveData("EBTN-UP.SHP"),
                       MixArchive::RetrieveData("EBTN-DN.SHP"));
  for (const QuarryType q : magic_enum::enum_values<QuarryType>()) {
    qlist1.Add_Item(QuarryName.at(q));
    qlist2.Add_Item(QuarryName.at(q));
  }
  if (magic_enum::enum_contains(Action1.data.quarry)) {
    qlist1.Set_Selected_Index(static_cast<int>(Action1.data.quarry));
  } else {
    qlist1.Set_Selected_Index(0);
  }
  if (magic_enum::enum_contains(Action2.data.quarry)) {
    qlist2.Set_Selected_Index(static_cast<int>(Action2.data.quarry));
  } else {
    qlist2.Set_Selected_Index(0);
  }

  // The trigger name: four characters, which is all Description() shows.
  char namebuf[5] = "";
  EditClass name_edt(kNameEdit, namebuf, sizeof(namebuf), FontType::kEditor,
                     TPF_NOSHADOW, kDialogX + 40, kDialogY + 30, 40, 9,
                     EditClass::kAlphanumeric);
  base::SafeCopy(namebuf, IniName);  // Name

  // The house that owns the trigger. The list cannot show "no house", so an
  // unset house becomes HOUSE_GOOD, even if the dialog is then cancelled.
  char housetext[kDescSize] = "";
  DropListClass housebtn(kButtonHouse, housetext, sizeof(housetext),
                         FontType::kEditor, TPF_NOSHADOW,
                         name_edt.X + name_edt.Width + 20, name_edt.Y, 95,
                         8 * 5, MixArchive::RetrieveData("EBTN-UP.SHP"),
                         MixArchive::RetrieveData("EBTN-DN.SHP"));
  for (const HousesType house : magic_enum::enum_values<HousesType>()) {
    housebtn.Add_Item(HouseTypeClass::As_Reference(house).IniName);
  }
  if (House == HOUSE_NONE) {
    House = HOUSE_GOOD;
  }
  housebtn.Set_Selected_Index(static_cast<int>(House));

  // The persistence list; the list index is the PersistantType, so the names
  // must stay in the enum's order.
  char perstext[kDescSize] = "";
  static const char* _perstext[3] = {"Volatile", "Semi-persistent",
                                     "Persistent"};
  DropListClass persbtn(kButtonPersistance, perstext, sizeof(perstext),
                        FontType::kEditor, TPF_NOSHADOW,
                        housebtn.X + housebtn.Width + 20, housebtn.Y, 105,
                        8 * 5, MixArchive::RetrieveData("EBTN-UP.SHP"),
                        MixArchive::RetrieveData("EBTN-DN.SHP"));
  for (auto& i : _perstext) {
    persbtn.Add_Item(i);
  }
  persbtn.Set_Selected_Index(static_cast<int>(IsPersistant));

  // This button cycles through the four MultiStyleType values for the events;
  // `eventflag` is the value as a number.
  int eventflag = static_cast<int>(EventControl);
  TextButtonClass eventbtn(kButtonEvent, TXT_TRIGGER_JUST_EVENT, kEButtonFont,
                           kEButtonFlags, event1list.X, event1list.Y + 11, 100,
                           9);

  // This button toggles the second action. Linked events always have one, so
  // the button is hidden then and `actionflag` is not consulted.
  bool actionflag = ActionControl != MULTI_ONLY;
  TextButtonClass actionbtn(kButtonAction, TXT_TRIGGER_JUST_ACTION,
                            kEButtonFont, kEButtonFlags, action1list.X,
                            action1list.Y + 11, 100, 9);

  // Create the ubiquitous OK and Cancel buttons.
  TextButtonClass okbtn(kButtonOk, TXT_OK, kEButtonFont, kEButtonFlags,
                        kDialogX + 35, kDialogY + kDialogH - 30, 45, 9);
  TextButtonClass cancelbtn(kKbuttoncancel, TXT_CANCEL, kEButtonFont,
                            kEButtonFlags, kDialogX + kDialogW - 80,
                            kDialogY + kDialogH - 30, 45, 9);

  PixelView& view = TheScreen().visible_view();

  // Link in the gadgets that are always shown. The second event and action
  // lists and all the data fields are added by the redraw pass.
  commands = &okbtn;
  cancelbtn.Add_Tail(*commands);
  event1list.Add_Tail(*commands);
  action1list.Add_Tail(*commands);
  eventbtn.Add_Tail(*commands);
  actionbtn.Add_Tail(*commands);
  name_edt.Add_Tail(*commands);
  persbtn.Add_Tail(*commands);
  housebtn.Add_Tail(*commands);

  // Main Processing Loop
  bool display = true;
  bool process = true;
  while (process) {
    // Service the real-time systems while the dialog has the main loop.
    ServiceRealTime();

    // Redraw the dialog and relink its gadgets whenever a choice may have
    // changed what it shows.
    if (display) {
      game::TheMouse().Hide();
      Dialog_Box(kDialogX, kDialogY, kDialogW, kDialogH);
      Draw_Caption(view, TXT_TRIGGER_EDITOR, kDialogX, kDialogY, kDialogW);

      // Draw the captions
      Fancy_Text_Print(view, "Trigger Event:", event1list.X, event1list.Y - 7,
                       scheme, kTBlack, FontType::kEditor, TPF_NOSHADOW);
      Fancy_Text_Print(view, "Action to Perform:", action1list.X,
                       action1list.Y - 7, scheme, kTBlack, FontType::kEditor,
                       TPF_NOSHADOW);
      Fancy_Text_Print(view, "House:", housebtn.X, housebtn.Y - 7, scheme,
                       kTBlack, FontType::kEditor, TPF_NOSHADOW);
      Fancy_Text_Print(view, "Name:", name_edt.X, name_edt.Y - 7, scheme,
                       kTBlack, FontType::kEditor, TPF_NOSHADOW);
      Fancy_Text_Print(view, "Persistence:", persbtn.X, persbtn.Y - 7, scheme,
                       kTBlack, FontType::kEditor, TPF_NOSHADOW);

      // For linked events, draw a bracket from each event list to the action
      // list it sets off.
      if (eventflag == 3) {
        view.DrawLine(event1list.X - 1, event1list.Y + 3, event1list.X - 4,
                      event1list.Y + 3, kWhite);
        view.DrawLine(event1list.X - 4, event1list.Y + 3, action1list.X - 4,
                      action1list.Y + 3, kWhite);
        view.DrawLine(action1list.X - 1, action1list.Y + 3, action1list.X - 4,
                      action1list.Y + 3, kWhite);

        view.DrawLine(event2list.X - 1, event2list.Y + 3, event2list.X - 10,
                      event2list.Y + 3, kWhite);
        view.DrawLine(event2list.X - 10, event2list.Y + 3, action2list.X - 10,
                      action2list.Y + 3, kWhite);
        view.DrawLine(action2list.X - 1, action2list.Y + 3, action2list.X - 10,
                      action2list.Y + 3, kWhite);
      }

      // Show the second event list unless there is only one event.
      event2list.Remove();
      switch (eventflag) {
        case 0:
          eventbtn.Set_Text(TXT_TRIGGER_JUST_EVENT);
          break;

        case 1:
          eventbtn.Set_Text(TXT_TRIGGER_AND);
          event2list.Add(*commands);
          break;

        case 2:
          eventbtn.Set_Text(TXT_TRIGGER_OR);
          event2list.Add(*commands);
          break;

        case 3:
          eventbtn.Set_Text(TXT_TRIGGER_LINKED);
          event2list.Add(*commands);
          break;
        default:
          break;
      }

      // Show the data field the first event needs, if any.
      htype1list.Remove();
      way1data.Remove();
      event1data.Remove();
      btype1list.Remove();
      itype1list.Remove();
      atype1list.Remove();
      utype1list.Remove();
      ttype1list.Remove();
      switch (Event_Needs(*event1list.Current_Item())) {
        case NEED_HOUSE:
          htype1list.Add(*commands);
          break;

        case NEED_TEAM:
          ttype1list.Add(*commands);
          break;

        case NEED_WAYPOINT:
          way1data.Add(*commands);
          break;

        case NEED_TIME:
        case NEED_NUMBER:
          event1data.Add(*commands);
          break;

        case NEED_STRUCTURE:
          btype1list.Add(*commands);
          break;

        case NEED_INFANTRY:
          itype1list.Add(*commands);
          break;

        case NEED_AIRCRAFT:
          atype1list.Add(*commands);
          break;

        case NEED_UNIT:
          utype1list.Add(*commands);
          break;

        case NeedType::NEED_NONE:
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
      htype2list.Remove();
      way2data.Remove();
      event2data.Remove();
      btype2list.Remove();
      itype2list.Remove();
      atype2list.Remove();
      utype2list.Remove();
      ttype2list.Remove();
      if (commands->Extract_Gadget(kEventList2)) {
        switch (Event_Needs(*event2list.Current_Item())) {
          case NEED_HOUSE:
            htype2list.Add(*commands);
            break;

          case NEED_TEAM:
            ttype2list.Add(*commands);
            break;

          case NEED_WAYPOINT:
            way2data.Add(*commands);
            break;

          case NEED_TIME:
          case NEED_NUMBER:
            event2data.Add(*commands);
            break;

          case NEED_STRUCTURE:
            btype2list.Add(*commands);
            break;

          case NEED_INFANTRY:
            itype2list.Add(*commands);
            break;

          case NEED_AIRCRAFT:
            atype2list.Add(*commands);
            break;

          case NEED_UNIT:
            utype2list.Add(*commands);
            break;

          case NeedType::NEED_NONE:
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
      actionbtn.Remove();
      action2list.Remove();
      if (eventflag == 3) {
        action2list.Add(*commands);
      } else {
        actionbtn.Add(*commands);
        if (actionflag) {
          actionbtn.Set_Text(TXT_TRIGGER_AND);
          action2list.Add(*commands);
        } else {
          actionbtn.Set_Text(TXT_TRIGGER_JUST_ACTION);
        }
      }

      // Show the data field the first action needs, if any.
      qlist1.Remove();
      spc1.Remove();
      htype3list.Remove();
      booltype1list.Remove();
      trtype1list.Remove();
      way3data.Remove();
      action1data.Remove();
      ttype3list.Remove();
      themetype1list.Remove();
      soundtype1list.Remove();
      movietype1list.Remove();
      speechtype1list.Remove();
      switch (ActionNeed(*action1list.Current_Item())) {
        case NEED_MOVIE:
          movietype1list.Add(*commands);
          break;

        case NEED_SPECIAL:
          spc1.Add(*commands);
          break;

        case NEED_HOUSE:
          htype3list.Add(*commands);
          break;

        case NEED_BOOL:
          booltype1list.Add(*commands);
          break;

        case NEED_TRIGGER:
          trtype1list.Add(*commands);
          break;

        case NEED_TEAM:
          ttype3list.Add(*commands);
          break;

        case NEED_NUMBER:
          action1data.Add(*commands);
          break;

        case NEED_WAYPOINT:
          way3data.Add(*commands);
          break;

        case NEED_THEME:
          themetype1list.Add(*commands);
          break;

        case NEED_SOUND:
          soundtype1list.Add(*commands);
          break;

        case NEED_SPEECH:
          speechtype1list.Add(*commands);
          break;

        case NEED_QUARRY:
          qlist1.Add(*commands);
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
      qlist2.Remove();
      spc2.Remove();
      htype4list.Remove();
      booltype2list.Remove();
      trtype2list.Remove();
      way4data.Remove();
      action2data.Remove();
      ttype4list.Remove();
      themetype2list.Remove();
      soundtype2list.Remove();
      movietype2list.Remove();
      speechtype2list.Remove();
      if (commands->Extract_Gadget(kActionList2)) {
        switch (ActionNeed(*action2list.Current_Item())) {
          case NEED_MOVIE:
            movietype2list.Add(*commands);
            break;

          case NEED_SPECIAL:
            spc2.Add(*commands);
            break;

          case NEED_HOUSE:
            htype4list.Add(*commands);
            break;

          case NEED_BOOL:
            booltype2list.Add(*commands);
            break;

          case NEED_TRIGGER:
            trtype2list.Add(*commands);
            break;

          case NEED_TEAM:
            ttype4list.Add(*commands);
            break;

          case NEED_NUMBER:
            action2data.Add(*commands);
            break;

          case NEED_WAYPOINT:
            way4data.Add(*commands);
            break;

          case NEED_THEME:
            themetype2list.Add(*commands);
            break;

          case NEED_SOUND:
            soundtype2list.Add(*commands);
            break;

          case NEED_SPEECH:
            speechtype2list.Add(*commands);
            break;

          case NEED_QUARRY:
            qlist2.Add(*commands);
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
      spc1.Collapse();
      spc2.Collapse();
      qlist1.Collapse();
      qlist2.Collapse();
      htype1list.Collapse();
      htype2list.Collapse();
      htype3list.Collapse();
      htype4list.Collapse();
      ttype1list.Collapse();
      ttype2list.Collapse();
      ttype3list.Collapse();
      ttype4list.Collapse();
      btype1list.Collapse();
      btype2list.Collapse();
      utype1list.Collapse();
      utype2list.Collapse();
      itype1list.Collapse();
      itype2list.Collapse();
      atype1list.Collapse();
      atype2list.Collapse();
      trtype1list.Collapse();
      trtype2list.Collapse();
      action1list.Collapse();
      action2list.Collapse();
      event1list.Collapse();
      event2list.Collapse();
      housebtn.Collapse();
      persbtn.Collapse();
      booltype1list.Collapse();
      booltype2list.Collapse();
      themetype1list.Collapse();
      themetype2list.Collapse();
      soundtype1list.Collapse();
      soundtype2list.Collapse();
      movietype1list.Collapse();
      movietype2list.Collapse();
      speechtype1list.Collapse();
      speechtype2list.Collapse();
      commands->Flag_List_To_Redraw();
      game::TheMouse().Show();
      display = false;
    }

    const engine::window::KeyNumber input = commands->Input(view).key;

    switch (static_cast<int>(input)) {
      case engine::window::ButtonKey(kButtonEvent):
        eventflag = (eventflag + 1) % 4;
        display = true;
        break;

      case engine::window::ButtonKey(kButtonAction):
        actionflag = !actionflag;
        display = true;
        break;

      case engine::window::ButtonKey(kDataSpeech1):
        Speak(VoxType(speechtype1list.Current_Index()));
        display = true;
        break;

      case engine::window::ButtonKey(kDataSpeech2):
        Speak(VoxType(speechtype2list.Current_Index()));
        display = true;
        break;

      case engine::window::ButtonKey(kDataSound1):
        PlaySoundEffect(VocType(soundtype1list.Current_Index()));
        display = true;
        break;

      case engine::window::ButtonKey(kDataSound2):
        PlaySoundEffect(VocType(soundtype2list.Current_Index()));
        display = true;
        break;

      // Store the dialog's values in the trigger type. Each event and action
      // takes only the field it needs; the rest of its data is left alone.
      case KN_RETURN:
      case engine::window::ButtonKey(kButtonOk):
        House = HousesType(housebtn.Current_Index());
        IsPersistant = PersistantType(persbtn.Current_Index());
        if (std::string_view(namebuf).empty()) {
          Set_Name("____");
        } else {
          Set_Name(namebuf);
        }

        // The first event and its data.
        EventControl = MultiStyleType(eventflag);
        Event1.Event = *event1list.Current_Item();
        switch (Event_Needs(Event1.Event)) {
          case NEED_HOUSE:
            Event1.Data.House = HousesType(htype1list.Current_Index());
            break;

          case NEED_TIME:
          case NEED_NUMBER:
            Event1.Data.Value =
                base::ParseIntegerOr<int>(event1data.Get_Text(), 0);
            break;

          case NEED_STRUCTURE:
            Event1.Data.Structure = StructType(btype1list.Current_Index());
            break;

          case NEED_UNIT:
            Event1.Data.Unit = UnitType(utype1list.Current_Index());
            break;

          case NEED_INFANTRY:
            Event1.Data.Infantry = InfantryType(itype1list.Current_Index());
            break;

          case NEED_AIRCRAFT:
            Event1.Data.Aircraft = AircraftType(atype1list.Current_Index());
            break;

          case NEED_WAYPOINT:
            Event1.Data.Value = toupper(way1[0]) - 'A';
            if (way1[1] != '\0') {
              Event1.Data.Value = (Event1.Data.Value + 1) * 26;
              Event1.Data.Value += toupper(way1[1]) - 'A';
            }
            break;

          case NEED_TEAM:
            Event1.Team = TeamTypeClass::From_Name(SelectedName(ttype1list));
            break;
          case NeedType::NEED_NONE:
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
        Event2.Event = *event2list.Current_Item();
        switch (Event_Needs(Event2.Event)) {
          case NEED_HOUSE:
            Event2.Data.House = HousesType(htype2list.Current_Index());
            break;

          case NEED_TIME:
          case NEED_NUMBER:
            Event2.Data.Value =
                base::ParseIntegerOr<int>(event2data.Get_Text(), 0);
            break;

          case NEED_STRUCTURE:
            Event2.Data.Structure = StructType(btype2list.Current_Index());
            break;

          case NEED_UNIT:
            Event2.Data.Unit = UnitType(utype2list.Current_Index());
            break;

          case NEED_INFANTRY:
            Event2.Data.Infantry = InfantryType(itype2list.Current_Index());
            break;

          case NEED_AIRCRAFT:
            Event2.Data.Aircraft = AircraftType(atype2list.Current_Index());
            break;

          case NEED_WAYPOINT:
            Event2.Data.Value = toupper(way2[0]) - 'A';
            if (way2[1] != '\0') {
              Event2.Data.Value = (Event2.Data.Value + 1) * 26;
              Event2.Data.Value += toupper(way2[1]) - 'A';
            }
            break;

          case NEED_TEAM:
            Event2.Team = TeamTypeClass::As_Pointer(SelectedName(ttype2list));
            break;
          case NeedType::NEED_NONE:
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

        // The first action and its data. `actionflag` converts to MULTI_ONLY
        // or MULTI_AND.
        ActionControl = MultiStyleType(actionflag);
        Action1.type = *action1list.Current_Item();
        switch (ActionNeed(Action1.type)) {
          case NEED_SPECIAL:
            Action1.data.special = SpecialWeaponType(spc1.Current_Index());
            break;

          case NEED_HOUSE:
            Action1.data.house = HousesType(htype3list.Current_Index());
            break;

          case NEED_TRIGGER:
            Action1.trigger_type =
                TriggerTypeClass::From_Name(SelectedName(trtype1list));
            break;

          case NEED_TEAM:
            Action1.team_type =
                TeamTypeClass::From_Name(SelectedName(ttype3list));
            break;

          case NEED_NUMBER:
            Action1.data.value =
                base::ParseIntegerOr<int>(action1data.Get_Text(), 0);
            break;

          case NEED_WAYPOINT:
            Action1.data.value = toupper(way3[0]) - 'A';
            if (way3[1] != '\0') {
              Action1.data.value = (Action1.data.value + 1) * 26;
              Action1.data.value += toupper(way3[1]) - 'A';
            }
            break;

          case NEED_BOOL:
            Action1.data.flag = booltype1list.Current_Index() != 0;
            break;

          case NEED_THEME:
            Action1.data.theme = ThemeType(themetype1list.Current_Index());
            break;

          case NEED_SOUND:
            Action1.data.sound = VocType(soundtype1list.Current_Index());
            break;

          case NEED_MOVIE:
            Action1.data.movie = VQType(movietype1list.Current_Index());
            break;

          case NEED_SPEECH:
            Action1.data.speech = VoxType(speechtype1list.Current_Index());
            break;

          case NEED_QUARRY:
            Action1.data.quarry = QuarryType(qlist1.Current_Index());
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
        Action2.type = *action2list.Current_Item();
        switch (ActionNeed(Action2.type)) {
          case NEED_SPECIAL:
            Action2.data.special = SpecialWeaponType(spc2.Current_Index());
            break;

          case NEED_HOUSE:
            Action2.data.house = HousesType(htype4list.Current_Index());
            break;

          case NEED_TRIGGER:
            Action2.trigger_type =
                TriggerTypeClass::From_Name(SelectedName(trtype2list));
            break;

          case NEED_TEAM:
            Action2.team_type =
                TeamTypeClass::From_Name(SelectedName(ttype4list));
            break;

          case NEED_NUMBER:
            Action2.data.value =
                base::ParseIntegerOr<int>(action2data.Get_Text(), 0);
            break;

          case NEED_WAYPOINT:
            Action2.data.value = toupper(way4[0]) - 'A';
            if (way4[1] != '\0') {
              Action2.data.value = (Action2.data.value + 1) * 26;
              Action2.data.value += toupper(way4[1]) - 'A';
            }
            break;

          case NEED_BOOL:
            Action2.data.flag = booltype2list.Current_Index() != 0;
            break;

          case NEED_THEME:
            Action2.data.theme = ThemeType(themetype2list.Current_Index());
            break;

          case NEED_MOVIE:
            Action2.data.movie = VQType(movietype2list.Current_Index());
            break;

          case NEED_SOUND:
            Action2.data.sound = VocType(soundtype2list.Current_Index());
            break;

          case NEED_SPEECH:
            Action2.data.speech = VoxType(speechtype2list.Current_Index());
            break;

          case NEED_QUARRY:
            Action2.data.quarry = QuarryType(qlist2.Current_Index());
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
        return true;

      case KN_ESC:
      case engine::window::ButtonKey(kKbuttoncancel):
        process = false;
        [[fallthrough]];

      // Any gadget may have changed what the dialog should show, so redraw
      // whenever the input came from one: those carry the button bit.
      default:
        if (input & KN_BUTTON) {
          display = true;
        }
        break;
    }
  }
  return false;
}

const char* TriggerTypeClass::Description() const {
  if constexpr (config::kCheatKeysEnabled || config::kScenarioEditorEnabled) {
    static char _buffer[128];

    // Marks for how the second event and action are used: '.' for unused.
    char special = 0;
    switch (EventControl) {
      case MULTI_AND:
        special = '&';
        break;

      case MULTI_OR:
        special = '|';
        break;

      case MULTI_LINKED:
        special = '=';
        break;

      case MultiStyleType::MULTI_ONLY:
      default:
        special = '.';
        break;
    }

    char special2 = '.';
    if (ActionControl == MULTI_AND) {
      special2 = '&';
    }

    // The first event's value, when it has one worth showing.
    char tbuf[32];
    const char* added = "";
    switch (Event_Needs(Event1.Event)) {
      case NEED_NUMBER:
        absl::SNPrintF(tbuf, sizeof(tbuf), "%d",
                       static_cast<int>(Event1.Data.Value));
        added = tbuf;
        break;

      case NEED_UNIT:
        added = Text_String(
            UnitTypeClass::As_Reference(Event1.Data.Unit).Full_Name());
        break;

      case NEED_AIRCRAFT:
        added = Text_String(
            AircraftTypeClass::As_Reference(Event1.Data.Aircraft).Full_Name());
        break;

      case NEED_STRUCTURE:
        added = Text_String(
            BuildingTypeClass::As_Reference(Event1.Data.Structure).Full_Name());
        break;

      case NEED_INFANTRY:
        added = Text_String(
            InfantryTypeClass::As_Reference(Event1.Data.Infantry).Full_Name());
        break;

      case NEED_WAYPOINT:
        if (Event1.Data.Value < 26) {
          absl::SNPrintF(tbuf, sizeof(tbuf), "'%c'",
                         static_cast<int>(Event1.Data.Value) + 'A');
        } else {
          absl::SNPrintF(tbuf, sizeof(tbuf), "'%c%c'",
                         (Event1.Data.Value / 26) + 'A' - 1,
                         (Event1.Data.Value % 26) + 'A');
        }
        added = tbuf;
        break;

      case NeedType::NEED_NONE:
      case NeedType::NEED_THEME:
      case NeedType::NEED_MOVIE:
      case NeedType::NEED_SOUND:
      case NeedType::NEED_SPEECH:
      case NeedType::NEED_TRIGGER:
      case NeedType::NEED_TEAM:
      case NeedType::NEED_HOUSE:
      case NeedType::NEED_TIME:
      case NeedType::NEED_QUARRY:
      case NeedType::NEED_FORMATION:
      case NeedType::NEED_BOOL:
      case NeedType::NEED_SPECIAL:
      case NeedType::NEED_MISSION:
      case NeedType::NEED_HEX_NUMBER:
      default:
        break;
    }

    // Persistence mark: V(olatile), S(emi-persistent) or P(ersistent).
    char pers = 'V';
    if (IsPersistant == SEMIPERSISTANT) {
      pers = 'S';
    }
    if (IsPersistant == PERSISTANT) {
      pers = 'P';
    }

    absl::SNPrintF(_buffer, sizeof(_buffer), "%4.4s\t %s %c%c%c  %s%s", IniName,
                   HouseTypeClass::As_Reference(House).Suffix, pers, special,
                   special2, Name_From_Event(Event1.Event), added);
    return _buffer;
  }
  return "";
}

AttachType TriggerTypeClass::Attaches_To() const {
  AttachType attach = ra::Attaches_To(Event1.Event);

  if (EventControl != MULTI_ONLY) {
    attach = attach | ra::Attaches_To(Event2.Event);
  }
  return attach;
}

void TriggerTypeClass::Read_INI(engine::file::IniFile& ini) {
  for (const auto& [entry, value] : ini.Entries(INI_Name())) {
    auto* trigger = base::NewPooled<TriggerTypeClass>();
    trigger->Fill_In(entry, value);
  }

  if (TheWorld().new_ini_format() < 2) {
    // Before format 2 an action named its trigger type instead of giving its
    // position, and the name may belong to an entry read after it. Resolve
    // the names now that every trigger type exists.
    for (int trig_index = 0;
         trig_index < TheObjectHeaps().trigger_type().Count(); trig_index++) {
      TriggerTypeClass* indexed_trigger =
          TheObjectHeaps().trigger_type().ActiveAt(trig_index);

      for (TriggerAction* action :
           {&indexed_trigger->Action1, &indexed_trigger->Action2}) {
        if (!action->pending_trigger_name.empty()) {
          action->trigger_type = From_Name(action->pending_trigger_name);
          action->pending_trigger_name.clear();
        }
      }
    }
  }
}

void TriggerTypeClass::Fill_In(std::string_view name, std::string_view entry) {
  DCHECK_HEAP_SLOT(TheObjectHeaps().trigger_type(), this);

  Set_Name(name);

  base::Tokenizer tokens(entry);
  IsPersistant =
      static_cast<PersistantType>(base::ParseIntegerOr<int>(tokens.Next(), 0));
  House = static_cast<HousesType>(base::ParseIntegerOr<int>(tokens.Next(), 0));
  EventControl =
      static_cast<MultiStyleType>(base::ParseIntegerOr<int>(tokens.Next(), 0));
  ActionControl =
      static_cast<MultiStyleType>(base::ParseIntegerOr<int>(tokens.Next(), 0));

  Event1.Read_INI(tokens);
  Event2.Read_INI(tokens);
  Action1.ReadIni(tokens);
  Action2.ReadIni(tokens);
}

void TriggerTypeClass::Write_INI(engine::file::IniFile& ini) {
  // [Triggers] is where older formats kept them; a file with both would
  // describe the triggers twice.
  ini.Clear("Triggers");
  ini.Clear(INI_Name());

  for (int index = 0; index < TheObjectHeaps().trigger_type().Count();
       index++) {
    std::string buf;
    TriggerTypeClass* trigger = TheObjectHeaps().trigger_type().ActiveAt(index);

    trigger->Build_INI_Entry(buf);
    ini.PutString(INI_Name(), trigger->IniName, buf);
  }
}

// LLVM 23 treats assignment as invalidating the string reference itself. No
// pointer, reference or iterator into its old character storage is retained.
// NOLINTNEXTLINE(clang-diagnostic-lifetime-safety-invalidation)
void TriggerTypeClass::Build_INI_Entry(std::string& buffer) const {
  // The enums are stored as their numbers, which Fill_In() casts back.
  buffer =
      std::format("{},{},{},{},", std::to_underlying(IsPersistant),
                  std::to_underlying(House), std::to_underlying(EventControl),
                  std::to_underlying(ActionControl));

  Event1.Build_INI_Entry(buffer);
  buffer += ',';

  Event2.Build_INI_Entry(buffer);
  buffer += ',';

  buffer += Action1.ToIniEntry();
  buffer += ',';

  buffer += Action2.ToIniEntry();
}

void TriggerTypeClass::Draw_It(PixelView& view, int /*unused*/, int x, int y,
                               int width, int height, bool selected,
                               FontType font, TextPrintType flags) const {
  if (config::kCheatKeysEnabled || config::kScenarioEditorEnabled) {
    RemapControlType* scheme = Get_Color_Scheme();
    // Tab stops in pixels: Description() puts a tab after the name.
    static const int _tabs[] = {13, 40};
    if (font == FontType::k6PointGradient || font == FontType::kEditor) {
      if (selected) {
        flags = flags | TPF_BRIGHT_COLOR;
        view.FillRect(x, y, x + width - 1, y + height - 1, scheme->Shadow);
      } else {
        if (!base::Any(flags & TPF_USE_GRAD_PAL)) {
          flags = flags | TPF_MEDIUM_COLOR;
        }
      }

      Conquer_Clip_Text_Print(view, Description(), x, y, scheme, kTBlack, font,
                              flags, width, _tabs);
    } else {
      Conquer_Clip_Text_Print(
          view, Description(), x, y,
          (selected ? &ThePalettes().color_remaps().at(PCOLOR_DIALOG_BLUE)
                    : &ThePalettes().color_remaps().at(PCOLOR_GREY)),
          kTBlack, font, flags, width, _tabs);
    }
  }
}

void TriggerTypeClass::Init() { TheObjectHeaps().trigger_type().FreeAll(); }

TriggerTypeClass* TriggerTypeClass::From_Name(const std::string_view name) {
  if (!name.empty()) {
    for (int index = 0; index < TheObjectHeaps().trigger_type().Count();
         index++) {
      if (absl::EqualsIgnoreCase(
              TheObjectHeaps().trigger_type().ActiveAt(index)->Name(), name)) {
        return TheObjectHeaps().trigger_type().ActiveAt(index);
      }
    }
  }
  return nullptr;
}

}  // namespace ra
