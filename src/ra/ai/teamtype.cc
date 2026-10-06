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

// Team types: reading and writing them in a scenario INI, picking one for the
// computer to build, and the scenario editor's dialog and list text for them.

#include "ra/ai/teamtype.h"

#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/log/check.h"
#include "absl/strings/match.h"
#include "absl/strings/str_format.h"
#include "base/array.h"
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
#include "engine/stream/readline.h"
#include "engine/window/hardware_cursor.h"
#include "engine/window/keyboard.h"
#include "game/core/object_heap.h"
#include "game/ui/control.h"
#include "game/ui/gadget.h"
#include "game/ui/input.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/ai/scenario.h"
#include "ra/ai/team.h"
#include "ra/ai/trigger_type.h"
#include "ra/assets/palettes.h"
#include "ra/assets/screen.h"
#include "ra/core/config.h"
#include "ra/core/const.h"
#include "ra/core/defines.h"
#include "ra/core/object_heaps.h"
#include "ra/core/random_pick.h"
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

// Parses a hex number for the editor's cell-number field.
static int atoh(std::string_view str);

void TeamType::Draw_It(PixelView& view, int /*unused*/, int x, int y, int width,
                       int height, bool selected, FontType font,
                       TextPrintType flags) const {
  if constexpr (config::kCheatKeysEnabled || config::kScenarioEditorEnabled) {
    RemapControlType* scheme = Get_Color_Scheme();
    // Pixel columns of Description()'s fields after the name: house, origin,
    // mission count, members.
    static const int _tabs[] = {35, 60, 80, 100};
    if (font == FontType::k6PointGradient || font == FontType::kEditor) {
      if (selected) {
        flags = flags | TPF_BRIGHT_COLOR;
        view.FillRect(x, y, x + width - 1, y + height - 1, scheme->Shadow);
      } else {
        if (!base::Any(flags & TPF_USE_GRAD_PAL)) {
          flags = flags | TPF_MEDIUM_COLOR;
        }
      }

      Conquer_Clip_Text_Print(view, Description().c_str(), x, y, scheme,
                              kTBlack, font, flags, width, _tabs);
    } else {
      Conquer_Clip_Text_Print(
          view, Description().c_str(), x, y,
          (selected ? &ThePalettes().color_remaps().at(PCOLOR_DIALOG_BLUE)
                    : &ThePalettes().color_remaps().at(PCOLOR_GREY)),
          kTBlack, font, flags, width, _tabs);
    }
  }
}

base::EnumArray<TeamMissionType, const char*> TeamType::TMissions = {
    "Attack...",
    "Attack Waypoint...",
    "Change Formation to...",
    "Move to waypoint...",
    "Move to Cell...",
    "Guard area (1/10th min)...",
    "Jump to line #...",
    "Attack Tarcom",
    "Unload",
    "Deploy",
    "Follow friendlies",
    "Do this...",
    "Set global...",
    "Invulnerable",
    "Load onto Transport",
    "Spy on bldg @ waypt...",
    "Patrol to waypoint..."};

TeamType::TeamType()
    : AbstractTypeClass(RTTI_TEAMTYPE,
                        TheObjectHeaps().team_type().SlotIndexOf(this),
                        TXT_NONE, "") {}

TeamType::~TeamType() = default;

void TeamType::Init() { TheObjectHeaps().team_type().FreeAll(); }

TeamType* TeamType::As_Pointer(const std::string_view name) {
  if (!name.empty()) {
    for (int index = 0; index < TheObjectHeaps().team_type().Count(); index++) {
      if (absl::EqualsIgnoreCase(
              name, TheObjectHeaps().team_type().ActiveAt(index)->IniName)) {
        return TheObjectHeaps().team_type().ActiveAt(index);
      }
    }
  }
  return nullptr;
}

const char* TeamType::Name_From_Mission(TeamMissionType order) {
  return TMissions.at(order);
}

void* TeamType::operator new(std::size_t /*unused*/) {
  void* ptr = TheObjectHeaps().team_type().Allocate();
  if (ptr) {
    static_cast<TeamType*>(ptr)->IsActive = true;
  }
  return ptr;
}

void TeamType::operator delete(void* ptr) {
  if (ptr) {
    static_cast<TeamType*>(ptr)->IsActive = false;
  }
  TheObjectHeaps().team_type().Free(static_cast<TeamType*>(ptr));
}

TeamClass* TeamType::Create_One_Of() {
  if (TheWorld().scenario_init() || std::cmp_less(Number, MaxAllowed)) {
    return base::NewPooled<TeamClass>(this, HouseClass::As_Pointer(House));
  }
  return nullptr;
}

void TeamType::Destroy_All_Of() const {
  for (int index = 0; index < TheObjectHeaps().team().Count(); index++) {
    const TeamClass* team = TheObjectHeaps().team().ActiveAt(index);

    if (team->Class == this) {
      // Deleting closes the gap in the heap's in-use list, so the next team
      // is now at this position.
      base::DeletePooled(team);
      index--;
    }
  }
}

TeamType* TeamType::Suggested_New_Team(HouseClass* house, uint64_t /*unused*/,
                                       uint64_t /*unused*/, uint64_t /*unused*/,
                                       uint64_t /*unused*/, bool alerted) {
  // Only the first 20 candidates in heap order take part in the draw.
  TeamType* choices[20];
  int choicecount = 0;

  for (int index = 0; index < TheObjectHeaps().team_type().Count(); index++) {
    TeamType* ttype = TheObjectHeaps().team_type().ActiveAt(index);

    DCHECK(ttype != nullptr);

    // A team type on the wrong side of the alert is treated as full.
    int maxnum = ttype->MaxAllowed;
    if ((alerted && !ttype->IsAutocreate) ||
        (!alerted && ttype->IsAutocreate)) {
      maxnum = 0;
    }

    if (choicecount >= std::ssize(choices)) {
      break;
    }

    if (ttype != nullptr && ttype->House == house->Class->House &&
        ttype->Number < maxnum) {
      base::At(choices, choicecount++) = ttype;
    }
  }

  if (choicecount > 0) {
    return base::At(choices, RandomPick(0, choicecount - 1));
  }
  return nullptr;
}

