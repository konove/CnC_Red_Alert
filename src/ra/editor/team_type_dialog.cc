/*
**	Command & Conquer Red Alert(mission)
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

// The scenario editor's team type dialog, TeamType::Edit().

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "absl/strings/str_format.h"
#include "base/array.h"
#include "base/enum_array.h"
#include "base/strings/number_parse.h"
#include "base/strings/safe_string.h"
#include "engine/file/mix_archive.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/wwstd.h"
#include "engine/stream/readline.h"
#include "engine/window/hardware_cursor.h"
#include "engine/window/keyboard.h"
#include "game/core/object_heap.h"
#include "game/ui/control.h"
#include "game/ui/gadget.h"
#include "game/ui/input.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/ai/scenario.h"
#include "ra/ai/teamtype.h"
#include "ra/ai/trigger_type.h"
#include "ra/ai/waypoint_name.h"
#include "ra/assets/screen.h"
#include "ra/core/const.h"
#include "ra/core/defines.h"
#include "ra/core/object_heaps.h"
#include "ra/core/target.h"
#include "ra/data/text_ids.h"
#include "ra/debug/debug.h"
#include "ra/editor/mapedit.h"
#include "ra/map/world.h"
#include "ra/objects/house.h"
#include "ra/objects/mission.h"
#include "ra/objects/type.h"
#include "ra/startup/conquer.h"
#include "ra/ui/checkbox.h"
#include "ra/ui/color_scheme.h"
#include "ra/ui/dialog.h"
#include "ra/ui/drop.h"
#include "ra/ui/edit.h"
#include "ra/ui/textbtn.h"
#include "ra/ui/tlist.h"

namespace ra {

using enum engine::window::KeyNumber;

namespace {

// Returns the value of the hex digits in `text`, for the editor's cell-number
// field. A character that is not a hex digit counts as 0.
int ParseHexDigits(const std::string_view text) {
  int value = 0;
  for (const char digit : text) {
    value *= 16;
    if (digit >= '0' && digit <= '9') {
      value += digit - '0';
    } else if (digit >= 'a' && digit <= 'f') {
      value += 10 + (digit - 'a');
    } else if (digit >= 'A' && digit <= 'F') {
      value += 10 + (digit - 'A');
    }
  }
  return value;
}

}  // namespace

bool TeamType::Edit() {
  // Dialog and button dimensions, in pixels of the editor's 640x400 screen.
  constexpr int kDialogW = 400;
  constexpr int kDialogH = 250;
  constexpr int kDialogX = 0;
  constexpr int kDialogY = 0;
  constexpr int kNameX = kDialogX + 35;
  constexpr int kNameY = kDialogY + 27;
  constexpr int kEditWidth = 40;
  constexpr int kCheckX = kDialogX + 35;  // Start of check box attribute list.
  constexpr int kCheckY = kNameY + 25;
  constexpr int kCheckSpacingY =
      9;  // Vertical spacing between check box lines.
  constexpr int kCheckTextOffsetX =
      8;  // Horizontal spacing for check box description text.
  constexpr int kFieldSpacingX =
      9;  // Horizontal spacing between data entry fields.
  constexpr int kCancelW = 50;
  constexpr int kCancelH = 9;
  constexpr int kCancelX = kDialogX + kDialogW - (kCancelW + 35);
  constexpr int kCancelY = kDialogY + kDialogH - (kCancelH + 20);
  constexpr int kOkW = 50;
  constexpr int kOkH = 9;
  constexpr int kOkX = kDialogX + kDialogW - ((kOkW + 18) * 2);
  constexpr int kOkY = kCancelY;

  // Control IDs; Input() returns the one that was used.
  constexpr int kButtonName = 100;
  constexpr int kButtonRecruit = 101;
  constexpr int kButtonMaxnum = 102;
  constexpr int kButtonInitnum = 103;
  constexpr int kButtonHouse = 104;
  constexpr int kButtonRoundabout = 105;
  constexpr int kButtonSuicide = 107;
  constexpr int kButtonAuto = 108;
  constexpr int kButtonPrebuilt = 109;
  constexpr int kButtonReinforce = 110;
  constexpr int kButtonMission1 = 111;
  constexpr int kButtonMission2 = 112;
  constexpr int kButtonAdd = 113;
  constexpr int kButtonInsert = 114;
  constexpr int kButtonDelete = 115;
  constexpr int kButtonReplace = 116;
  constexpr int kButtonArg = 117;
  constexpr int kButtonFormation = 118;
  constexpr int kButtonMembers = 119;
  constexpr int kButtonMission = 120;
  constexpr int kButtonTrigger = 121;
  constexpr int kButtonOrigin = 122;
  constexpr int kButtonOk = 123;
  constexpr int kButtonCancel = 124;
  constexpr int kButtonQuarry = 125;

  const auto up_shape = MixArchive::RetrieveData("EBTN-UP.SHP");
  const auto down_shape = MixArchive::RetrieveData("EBTN-DN.SHP");

  // Head of the dialog's control list.
  game::ControlClass* commands = nullptr;
  RemapControlType* scheme = Get_Color_Scheme();

  // Team name edit field.
  char name_text[10];
  EditClass name_edit(kButtonName, name_text, sizeof(name_text),
                      FontType::kEditor, TPF_NOSHADOW, kNameX, kNameY,
                      kEditWidth, 9, EditClass::kAlphanumeric);
  base::SafeCopy(name_text, IniName);
  commands = &name_edit;

  // House ownership of this team.
  char house_text[25] = "";
  DropListClass house_drop(kButtonHouse, house_text, sizeof(house_text),
                           FontType::kEditor, TPF_NOSHADOW,
                           name_edit.X + name_edit.Width + kFieldSpacingX,
                           name_edit.Y, 55, 8 * 5, up_shape, down_shape);
  for (const HousesType house : magic_enum::enum_values<HousesType>()) {
    house_drop.Add_Item(HouseTypeClass::As_Reference(house).IniName);
  }
  if (House == HOUSE_NONE) {
    House = HOUSE_GOOD;
  }
  house_drop.Set_Selected_Index(static_cast<int>(House));
  house_drop.Add(*commands);

  // Recruit priority for this team.
  char priority_text[4];
  EditClass priority_edit(kButtonRecruit, priority_text, sizeof(priority_text),
                          FontType::kEditor, TPF_NOSHADOW,
                          house_drop.X + house_drop.Width + 5 + kFieldSpacingX,
                          house_drop.Y, kEditWidth, 9, EditClass::kNumeric);
  absl::SNPrintF(priority_text, sizeof(priority_text), "%d", RecruitPriority);
  priority_edit.Add(*commands);

  // Maximum allowed for this team type.
  char max_text[4];
  EditClass max_edit(kButtonMaxnum, max_text, sizeof(max_text),
                     FontType::kEditor, TPF_NOSHADOW,
                     priority_edit.X + priority_edit.Width + kFieldSpacingX,
                     priority_edit.Y, kEditWidth, 9, EditClass::kNumeric);
  absl::SNPrintF(max_text, sizeof(max_text), "%d", MaxAllowed);
  max_edit.Add(*commands);

  // Initial number for this team type.
  char initial_text[4];
  EditClass initial_edit(kButtonInitnum, initial_text, sizeof(initial_text),
                         FontType::kEditor, TPF_NOSHADOW,
                         max_edit.X + max_edit.Width + kFieldSpacingX,
                         max_edit.Y, kEditWidth, 9, EditClass::kNumeric);
  absl::SNPrintF(initial_text, sizeof(initial_text), "%d", InitNum);
  initial_edit.Add(*commands);

  // Waypoint to create or reinforce this team at, as the editor's letters;
  // empty for none.
  char origin_text[4];
  EditClass origin_edit(kButtonOrigin, origin_text, sizeof(origin_text),
                        FontType::kEditor, TPF_NOSHADOW,
                        initial_edit.X + initial_edit.Width + kFieldSpacingX,
                        initial_edit.Y, 20, 9, EditClass::kAlpha);
  base::SafeCopy(origin_text, Origin != -1 ? WaypointLetters(Origin) : "");
  origin_edit.Add(*commands);

  // Button that opens the members dialog.
  TextButtonClass members_button(kButtonMembers, "Members", kEButtonFont,
                                 kEButtonFlags, name_edit.X, name_edit.Y + 12,
                                 50);
  members_button.Add(*commands);

  // Trigger to assign to each member of this team (when object joins team).
  // Entry 0 is "<NONE>", so a trigger type's entry is its heap position + 1.
  char trigger_text[25] = "";
  DropListClass trigger_drop(kButtonTrigger, trigger_text, sizeof(trigger_text),
                             FontType::kEditor, TPF_NOSHADOW,
                             kDialogX + kDialogW - 95, members_button.Y, 60,
                             8 * 5, up_shape, down_shape);
  trigger_drop.Add_Item("<NONE>");
  for (int index = 0; index < TheObjectHeaps().trigger_type().Count();
       index++) {
    trigger_drop.Add_Item(
        TheObjectHeaps().trigger_type().ActiveAt(index)->IniName);
  }
  if (Trigger != nullptr) {
    trigger_drop.Set_Selected_Index(Trigger->Name());
  } else {
    trigger_drop.Set_Selected_Index(0);
  }
  trigger_drop.Add(*commands);

  const auto set_check = [](CheckBoxClass& box, const bool on) {
    if (on) {
      box.Turn_On();
    } else {
      box.Turn_Off();
    }
  };

  // Roundabout travel logic attribute for this team.
  CheckBoxClass roundabout_check(kButtonRoundabout, kCheckX, kCheckY);
  set_check(roundabout_check, IsRoundAbout);
  roundabout_check.Add(*commands);

  // Suicide travel to target attribute.
  CheckBoxClass suicide_check(kButtonSuicide, kCheckX,
                              roundabout_check.Y + kCheckSpacingY);
  set_check(suicide_check, IsSuicide);
  suicide_check.Add(*commands);

  // Autocreate attribute for this team.
  CheckBoxClass autocreate_check(kButtonAuto, kCheckX,
                                 suicide_check.Y + kCheckSpacingY);
  set_check(autocreate_check, IsAutocreate);
  autocreate_check.Add(*commands);

  // Prebuild team members attribute for this team.
  CheckBoxClass prebuilt_check(kButtonPrebuilt, kCheckX,
                               autocreate_check.Y + kCheckSpacingY);
  set_check(prebuilt_check, IsPrebuilt);
  prebuilt_check.Add(*commands);

  // Reinforce this team in progress attribute.
  CheckBoxClass reinforce_check(kButtonReinforce, kCheckX,
                                prebuilt_check.Y + kCheckSpacingY);
  set_check(reinforce_check, IsReinforcable);
  reinforce_check.Add(*commands);

  // Drop list of the possible team missions, in TeamMissionType order so an
  // entry's index is its mission. It and the argument control beside it hold
  // the "work mission" that Append, Insert and Replace put into the script.
  char mission_text[45];
  DropListClass mission_drop(
      kButtonMission1, mission_text, sizeof(mission_text), FontType::kEditor,
      TPF_NOSHADOW, reinforce_check.X, reinforce_check.Y + 15, 170, 8 * 8,
      up_shape, down_shape);
  for (const TeamMissionType mission :
       magic_enum::enum_values<TeamMissionType>()) {
    mission_drop.Add_Item(TeamType::MissionName(mission));
  }
  mission_drop.Set_Selected_Index(0);
  mission_drop.Add_Tail(*commands);

  // The mission's argument. Four controls share the spot beside the mission
  // list and the redraw links in the one the selected mission needs: a text
  // field for a number or waypoint, or a list of quarries, formations or unit
  // missions.
  char argument_text[6] = {0};
  EditClass argument_edit(kButtonArg, argument_text, sizeof(argument_text),
                          FontType::kEditor, TPF_NOSHADOW,
                          mission_drop.X + mission_drop.Width + 15,
                          mission_drop.Y, 60, -1, EditClass::kAlphanumeric);

  char quarry_text[55];
  DropListClass quarry_drop(kButtonQuarry, quarry_text, sizeof(quarry_text),
                            FontType::kEditor, TPF_NOSHADOW,
                            mission_drop.X + mission_drop.Width + 15,
                            mission_drop.Y, 100, 5 * 8, up_shape, down_shape);
  for (const QuarryType q : magic_enum::enum_values<QuarryType>()) {
    quarry_drop.Add_Item(QuarryName.at(q));
  }
  quarry_drop.Set_Selected_Index(0);
  quarry_drop.Add_Tail(*commands);

  char formation_text[55];
  DropListClass formation_drop(
      kButtonFormation, formation_text, sizeof(formation_text),
      FontType::kEditor, TPF_NOSHADOW, mission_drop.X + mission_drop.Width + 15,
      mission_drop.Y, 100, 5 * 8, up_shape, down_shape);
  for (const FormationType f : magic_enum::enum_values<FormationType>()) {
    formation_drop.Add_Item(FormationName.at(f));
  }
  formation_drop.Set_Selected_Index(0);
  formation_drop.Add_Tail(*commands);

  char unit_mission_text[55];
  DropListClass unit_mission_drop(
      kButtonMission, unit_mission_text, sizeof(unit_mission_text),
      FontType::kEditor, TPF_NOSHADOW, mission_drop.X + mission_drop.Width + 15,
      mission_drop.Y, 100, 5 * 8, up_shape, down_shape);
  for (const MissionType m : magic_enum::enum_values<MissionType>()) {
    unit_mission_drop.Add_Item(MissionClass::Mission_Name(m));
  }
  unit_mission_drop.Set_Selected_Index(0);
  unit_mission_drop.Add_Tail(*commands);

  // Owns the missions script_list shows: copies of the team type's own, and
  // the ones the dialog adds.
  std::vector<std::unique_ptr<TeamMission>> missions;
  TListClass<TeamMission*> script_list(
      kButtonMission2, mission_drop.X + 60, mission_drop.Y + 22, 240, 8 * 7,
      FontType::kEditor, TPF_NOSHADOW, up_shape, down_shape);
  for (int index = 0; index < MissionCount; index++) {
    script_list.Add_Item(missions
                             .emplace_back(std::make_unique<TeamMission>(
                                 base::At(MissionList, index)))
                             .get());
  }
  script_list.Add_Tail(*commands);

  // Mission editing command buttons.
  TextButtonClass append_button(kButtonAdd, "Append", kEButtonFont,
                                kEButtonFlags, kNameX,
                                mission_drop.Y + mission_drop.Height + 1, 50);
  append_button.Add(*commands);

  TextButtonClass insert_button(kButtonInsert, "Insert", kEButtonFont,
                                kEButtonFlags, append_button.X,
                                append_button.Y + 10, 50);
  insert_button.Add(*commands);

  TextButtonClass delete_button(kButtonDelete, "Delete", kEButtonFont,
                                kEButtonFlags, insert_button.X,
                                insert_button.Y + 10, 50);
  delete_button.Add(*commands);

  TextButtonClass replace_button(kButtonReplace, "Replace", kEButtonFont,
                                 kEButtonFlags, delete_button.X,
                                 delete_button.Y + 10, 50);
  replace_button.Add(*commands);

  TextButtonClass ok_button(kButtonOk, TXT_OK, kEButtonFont, kEButtonFlags,
                            kOkX, kOkY, kOkW, kOkH);
  TextButtonClass cancel_button(kButtonCancel, TXT_CANCEL, kEButtonFont,
                                kEButtonFlags, kCancelX, kCancelY, kCancelW,
                                kCancelH);

  // Returns the work mission: the mission selected in mission_drop with the
  // argument its control holds.
  const auto work_mission = [&] {
    TeamMission mission{};
    mission.Mission = TeamMissionType(mission_drop.Current_Index());
    mission.Data.Value = 0;
    switch (TeamMissionNeeds(mission.Mission)) {
      case NEED_MISSION:
        mission.Data.Mission = MissionType(unit_mission_drop.Current_Index());
        break;

      case NEED_FORMATION:
        mission.Data.Formation = FormationType(formation_drop.Current_Index());
        break;

      case NEED_QUARRY:
        mission.Data.Quarry = QuarryType(quarry_drop.Current_Index());
        break;

      case NEED_NUMBER:
        mission.Data.Value =
            base::ParseIntegerOr<int>(argument_edit.Get_Text(), 0);
        break;

      case NEED_HEX_NUMBER:
        mission.Data.Value = ParseHexDigits(argument_edit.Get_Text());
        break;

      // A waypoint the scenario does not have becomes waypoint A.
      case NEED_WAYPOINT:
        mission.Data.Value = WaypointFromLetters(argument_edit.Get_Text());
        if (mission.Data.Value < 0 ||
            mission.Data.Value >= ScenarioClass::kHomeWaypoint) {
          mission.Data.Value = 0;
        }
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
      case NeedType::NEED_TRIGGER:
      case NeedType::NEED_TEAM:
      case NeedType::NEED_HOUSE:
      case NeedType::NEED_TIME:
      case NeedType::NEED_BOOL:
      case NeedType::NEED_SPECIAL:
      default:
        break;
    }
    return mission;
  };

  PixelView& view = TheScreen().visible_view();

  const auto caption = [&](const char* const text, const int x, const int y) {
    Fancy_Text_Print(view, text, x, y, scheme, kTBlack, FontType::kEditor,
                     TPF_NOSHADOW);
  };

  ok_button.Add(*commands);
  cancel_button.Add_Tail(*commands);

  // Main processing loop. The last* values are the script list's selection,
  // its length and the control used on the previous event (-1 before any):
  // clicking the same script line twice in a row loads it into the work
  // mission.
  int last_script_index = -1;
  int last_script_count = -1;
  int last_control = -1;
  bool redraw = true;
  while (true) {
    // Keep the sound, timers and network serviced while the dialog is up.
    ServiceRealTime();

    // Refresh display if needed
    if (redraw) {
      // Link in only the argument control the work mission needs.
      argument_edit.Remove();
      quarry_drop.Remove();
      formation_drop.Remove();
      unit_mission_drop.Remove();
      switch (TeamMissionNeeds(TeamMissionType(mission_drop.Current_Index()))) {
        case NeedType::NEED_NONE:
        case NeedType::NEED_THEME:
        case NeedType::NEED_MOVIE:
        case NeedType::NEED_SOUND:
        case NeedType::NEED_SPEECH:
        case NeedType::NEED_INFANTRY:
        case NeedType::NEED_UNIT:
        case NeedType::NEED_AIRCRAFT:
        case NeedType::NEED_STRUCTURE:
        case NeedType::NEED_TRIGGER:
        case NeedType::NEED_TEAM:
        case NeedType::NEED_HOUSE:
        case NeedType::NEED_TIME:
        case NeedType::NEED_BOOL:
        case NeedType::NEED_SPECIAL:
        default:
          break;

        case NEED_MISSION:
          unit_mission_drop.Add(*commands);
          break;

        case NEED_FORMATION:
          formation_drop.Add(*commands);
          break;

        case NEED_WAYPOINT:
        case NEED_NUMBER:
        case NEED_HEX_NUMBER:
          argument_edit.Add(*commands);
          break;

        case NEED_QUARRY:
          quarry_drop.Add(*commands);
          break;
      }

      // Display the dialog box
      game::TheMouse().Hide();
      Dialog_Box(kDialogX, kDialogY, kDialogW, kDialogH);

      Draw_Caption(view, TXT_TEAM_EDIT, kDialogX, kDialogY, kDialogW);

      // Draw the captions
      caption("Name:", name_edit.X, name_edit.Y - 7);
      caption("House:", house_drop.X, house_drop.Y - 7);
      caption("Pri:", priority_edit.X, priority_edit.Y - 7);
      caption("Max:", max_edit.X, max_edit.Y - 7);
      caption("Num:", initial_edit.X, initial_edit.Y - 7);
      caption("Loc:", origin_edit.X, origin_edit.Y - 7);
      caption("#  Team Mission", script_list.X, script_list.Y - 7);
      Fancy_Text_Print(view, "Trigger:", trigger_drop.X - 4, trigger_drop.Y + 1,
                       scheme, kTBlack, FontType::kEditor,
                       TPF_RIGHT | TPF_NOSHADOW);

      caption(MemberDescription().c_str(),
              members_button.X + members_button.Width + 3,
              members_button.Y + 1);

      caption("Use safest, possibly longer, route to target?",
              roundabout_check.X + kCheckTextOffsetX, roundabout_check.Y);
      caption("Charge toward target ignoring distractions?",
              suicide_check.X + kCheckTextOffsetX, suicide_check.Y);
      caption("Only 'Autocreate A.I.' uses this team type?",
              autocreate_check.X + kCheckTextOffsetX, autocreate_check.Y);
      caption("Prebuild team members before team is created?",
              prebuilt_check.X + kCheckTextOffsetX, prebuilt_check.Y);
      caption("Automatically reinforce team whenever possible?",
              reinforce_check.X + kCheckTextOffsetX, reinforce_check.Y);

      // Redraw the buttons
      commands->Draw_All(view);
      game::TheMouse().Show();

      redraw = false;
    }

    // Get user input
    const engine::window::KeyNumber input = commands->Input(view).key;

    // Process input
    switch (static_cast<int>(input)) {
      // A click on the script. The second click on the same line, with no
      // other control used in between, copies that line into the work mission
      // and its argument control as an aid to editing.
      case engine::window::ButtonKey(kButtonMission2):
        if (script_list.Count() && last_script_count == script_list.Count() &&
            last_control == kButtonMission2 &&
            last_script_index == script_list.Current_Index()) {
          const TeamMission& line = *script_list.Current_Item();
          mission_drop.Set_Selected_Index(static_cast<int>(line.Mission));

          switch (TeamMissionNeeds(line.Mission)) {
            case NEED_MISSION:
              unit_mission_drop.Set_Selected_Index(
                  static_cast<int>(line.Data.Mission));
              break;

            case NEED_FORMATION:
              formation_drop.Set_Selected_Index(
                  static_cast<int>(line.Data.Formation));
              break;

            case NEED_NUMBER:
              absl::SNPrintF(argument_text, sizeof(argument_text), "%d",
                             line.Data.Value);
              break;

            case NEED_HEX_NUMBER:
              absl::SNPrintF(argument_text, sizeof(argument_text), "%x",
                             static_cast<unsigned int>(line.Data.Value));
              break;

            case NEED_QUARRY:
              quarry_drop.Set_Selected_Index(
                  static_cast<int>(line.Data.Quarry));
              break;

            case NEED_WAYPOINT:
              base::SafeCopy(argument_text, WaypointLetters(line.Data.Value));
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
            case NeedType::NEED_TRIGGER:
            case NeedType::NEED_TEAM:
            case NeedType::NEED_HOUSE:
            case NeedType::NEED_TIME:
            case NeedType::NEED_BOOL:
            case NeedType::NEED_SPECIAL:
            default:
              break;
          }
        }
        last_script_index = script_list.Current_Index();
        last_script_count = script_list.Count();
        redraw = true;
        break;

      // Insert the work mission at the selected line of the script; the
      // lines from there on move down.
      case engine::window::ButtonKey(kButtonInsert):
        if (script_list.Count() < kMaxMissions) {
          script_list.Insert_Item(
              missions
                  .emplace_back(std::make_unique<TeamMission>(work_mission()))
                  .get());
        }
        redraw = true;
        break;

      // Append the work mission to the script.
      case engine::window::ButtonKey(kButtonAdd):
        if (script_list.Count() < kMaxMissions) {
          script_list.Add_Item(
              missions
                  .emplace_back(std::make_unique<TeamMission>(work_mission()))
                  .get());
          redraw = true;
        }
        break;

      // Overwrite the selected line of the script with the work mission.
      case engine::window::ButtonKey(kButtonReplace):
        if (script_list.Count()) {
          *script_list.Current_Item() = work_mission();
        }
        redraw = true;
        break;

      // Delete the currently selected mission.
      case engine::window::ButtonKey(kButtonDelete):
        if (script_list.Count()) {
          const TeamMission* mission = script_list.Current_Item();
          script_list.Remove_Index(script_list.Current_Index());
          std::erase_if(missions, [mission](const auto& owned) {
            return owned.get() == mission;
          });
        }
        redraw = true;
        break;

      // Invoke the members dialog
      case engine::window::ButtonKey(kButtonMembers):

        // The button stays pressed otherwise: the other dialog takes the
        // release.
        members_button.Turn_Off();

        TheMap().Team_Members(HousesType(house_drop.Current_Index()));

        redraw = true;
        break;

      // OK: copy the dialog's values into the team type. A blank name becomes
      // "----" so the team type can still be listed and written to the INI.
      case engine::window::ButtonKey(kButtonOk):
        strtrim(name_edit.Get_Text_Buffer());
        base::SafeCopy(IniName, std::string_view(name_edit.Get_Text()).empty()
                                    ? "----"
                                    : name_edit.Get_Text());

        IsRoundAbout = roundabout_check.IsOn;
        IsSuicide = suicide_check.IsOn;
        IsAutocreate = autocreate_check.IsOn;
        IsPrebuilt = prebuilt_check.IsOn;
        IsReinforcable = reinforce_check.IsOn;

        RecruitPriority =
            base::ParseIntegerOr<int>(priority_edit.Get_Text(), 0);
        // The fields take three digits; the team type stores a byte.
        InitNum = static_cast<unsigned char>(std::clamp(
            base::ParseIntegerOr<int>(initial_edit.Get_Text(), 0), 0, 255));
        MaxAllowed = static_cast<unsigned char>(std::clamp(
            base::ParseIntegerOr<int>(max_edit.Get_Text(), 0), 0, 255));
        House = HousesType(house_drop.Current_Index());
        Trigger = nullptr;
        if (trigger_drop.Current_Index() > 0) {
          Trigger = TheObjectHeaps().trigger_type().ActiveAt(
              trigger_drop.Current_Index() - 1);
        }

        MissionCount = script_list.Count();
        for (int index = 0; index < MissionCount; index++) {
          base::At(MissionList, index) = *script_list.at(index);
        }

        Origin = WaypointFromLetters(origin_edit.Get_Text());
        // Text that names no waypoint of the scenario - letters past the last
        // one, or a letter and a space, which comes out negative - is none.
        if (Origin < 0 || Origin >= ScenarioClass::kWaypointCount) {
          Origin = -1;
        }
        return true;

      // Cancel: leave the team type as it was.
      case engine::window::ButtonKey(kButtonCancel):
        return false;

      // Unrecognized events are ignored. If any button related event is
      // detected, then collapse any drop down list boxes. This keeps the
      // dialog box clean.
      default:
        if (input & KN_BUTTON) {
          house_drop.Collapse();
          mission_drop.Collapse();
          trigger_drop.Collapse();
          redraw = true;
        }
        break;
    }

    // Record the last dialog control touched so that a double click
    // can be detected.
    if (input & KN_BUTTON) {
      last_control = ButtonId(input);
    }
  }
}

}  // namespace ra