TeamType* TeamType::From_Name(const std::string_view name) {
  if (!name.empty()) {
    for (int index = 0; index < TheObjectHeaps().team_type().Count(); index++) {
      if (absl::EqualsIgnoreCase(
              name, TheObjectHeaps().team_type().ActiveAt(index)->IniName)) {
        return TheObjectHeaps().team_type().ActiveAt(index);
      }
    }
  }
  return nullptr;
}

NeedType TeamMission_Needs(TeamMissionType tmtype) {
  switch (tmtype) {
    case TeamMissionType::kFormation:
      return NEED_FORMATION;

    case TeamMissionType::kAttack:
      return NEED_QUARRY;

    // A cell number, which the editor shows and reads in hex.
    case TeamMissionType::kMoveCell:
      return NEED_HEX_NUMBER;

    case TeamMissionType::kSetGlobal:
    case TeamMissionType::kGuard:
    case TeamMissionType::kLoop:
      return NEED_NUMBER;

    case TeamMissionType::kPatrol:
    case TeamMissionType::kMove:
    case TeamMissionType::kAttackWaypoint:
    case TeamMissionType::kSpy:
      return NEED_WAYPOINT;

    case TeamMissionType::kDo:
      return NEED_MISSION;

    case TeamMissionType::kNone:
    case TeamMissionType::kAttackTarcom:
    case TeamMissionType::kUnload:
    case TeamMissionType::kDeploy:
    case TeamMissionType::kHoundDog:
    case TeamMissionType::kInvulnerable:
    case TeamMissionType::kLoad:
    default:
      break;
  }
  return NEED_NONE;
}

void TeamMission::Draw_It(PixelView& view, int index, int x, int y, int width,
                          int height, bool selected, FontType font,
                          TextPrintType flags) const {
  RemapControlType* scheme = Get_Color_Scheme();
  // Pixel columns of the mission's name and of a second field
  // Description() never writes.
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

    Conquer_Clip_Text_Print(view, Description(index).c_str(), x, y, scheme,
                            kTBlack, font, flags, width, _tabs);
  } else {
    Conquer_Clip_Text_Print(
        view, Description(index).c_str(), x, y,
        (selected ? &ThePalettes().color_remaps().at(PCOLOR_DIALOG_BLUE)
                  : &ThePalettes().color_remaps().at(PCOLOR_GREY)),
        kTBlack, font, flags, width, _tabs);
  }
}

bool TeamType::Edit() {
  // Dialog and button dimensions, in pixels of the editor's 640x400 screen.
  constexpr int kDDialogW = 400;
  constexpr int kDDialogH = 250;
  constexpr int kDDialogX = 0;
  constexpr int kDDialogY = 0;
  constexpr int kDNameX = kDDialogX + 35;
  constexpr int kDNameY = kDDialogY + 27;
  constexpr int kEdWidth = 40;
  constexpr int kDCheckX =
      kDDialogX + 35;  // Start of check box attribute list.
  constexpr int kDCheckY = kDNameY + 25;
  constexpr int kCbSpacingY = 9;  // Vertical spacing between check box lines.
  constexpr int kCbSpacingX =
      8;  // Horizontal spacing for check box description text.
  constexpr int kDSpacingX =
      9;  // Horizontal spacing between data entry fields.
  constexpr int kDCancelW = 50;
  constexpr int kDCancelH = 9;
  constexpr int kDCancelX = kDDialogX + kDDialogW - (kDCancelW + 35);
  constexpr int kDCancelY = kDDialogY + kDDialogH - (kDCancelH + 20);
  constexpr int kDOkW = 50;
  constexpr int kDOkH = 9;
  constexpr int kDOkX = kDDialogX + kDDialogW - ((kDOkW + 18) * 2);
  constexpr int kDOkY = kDCancelY;

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

  // Head of the dialog's control list.
  game::ControlClass* commands = nullptr;
  RemapControlType* scheme = Get_Color_Scheme();

  // Team name edit field.
  char name_buf[10];
  EditClass name_edt(kButtonName, name_buf, sizeof(name_buf), FontType::kEditor,
                     TPF_NOSHADOW, kDNameX, kDNameY, kEdWidth, 9,
                     EditClass::kAlphanumeric);
  base::SafeCopy(name_buf, IniName);
  commands = &name_edt;

  // House ownership of this team.
  char housetext[25] = "";
  DropListClass housebtn(kButtonHouse, housetext, sizeof(housetext),
                         FontType::kEditor, TPF_NOSHADOW,
                         name_edt.X + name_edt.Width + kDSpacingX, name_edt.Y,
                         55, 8 * 5, MixArchive::RetrieveData("EBTN-UP.SHP"),
                         MixArchive::RetrieveData("EBTN-DN.SHP"));
  for (const HousesType house : magic_enum::enum_values<HousesType>()) {
    housebtn.Add_Item(HouseTypeClass::As_Reference(house).IniName);
  }
  if (House == HOUSE_NONE) {
    House = HOUSE_GOOD;
  }
  housebtn.Set_Selected_Index(static_cast<int>(House));
  housebtn.Add(*commands);

  // Recruit priority for this team.
  char recr_buf[4];
  EditClass recr_edt(kButtonRecruit, recr_buf, sizeof(recr_buf),
                     FontType::kEditor, TPF_NOSHADOW,
                     housebtn.X + housebtn.Width + 5 + kDSpacingX, housebtn.Y,
                     kEdWidth, 9, EditClass::kNumeric);
  absl::SNPrintF(recr_buf, sizeof(recr_buf), "%d", RecruitPriority);
  recr_edt.Add(*commands);

  // Maximum allowed for this team type.
  char maxnum_buf[4];
  EditClass maxnum_edt(kButtonMaxnum, maxnum_buf, sizeof(maxnum_buf),
                       FontType::kEditor, TPF_NOSHADOW,
                       recr_edt.X + recr_edt.Width + kDSpacingX, recr_edt.Y,
                       kEdWidth, 9, EditClass::kNumeric);
  absl::SNPrintF(maxnum_buf, sizeof(maxnum_buf), "%d", MaxAllowed);
  maxnum_edt.Add(*commands);

  // Initial number for this team type.
  char initnum_buf[4];
  EditClass initnum_edt(kButtonInitnum, initnum_buf, sizeof(initnum_buf),
                        FontType::kEditor, TPF_NOSHADOW,
                        maxnum_edt.X + maxnum_edt.Width + kDSpacingX,
                        maxnum_edt.Y, kEdWidth, 9, EditClass::kNumeric);
  absl::SNPrintF(initnum_buf, sizeof(initnum_buf), "%d", InitNum);
  initnum_edt.Add(*commands);

  // Waypoint to create or reinforce this team at, as the editor's letters;
  // empty for none.
  char origin[4];
  EditClass originbtn(kButtonOrigin, origin, sizeof(origin), FontType::kEditor,
                      TPF_NOSHADOW,
                      initnum_edt.X + initnum_edt.Width + kDSpacingX,
                      initnum_edt.Y, 20, 9, EditClass::kAlpha);
  *originbtn.Get_Text() = '\0';
  if (Origin != -1) {
    if (Origin < 26) {
      absl::SNPrintF(originbtn.Get_Text(), sizeof(origin), "%c", Origin + 'A');
    } else {
      absl::SNPrintF(originbtn.Get_Text(), sizeof(origin), "%c%c",
                     (Origin / 26) + 'A' - 1, (Origin % 26) + 'A');
    }
  }
  originbtn.Add(*commands);

  // Button that opens the members dialog.
  TextButtonClass membersbtn(kButtonMembers, "Members", kEButtonFont,
                             kEButtonFlags, name_edt.X, name_edt.Y + 12, 50);
  membersbtn.Add(*commands);

  // Trigger to assign to each member of this team (when object joins team).
  // Entry 0 is "<NONE>", so a trigger type's entry is its heap position + 1.
  char trigtext[25] = "";
  DropListClass triggerbtn(kButtonTrigger, trigtext, sizeof(trigtext),
                           FontType::kEditor, TPF_NOSHADOW,
                           kDDialogX + kDDialogW - 95, membersbtn.Y, 60, 8 * 5,
                           MixArchive::RetrieveData("EBTN-UP.SHP"),
                           MixArchive::RetrieveData("EBTN-DN.SHP"));
  triggerbtn.Add_Item("<NONE>");
  for (int index = 0; index < TheObjectHeaps().trigger_type().Count();
       index++) {
    triggerbtn.Add_Item(
        TheObjectHeaps().trigger_type().ActiveAt(index)->IniName);
  }
  if (Trigger != nullptr) {
    triggerbtn.Set_Selected_Index(Trigger->Name());
  } else {
    triggerbtn.Set_Selected_Index(0);
  }
  triggerbtn.Add(*commands);

  // Roundabout travel logic attribute for this team.
  CheckBoxClass roundbtn(kButtonRoundabout, kDCheckX, kDCheckY);
  if (IsRoundAbout) {
    roundbtn.Turn_On();
  } else {
    roundbtn.Turn_Off();
  }
  roundbtn.Add(*commands);

  // Suicide travel to target attribute.
  CheckBoxClass suicidebtn(kButtonSuicide, kDCheckX, roundbtn.Y + kCbSpacingY);
  if (IsSuicide) {
    suicidebtn.Turn_On();
  } else {
    suicidebtn.Turn_Off();
  }
  suicidebtn.Add(*commands);

  // Autocreate attribute for this team.
  CheckBoxClass autocreatebtn(kButtonAuto, kDCheckX,
                              suicidebtn.Y + kCbSpacingY);
  if (IsAutocreate) {
    autocreatebtn.Turn_On();
  } else {
    autocreatebtn.Turn_Off();
  }
  autocreatebtn.Add(*commands);

  // Prebuild team members attribute for this team.
  CheckBoxClass prebuildbtn(kButtonPrebuilt, kDCheckX,
                            autocreatebtn.Y + kCbSpacingY);
  if (IsPrebuilt) {
    prebuildbtn.Turn_On();
  } else {
    prebuildbtn.Turn_Off();
  }
  prebuildbtn.Add(*commands);

  // Reinforce this team in progress attribute.
  CheckBoxClass reinforcebtn(kButtonReinforce, kDCheckX,
                             prebuildbtn.Y + kCbSpacingY);
  if (IsReinforcable) {
    reinforcebtn.Turn_On();
  } else {
    reinforcebtn.Turn_Off();
  }
  reinforcebtn.Add(*commands);

  // Drop list of the possible team missions, in TeamMissionType order so an
  // entry's index is its mission. It and the argument control beside it hold
  // the "work mission" that Append, Insert and Replace put into the script.
  char droptext[45];
  DropListClass missionlist1(kButtonMission1, droptext, sizeof(droptext),
                             FontType::kEditor, TPF_NOSHADOW, reinforcebtn.X,
                             reinforcebtn.Y + 15, 170, 8 * 8,
                             MixArchive::RetrieveData("EBTN-UP.SHP"),
                             MixArchive::RetrieveData("EBTN-DN.SHP"));
  for (const TeamMissionType tm : magic_enum::enum_values<TeamMissionType>()) {
    missionlist1.Add_Item(TeamType::Name_From_Mission(tm));
  }
  missionlist1.Set_Selected_Index(0);
  missionlist1.Add_Tail(*commands);

  // The mission's argument. Four controls share the spot beside the mission
  // list and the redraw links in the one the selected mission needs: a text
  // field for a number or waypoint, or a list of quarries, formations or unit
  // missions.
  char arg_buf[6] = {0};
  EditClass arg_edt(kButtonArg, arg_buf, sizeof(arg_buf), FontType::kEditor,
                    TPF_NOSHADOW, missionlist1.X + missionlist1.Width + 15,
                    missionlist1.Y, 60, -1, EditClass::kAlphanumeric);

  char qtext[55];
  DropListClass qlist(kButtonQuarry, qtext, sizeof(qtext), FontType::kEditor,
                      TPF_NOSHADOW, missionlist1.X + missionlist1.Width + 15,
                      missionlist1.Y, 100, 5 * 8,
                      MixArchive::RetrieveData("EBTN-UP.SHP"),
                      MixArchive::RetrieveData("EBTN-DN.SHP"));
  for (const QuarryType q : magic_enum::enum_values<QuarryType>()) {
    qlist.Add_Item(QuarryName.at(q));
  }
  qlist.Set_Selected_Index(0);
  qlist.Add_Tail(*commands);

  char ftext[55];
  DropListClass flist(kButtonFormation, ftext, sizeof(ftext), FontType::kEditor,
                      TPF_NOSHADOW, missionlist1.X + missionlist1.Width + 15,
                      missionlist1.Y, 100, 5 * 8,
                      MixArchive::RetrieveData("EBTN-UP.SHP"),
                      MixArchive::RetrieveData("EBTN-DN.SHP"));
  for (const FormationType f : magic_enum::enum_values<FormationType>()) {
    flist.Add_Item(FormationName.at(f));
  }
  flist.Set_Selected_Index(0);
  flist.Add_Tail(*commands);

  char mtext[55];
  DropListClass mlist(kButtonMission, mtext, sizeof(mtext), FontType::kEditor,
                      TPF_NOSHADOW, missionlist1.X + missionlist1.Width + 15,
                      missionlist1.Y, 100, 5 * 8,
                      MixArchive::RetrieveData("EBTN-UP.SHP"),
                      MixArchive::RetrieveData("EBTN-DN.SHP"));
  for (const MissionType m : magic_enum::enum_values<MissionType>()) {
    mlist.Add_Item(MissionClass::Mission_Name(m));
  }
  mlist.Set_Selected_Index(0);
  mlist.Add_Tail(*commands);

  // Owns the missions missionlist2 shows: copies of the team type's own, and
  // the ones the dialog adds.
  std::vector<std::unique_ptr<TeamMission>> missions;
  TListClass<TeamMission*> missionlist2(
      kButtonMission2, missionlist1.X + 60, missionlist1.Y + 22, 240, 8 * 7,
      FontType::kEditor, TPF_NOSHADOW, MixArchive::RetrieveData("EBTN-UP.SHP"),
      MixArchive::RetrieveData("EBTN-DN.SHP"));
  for (int index = 0; index < MissionCount; index++) {
    missionlist2.Add_Item(missions
                              .emplace_back(std::make_unique<TeamMission>(
                                  base::At(MissionList, index)))
                              .get());
  }
  static const int tabs[] = {13, 40};  // list box tab stops, in pixels
  missionlist2.Set_Tabs(tabs);
  missionlist2.Add_Tail(*commands);

  // Mission editing command buttons.
  TextButtonClass addbtn(kButtonAdd, "Append", kEButtonFont, kEButtonFlags,
                         kDNameX, missionlist1.Y + missionlist1.Height + 1, 50);
  addbtn.Add(*commands);

  TextButtonClass insertbtn(kButtonInsert, "Insert", kEButtonFont,
                            kEButtonFlags, addbtn.X, addbtn.Y + 10, 50);
  insertbtn.Add(*commands);

  TextButtonClass delbtn(kButtonDelete, "Delete", kEButtonFont, kEButtonFlags,
                         insertbtn.X, insertbtn.Y + 10, 50);
  delbtn.Add(*commands);

  TextButtonClass repbtn(kButtonReplace, "Replace", kEButtonFont, kEButtonFlags,
                         delbtn.X, delbtn.Y + 10, 50);
  repbtn.Add(*commands);

  TextButtonClass okbtn(kButtonOk, TXT_OK, kEButtonFont, kEButtonFlags, kDOkX,
                        kDOkY, kDOkW, kDOkH);
  TextButtonClass cancelbtn(kButtonCancel, TXT_CANCEL, kEButtonFont,
                            kEButtonFlags, kDCancelX, kDCancelY, kDCancelW,
                            kDCancelH);

  PixelView& view = TheScreen().visible_view();

  okbtn.Add(*commands);
  cancelbtn.Add_Tail(*commands);

  // Main processing loop. The last* values are the script list's selection,
  // its length and the control used on the previous event (-1 before any):
  // clicking the same script line twice in a row loads it into the work
  // mission.
  int lastindex = -1;
  int lastcount = -1;
  int lastbutton = -1;
  bool cancel = false;
  bool display = true;
  bool process = true;
  while (process) {
    // Keep the sound, timers and network serviced while the dialog is up.
    ServiceRealTime();

    // Refresh display if needed
    if (display) {
      // Link in only the argument control the work mission needs.
      arg_edt.Remove();
      qlist.Remove();
      flist.Remove();
      mlist.Remove();
      switch (
          TeamMission_Needs(TeamMissionType(missionlist1.Current_Index()))) {
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
          mlist.Add(*commands);
          break;

        case NEED_FORMATION:
          flist.Add(*commands);
          break;

        case NEED_WAYPOINT:
        case NEED_NUMBER:
        case NEED_HEX_NUMBER:
          arg_edt.Add(*commands);
          break;

        case NEED_QUARRY:
          qlist.Add(*commands);
          break;
      }

      // Display the dialog box
      game::TheMouse().Hide();
      Dialog_Box(kDDialogX, kDDialogY, kDDialogW, kDDialogH);

      Draw_Caption(view, TXT_TEAM_EDIT, kDDialogX, kDDialogY, kDDialogW);

      // Draw the captions
      Fancy_Text_Print(view, "Name:", name_edt.X, name_edt.Y - 7, scheme,
                       kTBlack, FontType::kEditor, TPF_NOSHADOW);
      Fancy_Text_Print(view, "House:", housebtn.X, housebtn.Y - 7, scheme,
                       kTBlack, FontType::kEditor, TPF_NOSHADOW);
      Fancy_Text_Print(view, "Pri:", recr_edt.X, recr_edt.Y - 7, scheme,
                       kTBlack, FontType::kEditor, TPF_NOSHADOW);
      Fancy_Text_Print(view, "Max:", maxnum_edt.X, maxnum_edt.Y - 7, scheme,
                       kTBlack, FontType::kEditor, TPF_NOSHADOW);
      Fancy_Text_Print(view, "Num:", initnum_edt.X, initnum_edt.Y - 7, scheme,
                       kTBlack, FontType::kEditor, TPF_NOSHADOW);
      Fancy_Text_Print(view, "Loc:", originbtn.X, originbtn.Y - 7, scheme,
                       kTBlack, FontType::kEditor, TPF_NOSHADOW);
      Fancy_Text_Print(view, "#  Team Mission", missionlist2.X,
                       missionlist2.Y - 7, scheme, kTBlack, FontType::kEditor,
                       TPF_NOSHADOW);
      Fancy_Text_Print(view, "Trigger:", triggerbtn.X - 4, triggerbtn.Y + 1,
                       scheme, kTBlack, FontType::kEditor,
                       TPF_RIGHT | TPF_NOSHADOW);

      Fancy_Text_Print(view, Member_Description().c_str(),
                       membersbtn.X + membersbtn.Width + 3, membersbtn.Y + 1,
                       scheme, kTBlack, FontType::kEditor, TPF_NOSHADOW);

      Fancy_Text_Print(view, "Use safest, possibly longer, route to target?",
                       roundbtn.X + kCbSpacingX, roundbtn.Y, scheme, kTBlack,
                       FontType::kEditor, TPF_NOSHADOW);
      Fancy_Text_Print(view, "Charge toward target ignoring distractions?",
                       suicidebtn.X + kCbSpacingX, suicidebtn.Y, scheme,
                       kTBlack, FontType::kEditor, TPF_NOSHADOW);
      Fancy_Text_Print(view, "Only 'Autocreate A.I.' uses this team type?",
                       autocreatebtn.X + kCbSpacingX, autocreatebtn.Y, scheme,
                       kTBlack, FontType::kEditor, TPF_NOSHADOW);
      Fancy_Text_Print(view, "Prebuild team members before team is created?",
                       prebuildbtn.X + kCbSpacingX, prebuildbtn.Y, scheme,
                       kTBlack, FontType::kEditor, TPF_NOSHADOW);
      Fancy_Text_Print(view, "Automatically reinforce team whenever possible?",
                       reinforcebtn.X + kCbSpacingX, reinforcebtn.Y, scheme,
                       kTBlack, FontType::kEditor, TPF_NOSHADOW);

      // Redraw the buttons
      commands->Draw_All(view);
      game::TheMouse().Show();

      display = false;
    }

    // Get user input
    const engine::window::KeyNumber input = commands->Input(view).key;

    // Process input
    switch (static_cast<int>(input)) {
      // A click on the script. The second click on the same line, with no
      // other control used in between, copies that line into the work mission
      // and its argument control as an aid to editing.
      case engine::window::ButtonKey(kButtonMission2):
        if (missionlist2.Count() && lastcount == missionlist2.Count() &&
            lastbutton == kButtonMission2 &&
            lastindex == missionlist2.Current_Index()) {
          missionlist1.Set_Selected_Index(
              static_cast<int>(missionlist2.Current_Item()->Mission));

          switch (TeamMission_Needs(missionlist2.Current_Item()->Mission)) {
            case NEED_MISSION:
              mlist.Set_Selected_Index(
                  static_cast<int>(missionlist2.Current_Item()->Data.Mission));
              break;

            case NEED_FORMATION:
              flist.Set_Selected_Index(static_cast<int>(
                  missionlist2.Current_Item()->Data.Formation));
              break;

            case NEED_NUMBER:
              absl::SNPrintF(arg_edt.Get_Text(), sizeof(arg_buf), "%d",
                             missionlist2.Current_Item()->Data.Value);
              break;

            case NEED_HEX_NUMBER:
              absl::SNPrintF(arg_edt.Get_Text(), sizeof(arg_buf), "%x",
                             static_cast<unsigned int>(
                                 missionlist2.Current_Item()->Data.Value));
              break;

            case NEED_QUARRY:
              qlist.Set_Selected_Index(
                  static_cast<int>(missionlist2.Current_Item()->Data.Quarry));
              break;

            case NEED_WAYPOINT:
              if (missionlist2.Current_Item()->Data.Value < 26) {
                absl::SNPrintF(arg_edt.Get_Text(), sizeof(arg_buf), "%c",
                               missionlist2.Current_Item()->Data.Value + 'A');
              } else {
                absl::SNPrintF(
                    arg_edt.Get_Text(), sizeof(arg_buf), "%c%c",
                    (missionlist2.Current_Item()->Data.Value / 26) + 'A' - 1,
                    (missionlist2.Current_Item()->Data.Value % 26) + 'A');
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
        }
        lastindex = missionlist2.Current_Index();
        lastcount = missionlist2.Count();
        display = true;
        break;

      // Insert the work mission at the selected line of the script; the
      // lines from there on move down.
      case engine::window::ButtonKey(kButtonInsert):
        if (missionlist2.Count() < kMaxTeamMissions) {
          auto* tm =
              missions.emplace_back(std::make_unique<TeamMission>()).get();
          tm->Mission = TeamMissionType(missionlist1.Current_Index());
          tm->Data.Value = 0;
          switch (TeamMission_Needs(tm->Mission)) {
            case NEED_MISSION:
              tm->Data.Mission = MissionType(mlist.Current_Index());
              break;

            case NEED_FORMATION:
              tm->Data.Formation = FormationType(flist.Current_Index());
              break;

            case NEED_QUARRY:
              tm->Data.Quarry = QuarryType(qlist.Current_Index());
              break;

            case NEED_NUMBER:
              tm->Data.Value = base::ParseIntegerOr<int>(arg_edt.Get_Text(), 0);
              break;

            case NEED_HEX_NUMBER:
              tm->Data.Value = atoh(arg_edt.Get_Text());
              break;

            case NEED_WAYPOINT:
              tm->Data.Value = toupper(*arg_edt.Get_Text()) - 'A';
              if (std::string_view(arg_edt.Get_Text()).size() > 1) {
                tm->Data.Value = (tm->Data.Value + 1) * 26;
                tm->Data.Value +=
                    toupper(std::string_view(arg_edt.Get_Text()).at(1)) - 'A';
              }
              if (tm->Data.Value < 0 ||
                  tm->Data.Value >= ScenarioClass::kHomeWaypoint) {
                tm->Data.Value = 0;
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
              tm->Data.Value = 0;
              break;
          }
          missionlist2.Insert_Item(tm);
        }
        display = true;
        break;

      // Append the work mission to the script.
      case engine::window::ButtonKey(kButtonAdd):
        if (missionlist2.Count() < kMaxTeamMissions) {
          auto* tm =
              missions.emplace_back(std::make_unique<TeamMission>()).get();
          tm->Mission = TeamMissionType(missionlist1.Current_Index());
          tm->Data.Value = 0;
          switch (TeamMission_Needs(tm->Mission)) {
            case NEED_MISSION:
              tm->Data.Mission = MissionType(mlist.Current_Index());
              break;

            case NEED_FORMATION:
              tm->Data.Formation = FormationType(flist.Current_Index());
              break;

            case NEED_QUARRY:
              tm->Data.Quarry = QuarryType(qlist.Current_Index());
              break;

            case NEED_NUMBER:
              tm->Data.Value = base::ParseIntegerOr<int>(arg_edt.Get_Text(), 0);
              break;

            case NEED_HEX_NUMBER:
              tm->Data.Value = atoh(arg_edt.Get_Text());
              break;

            case NEED_WAYPOINT:
              tm->Data.Value = toupper(*arg_edt.Get_Text()) - 'A';
              if (std::string_view(arg_edt.Get_Text()).size() > 1) {
                tm->Data.Value = (tm->Data.Value + 1) * 26;
                tm->Data.Value +=
                    toupper(std::string_view(arg_edt.Get_Text()).at(1)) - 'A';
              }
              if (tm->Data.Value < 0 ||
                  tm->Data.Value >= ScenarioClass::kHomeWaypoint) {
                tm->Data.Value = 0;
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
              tm->Data.Value = 0;
              break;
          }
          missionlist2.Add_Item(tm);
          display = true;
        }
        break;

      // Overwrite the selected line of the script with the work mission.
      case engine::window::ButtonKey(kButtonReplace):
        if (missionlist2.Count()) {
          TeamMission* tm = missionlist2.Current_Item();
          tm->Mission = TeamMissionType(missionlist1.Current_Index());
          tm->Data.Value = 0;
          switch (TeamMission_Needs(tm->Mission)) {
            case NEED_MISSION:
              tm->Data.Mission = MissionType(mlist.Current_Index());
              break;

            case NEED_FORMATION:
              tm->Data.Formation = FormationType(flist.Current_Index());
              break;

            case NEED_QUARRY:
              tm->Data.Quarry = QuarryType(qlist.Current_Index());
              break;

            case NEED_NUMBER:
              tm->Data.Value = base::ParseIntegerOr<int>(arg_edt.Get_Text(), 0);
              break;

            case NEED_HEX_NUMBER:
              tm->Data.Value = atoh(arg_edt.Get_Text());
              break;

            case NEED_WAYPOINT:
              tm->Data.Value = toupper(*arg_edt.Get_Text()) - 'A';
              if (std::string_view(arg_edt.Get_Text()).size() > 1) {
                tm->Data.Value = (tm->Data.Value + 1) * 26;
                tm->Data.Value +=
                    toupper(std::string_view(arg_edt.Get_Text()).at(1)) - 'A';
              }
              if (tm->Data.Value < 0 ||
                  tm->Data.Value >= ScenarioClass::kHomeWaypoint) {
                tm->Data.Value = 0;
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
              tm->Data.Value = 0;
              break;
          }
          missionlist2.at(missionlist2.Current_Index()) = tm;
        }
        display = true;
        break;

      // Delete the currently selected mission.
      case engine::window::ButtonKey(kButtonDelete):
        if (missionlist2.Count()) {
          const TeamMission* tm = missionlist2.Current_Item();
          missionlist2.Remove_Index(missionlist2.Current_Index());
          std::erase_if(missions,
                        [tm](const auto& owned) { return owned.get() == tm; });
        }
        display = true;
        break;

      // Invoke the members dialog
      case engine::window::ButtonKey(kButtonMembers):

        // The button stays pressed otherwise: the other dialog takes the
        // release.
        membersbtn.Turn_Off();

        TheMap().Team_Members(HousesType(housebtn.Current_Index()));

        display = true;
        break;

      // OK: copy the dialog's values into the team type. A blank name becomes
      // "----" so the team type can still be listed and written to the INI.
      case engine::window::ButtonKey(kButtonOk):
        strtrim(name_edt.Get_Text_Buffer());
        if (!std::string_view(name_edt.Get_Text()).empty()) {
          base::SafeCopy(IniName, name_edt.Get_Text());
        } else {
          base::SafeCopy(IniName, "----");
        }

        IsRoundAbout = roundbtn.IsOn;
        IsSuicide = suicidebtn.IsOn;
        IsAutocreate = autocreatebtn.IsOn;
        IsPrebuilt = prebuildbtn.IsOn;
        IsReinforcable = reinforcebtn.IsOn;

        RecruitPriority = base::ParseIntegerOr<int>(recr_edt.Get_Text(), 0);
        InitNum = static_cast<unsigned char>(
            base::ParseIntegerOr<int>(initnum_edt.Get_Text(), 0));
        MaxAllowed = static_cast<unsigned char>(
            base::ParseIntegerOr<int>(maxnum_edt.Get_Text(), 0));
        House = HousesType(housebtn.Current_Index());
        Trigger = nullptr;
        if (triggerbtn.Current_Index() > 0) {
          Trigger = TheObjectHeaps().trigger_type().ActiveAt(
              triggerbtn.Current_Index() - 1);
        }

        MissionCount = missionlist2.Count();
        for (int index = 0; index < MissionCount; index++) {
          base::At(MissionList, index).Data.Value = 0;
          base::At(MissionList, index) = *missionlist2.at(index);
        }

        if (!std::string_view(originbtn.Get_Text()).empty()) {
          if (std::string_view(originbtn.Get_Text()).size() == 1) {
            Origin = toupper(*originbtn.Get_Text()) - 'A';
          } else {
            Origin = (toupper(*originbtn.Get_Text()) + 1 - 'A') * 26;
            Origin +=
                toupper(std::string_view(originbtn.Get_Text()).at(1)) - 'A';
          }
        } else {
          Origin = -1;
        }
        // Letters past the scenario's last waypoint name none.
        if (Origin >= ScenarioClass::kWaypointCount) {
          Origin = -1;
        }
        cancel = false;
        process = false;
        break;

      // Cancel: leave the team type as it was.
      case engine::window::ButtonKey(kButtonCancel):
        cancel = true;
        process = false;
        break;

      // Unrecognized events are ignored. If any button related event is
      // detected, then collapse any drop down list boxes. This keeps the
      // dialog box clean.
      default:
        if (input & KN_BUTTON) {
          housebtn.Collapse();
          missionlist1.Collapse();
          triggerbtn.Collapse();
          display = true;
        }
        break;
    }

    // Record the last dialog control touched so that a double click
    // can be detected.
    if (input & KN_BUTTON) {
      lastbutton = ButtonId(input);
    }
  }

  return (!cancel);
}

// Returns the value of the hex digits in `str`. A character that is not a
// hex digit counts as 0.
int atoh(const std::string_view str) {
  int retval = 0;
  for (const char chr : str) {
    retval *= 16;
    if (chr >= '0' && chr <= '9') {
      retval += chr - '0';
    } else {
      if (chr >= 'a' && chr <= 'f') {
        retval += 10 + (chr - 'a');
      } else {
        if (chr >= 'A' && chr <= 'F') {
          retval += 10 + (chr - 'A');
        }
      }
    }
  }
  return retval;
}

std::string TeamType::Member_Description() const {
  std::string text;
  if constexpr (config::kCheatKeysEnabled || config::kScenarioEditorEnabled) {
    // "TYPE:count" for each kind of member, comma separated.
    for (int index = 0; index < ClassCount; index++) {
      const TeamMember& member = base::At(Members, index);
      absl::StrAppendFormat(&text, "%s%s:%d", index > 0 ? "," : "",
                            member.Class->IniName, member.Quantity);
    }

    // The team list's last column has room for 25 characters.
    constexpr std::string_view kEllipsis = "...";
    constexpr std::size_t kMaxLength = 25;
    if (text.size() > kMaxLength) {
      text.resize(kMaxLength - kEllipsis.size());
      text += kEllipsis;
    }
  }
  return text;
}

std::string TeamType::Description() const {
  if constexpr (config::kCheatKeysEnabled || config::kScenarioEditorEnabled) {
    // The origin waypoint in the editor's letters: "A" to "Z", then "AA" on.
    char loc[3] = {};
    if (Origin > -1) {
      if (Origin < 26) {
        loc[0] = static_cast<char>('A' + Origin);
      } else {
        loc[0] = static_cast<char>((Origin / 26) + 'A' - 1);
        loc[1] = static_cast<char>((Origin % 26) + 'A');
      }
    }

    // A team read from a scenario that names no house for it has none.
    const char* const house_suffix =
        House == HOUSE_NONE ? "" : HouseTypeClass::As_Reference(House).Suffix;
    return absl::StrFormat("%s\t%s\t%c%s\t%d\t%s", IniName, house_suffix,
                           IsAutocreate ? '*' : ' ', loc, MissionCount,
                           Member_Description());
  }
  return "";
}

std::string TeamMission::Description(int index) const {
  if constexpr (config::kCheatKeysEnabled || config::kScenarioEditorEnabled) {
    std::string text =
        absl::StrFormat("%d\t%s", index, TeamType::Name_From_Mission(Mission));

    switch (TeamMission_Needs(Mission)) {
      case NEED_MISSION:
        text += MissionClass::Mission_Name(Data.Mission);
        break;

      case NEED_FORMATION:
        text += FormationName.at(Data.Formation);
        break;

      case NEED_NUMBER:
        absl::StrAppendFormat(&text, "%d", Data.Value);
        break;

      case NEED_HEX_NUMBER:
        absl::StrAppendFormat(&text, "%x",
                              static_cast<unsigned int>(Data.Value));
        break;

      case NEED_QUARRY:
        text += QuarryName.at(Data.Quarry);
        break;

      case NEED_WAYPOINT:
        if (Data.Value < 26) {
          absl::StrAppendFormat(&text, "%c", Data.Value + 'A');
        } else {
          absl::StrAppendFormat(&text, "%c%c", (Data.Value / 26) + 'A' - 1,
                                (Data.Value % 26) + 'A');
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

    return text;
  }
  return "";
}

void TeamType::Detach(Target target, bool /*unused*/) {
  if (target.kind() == RTTI_TRIGGERTYPE && Trigger != nullptr &&
      Trigger == target.AsTriggerType()) {
    Trigger = nullptr;
  }
}

void TeamType::Read_INI(engine::file::IniFile& ini) {
  for (const auto& [entry, value] : ini.Entries(INI_Name())) {
    auto* team = base::NewPooled<TeamType>();
    if (team != nullptr) {
      team->Fill_In(entry, value);
    }
  }
}

void TeamType::Fill_In(std::string_view name, std::string_view entry) {
  DCHECK_HEAP_SLOT(TheObjectHeaps().team_type(), this);

  Set_Name(name);

  base::Tokenizer tokens(entry);
  // TODO: the house and the mission numbers below are taken from the file
  // unchecked; an out-of-range one CHECK-fails later, where the team type is
  // used.
  House = static_cast<HousesType>(base::ParseIntegerOr<int>(tokens.Next(), 0));

  // The scenario's NewINIFormat: 2 and up pack the options into one bit
  // field and put the trigger after the origin; 0 and 1 list each option and
  // end the entry with the trigger.
  uint32_t code = 0;
  switch (TheWorld().new_ini_format()) {
    default:
      code = base::ParseIntegerOr<uint32_t>(tokens.Next(), 0);
      IsRoundAbout = (code & 0x0001) != 0;
      IsSuicide = (code & 0x0002) != 0;
      IsAutocreate = (code & 0x0004) != 0;
      IsPrebuilt = (code & 0x0008) != 0;
      IsReinforcable = (code & 0x0010) != 0;
      break;

    case 0:
    case 1:
      IsRoundAbout = base::ParseIntegerOr<int>(tokens.Next(), 0) != 0;
      IsSuicide = base::ParseIntegerOr<int>(tokens.Next(), 0) != 0;
      IsAutocreate = base::ParseIntegerOr<int>(tokens.Next(), 0) != 0;
      IsPrebuilt = base::ParseIntegerOr<int>(tokens.Next(), 0) != 0;
      IsReinforcable = base::ParseIntegerOr<int>(tokens.Next(), 0) != 0;
      break;
  }

  RecruitPriority = base::ParseIntegerOr<int>(tokens.Next(), 0);
  InitNum =
      static_cast<unsigned char>(base::ParseIntegerOr<int>(tokens.Next(), 0));
  MaxAllowed =
      static_cast<unsigned char>(base::ParseIntegerOr<int>(tokens.Next(), 0));
  Origin = base::ParseIntegerOr<int>(tokens.Next(), 0);
  if (Origin < 0 || Origin >= ScenarioClass::kWaypointCount) {
    Origin = -1;
  }

  switch (TheWorld().new_ini_format()) {
    default:
      // The trigger types are read after the team types; the slot is where
      // that trigger type will be. -1 gives nullptr.
      Trigger = TheObjectHeaps().trigger_type().SlotOrNull(
          base::ParseIntegerOr<int>(tokens.Next(), 0));
      break;

    case 0:
    case 1:
      tokens.Next();  // Old-format trigger field, unused.
      break;
  }

  // The members: a count, then that many Type:Quantity pairs.
  ClassCount = base::ParseIntegerOr<int>(tokens.Next(), -1);
  if (ClassCount < 0 || ClassCount > kMaxTeamClasscount) {
    ClassCount = 0;
    MissionCount = 0;
    return;
  }
  for (int index = 0; index < ClassCount; index++) {
    const std::string_view p1 = tokens.Next(",:");
    const std::string_view p2 = tokens.Next(",:");
    if (p1.empty() || p2.empty()) {
      ClassCount = 0;
      MissionCount = 0;
      return;
    }
    const TechnoTypeClass* otype = nullptr;

    // The name may be of an infantry, unit, aircraft or vessel type.
    const InfantryType i_id = InfantryTypeClass::From_Name(p1);
    if (i_id != INFANTRY_NONE) {
      otype = &InfantryTypeClass::As_Reference(i_id);
    } else {
      const UnitType u_id = UnitTypeClass::From_Name(p1);
      if (u_id != UNIT_NONE) {
        otype = &UnitTypeClass::As_Reference(u_id);
      } else {
        const AircraftType a_id = AircraftTypeClass::From_Name(p1);
        if (a_id != AIRCRAFT_NONE) {
          otype = &AircraftTypeClass::As_Reference(a_id);
        } else {
          const VesselType v_id = VesselTypeClass::From_Name(p1);
          if (v_id != VESSEL_NONE) {
            otype = &VesselTypeClass::As_Reference(v_id);
          }
        }
      }
    }

    // An unknown type (a unit this build does not have) is dropped and the
    // next member takes its place in Members.
    if (otype) {
      base::At(Members, index).Class = otype;
      base::At(Members, index).Quantity = base::ParseIntegerOr<int>(p2, 0);
    } else {
      ClassCount--;
      index--;
    }
  }

  // The script: a count, then that many Mission:Argument pairs.
  MissionCount = base::ParseIntegerOr<int>(tokens.Next(), -1);
  if (MissionCount < 0 || MissionCount > kMaxTeamMissions) {
    ClassCount = 0;
    MissionCount = 0;
    return;
  }
  for (int index = 0; index < MissionCount; index++) {
    base::At(MissionList, index).Mission = static_cast<TeamMissionType>(
        base::ParseIntegerOr<int>(tokens.Next(",:"), 0));
    base::At(MissionList, index).Data.Value =
        base::ParseIntegerOr<int>(tokens.Next(",:"), 0);
  }

  if (TheWorld().new_ini_format() < 2) {
    // The older formats put the trigger last.
    Trigger = TheObjectHeaps().trigger_type().SlotOrNull(
        base::ParseIntegerOr<int>(tokens.Next(), 0));
  }
}

void TeamType::Write_INI(engine::file::IniFile& ini) {
  // TODO: the same section is cleared twice.
  ini.Clear("TeamTypes");
  ini.Clear(INI_Name());

  for (int index = 0; index < TheObjectHeaps().team_type().Count(); index++) {
    TeamType* team = TheObjectHeaps().team_type().ActiveAt(index);
    ini.PutString(INI_Name(), team->IniName, team->Build_INI_Entry());
  }
}

std::string TeamType::Build_INI_Entry() const {
  uint32_t code = 0;
  code |= IsRoundAbout ? 0x0001U : 0U;
  code |= IsSuicide ? 0x0002U : 0U;
  code |= IsAutocreate ? 0x0004U : 0U;
  code |= IsPrebuilt ? 0x0008U : 0U;
  code |= IsReinforcable ? 0x0010U : 0U;

  std::string buf = absl::StrFormat(
      "%d,%d,%d,%d,%d,%d,%d", House, code, RecruitPriority, InitNum, MaxAllowed,
      Origin, TheObjectHeaps().trigger_type().PositionOf(Trigger));

  // The members: a count, then Type:Quantity for each.
  absl::StrAppendFormat(&buf, ",%d", ClassCount);
  for (int i = 0; i < ClassCount; i++) {
    absl::StrAppendFormat(&buf, ",%s:%d", base::At(Members, i).Class->IniName,
                          base::At(Members, i).Quantity);
  }

  // The script: a count, then Mission:Argument for each, both as numbers.
  absl::StrAppendFormat(&buf, ",%d", MissionCount);
  for (int i = 0; i < MissionCount; i++) {
    absl::StrAppendFormat(&buf, ",%d:%d", base::At(MissionList, i).Mission,
                          base::At(MissionList, i).Data.Value);
  }
  return buf;
}

}  // namespace ra
