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

// Team types: reading and writing them in a scenario INI, picking one for the
// computer to build, and the scenario editor's list text for them.

#include "ra/ai/teamtype.h"

#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/strings/match.h"
#include "absl/strings/str_format.h"
#include "base/array.h"
#include "base/enum_array.h"
#include "base/numeric.h"
#include "base/pooled.h"
#include "base/strings/number_parse.h"
#include "base/strings/tokenizer.h"
#include "engine/file/ini_file.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/wwstd.h"
#include "engine/window/keyboard.h"
#include "game/core/object_heap.h"
#include "ra/ai/scenario.h"
#include "ra/ai/team.h"
#include "ra/ai/waypoint_name.h"
#include "ra/assets/palettes.h"
#include "ra/core/config.h"
#include "ra/core/const.h"
#include "ra/core/defines.h"
#include "ra/core/object_heaps.h"
#include "ra/core/random_pick.h"
#include "ra/core/target.h"
#include "ra/data/text_ids.h"
#include "ra/map/world.h"
#include "ra/objects/house.h"
#include "ra/objects/mission.h"
#include "ra/objects/type.h"
#include "ra/ui/color_scheme.h"
#include "ra/ui/dialog.h"

namespace ra {

using enum engine::window::KeyNumber;

namespace {

// The scenario INI section that lists the team types.
constexpr const char* kIniSection = "TeamTypes";

// The scenario editor's name for each team mission.
constexpr base::EnumArray<TeamMissionType, const char*> kMissionNames = {
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

// Returns the infantry, unit, aircraft or vessel type called `name`, or
// nullptr if there is none.
const TechnoTypeClass* MemberTypeFromName(const std::string_view name) {
  if (const InfantryType infantry = InfantryTypeClass::From_Name(name);
      infantry != INFANTRY_NONE) {
    return &InfantryTypeClass::As_Reference(infantry);
  }
  if (const UnitType unit = UnitTypeClass::From_Name(name); unit != UNIT_NONE) {
    return &UnitTypeClass::As_Reference(unit);
  }
  if (const AircraftType aircraft = AircraftTypeClass::From_Name(name);
      aircraft != AIRCRAFT_NONE) {
    return &AircraftTypeClass::As_Reference(aircraft);
  }
  if (const VesselType vessel = VesselTypeClass::From_Name(name);
      vessel != VESSEL_NONE) {
    return &VesselTypeClass::As_Reference(vessel);
  }
  return nullptr;
}

// Draws `text` as a list box entry in the given rectangle, highlighted when
// `selected`. `tab_stops` are the pixel columns its tabs advance to.
void DrawListEntry(PixelView& view, const std::string& text,
                   const std::span<const int> tab_stops, const int x,
                   const int y, const int width, const int height,
                   const bool selected, const FontType font,
                   TextPrintType flags) {
  RemapControlType* scheme = Get_Color_Scheme();
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
  Conquer_Clip_Text_Print(view, text.c_str(), x, y, scheme, kTBlack, font,
                          flags, width, tab_stops);
}

}  // namespace

void TeamType::Draw_It(PixelView& view, int /*unused*/, int x, int y, int width,
                       int height, bool selected, FontType font,
                       TextPrintType flags) const {
  if constexpr (config::kCheatKeysEnabled || config::kScenarioEditorEnabled) {
    // Pixel columns of Description()'s fields after the name: house, origin,
    // mission count, members.
    static constexpr int kTabStops[] = {35, 60, 80, 100};
    DrawListEntry(view, Description(), kTabStops, x, y, width, height, selected,
                  font, flags);
  }
}

TeamType::TeamType()
    : AbstractTypeClass(RTTI_TEAMTYPE,
                        TheObjectHeaps().team_type().SlotIndexOf(this),
                        TXT_NONE, "") {}

TeamType::~TeamType() = default;

void TeamType::FreeAll() { TheObjectHeaps().team_type().FreeAll(); }

const char* TeamType::MissionName(TeamMissionType order) {
  return kMissionNames.at(order);
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

TeamClass* TeamType::CreateTeam() {
  if (TheWorld().scenario_init() || std::cmp_less(Number, MaxAllowed)) {
    return base::NewPooled<TeamClass>(this, HouseClass::As_Pointer(House));
  }
  return nullptr;
}

void TeamType::DestroyTeams() const {
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

TeamType* TeamType::SuggestNewTeam(HouseClass* house, bool alerted) {
  // Only the first 20 candidates in heap order take part in the draw.
  TeamType* choices[20];
  int choice_count = 0;

  for (int index = 0; index < TheObjectHeaps().team_type().Count() &&
                      choice_count < std::ssize(choices);
       index++) {
    TeamType* candidate = TheObjectHeaps().team_type().ActiveAt(index);

    // A team type on the wrong side of the alert is treated as full.
    const int limit =
        alerted == candidate->IsAutocreate ? candidate->MaxAllowed : 0;
    if (candidate->House == house->Class->House && candidate->Number < limit) {
      base::At(choices, choice_count++) = candidate;
    }
  }

  if (choice_count > 0) {
    return base::At(choices, RandomPick(0, choice_count - 1));
  }
  return nullptr;
}

TeamType* TeamType::FromName(const std::string_view name) {
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

NeedType TeamMissionNeeds(TeamMissionType mission) {
  switch (mission) {
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
  // Pixel columns of the mission's name and of a second field
  // Description() never writes.
  static constexpr int kTabStops[] = {13, 40};
  DrawListEntry(view, Description(index), kTabStops, x, y, width, height,
                selected, font, flags);
}

std::string TeamType::MemberDescription() const {
  std::string text;
  if constexpr (config::kCheatKeysEnabled || config::kScenarioEditorEnabled) {
    // "TYPE:count" for each kind of member, comma separated.
    for (int index = 0; index < ClassCount; index++) {
      const TeamMember& member = base::At(Members, index);
      absl::StrAppendFormat(&text, "%s%s:%d", index > 0 ? "," : "",
                            member.type->IniName, member.quantity);
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
    const std::string origin_letters =
        Origin > -1 ? WaypointLetters(Origin) : "";

    // A team read from a scenario that names no house for it has none.
    const char* const house_suffix =
        House == HOUSE_NONE ? "" : HouseTypeClass::As_Reference(House).Suffix;
    return absl::StrFormat("%s\t%s\t%c%s\t%d\t%s", IniName, house_suffix,
                           IsAutocreate ? '*' : ' ', origin_letters,
                           MissionCount, MemberDescription());
  }
  return "";
}

std::string TeamMission::Description(int index) const {
  if constexpr (config::kCheatKeysEnabled || config::kScenarioEditorEnabled) {
    std::string text =
        absl::StrFormat("%d\t%s", index, TeamType::MissionName(Mission));

    switch (TeamMissionNeeds(Mission)) {
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
        text += WaypointLetters(Data.Value);
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

void TeamType::ReadIni(engine::file::IniFile& ini) {
  for (const auto& [entry, value] : ini.Entries(kIniSection)) {
    auto* team_type = base::NewPooled<TeamType>();
    if (team_type != nullptr) {
      team_type->ReadIniEntry(entry, value);
    }
  }
}

void TeamType::ReadIniEntry(std::string_view name, std::string_view entry) {
  DCHECK_HEAP_SLOT(TheObjectHeaps().team_type(), this);

  Set_Name(name);

  base::Tokenizer tokens(entry);
  // TODO: the house and the mission numbers below are taken from the file
  // unchecked; an out-of-range one CHECK-fails later, where the team type is
  // used.
  House = static_cast<HousesType>(base::ParseIntegerOr<int>(tokens.Next(), 0));

  // The scenario's NewINIFormat: 0 and 1 list each option and end the entry
  // with the trigger; the later ones pack the options into one bit field and
  // put the trigger after the origin.
  const int format = TheWorld().new_ini_format();
  const bool lists_options = format == 0 || format == 1;
  if (lists_options) {
    IsRoundAbout = base::ParseIntegerOr<int>(tokens.Next(), 0) != 0;
    IsSuicide = base::ParseIntegerOr<int>(tokens.Next(), 0) != 0;
    IsAutocreate = base::ParseIntegerOr<int>(tokens.Next(), 0) != 0;
    IsPrebuilt = base::ParseIntegerOr<int>(tokens.Next(), 0) != 0;
    IsReinforcable = base::ParseIntegerOr<int>(tokens.Next(), 0) != 0;
  } else {
    const auto flags = base::ParseIntegerOr<uint32_t>(tokens.Next(), 0);
    IsRoundAbout = (flags & 0x0001) != 0;
    IsSuicide = (flags & 0x0002) != 0;
    IsAutocreate = (flags & 0x0004) != 0;
    IsPrebuilt = (flags & 0x0008) != 0;
    IsReinforcable = (flags & 0x0010) != 0;
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

  if (lists_options) {
    tokens.Next();  // Old-format trigger field, unused.
  } else {
    // The trigger types are read after the team types; the slot is where
    // that trigger type will be. -1 gives nullptr.
    Trigger = TheObjectHeaps().trigger_type().SlotOrNull(
        base::ParseIntegerOr<int>(tokens.Next(), 0));
  }

  // The members: a count, then that many Type:Quantity pairs.
  const int listed_count = base::ParseIntegerOr<int>(tokens.Next(), -1);
  ClassCount = 0;
  if (listed_count < 0 || listed_count > kMaxMemberTypes) {
    MissionCount = 0;
    return;
  }
  for (int listed = 0; listed < listed_count; listed++) {
    const std::string_view type_name = tokens.Next(",:");
    const std::string_view quantity_text = tokens.Next(",:");
    if (type_name.empty() || quantity_text.empty()) {
      ClassCount = 0;
      MissionCount = 0;
      return;
    }

    // An unknown type (a unit this build does not have) is dropped and the
    // next member takes its place in Members.
    const TechnoTypeClass* const member_type = MemberTypeFromName(type_name);
    if (member_type != nullptr) {
      TeamMember& member = base::At(Members, ClassCount++);
      member.type = member_type;
      member.quantity = base::ParseIntegerOr<int>(quantity_text, 0);
    }
  }

  // The script: a count, then that many Mission:Argument pairs.
  MissionCount = base::ParseIntegerOr<int>(tokens.Next(), -1);
  if (MissionCount < 0 || MissionCount > kMaxMissions) {
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

  if (format < 2) {
    // The older formats put the trigger last.
    Trigger = TheObjectHeaps().trigger_type().SlotOrNull(
        base::ParseIntegerOr<int>(tokens.Next(), 0));
  }
}

void TeamType::WriteIni(engine::file::IniFile& ini) {
  ini.Clear(kIniSection);

  for (int index = 0; index < TheObjectHeaps().team_type().Count(); index++) {
    const TeamType* team_type = TheObjectHeaps().team_type().ActiveAt(index);
    ini.PutString(kIniSection, team_type->IniName, team_type->ToIniEntry());
  }
}

std::string TeamType::ToIniEntry() const {
  uint32_t flags = 0;
  flags |= IsRoundAbout ? 0x0001U : 0U;
  flags |= IsSuicide ? 0x0002U : 0U;
  flags |= IsAutocreate ? 0x0004U : 0U;
  flags |= IsPrebuilt ? 0x0008U : 0U;
  flags |= IsReinforcable ? 0x0010U : 0U;

  std::string entry = absl::StrFormat(
      "%d,%d,%d,%d,%d,%d,%d", House, flags, RecruitPriority, InitNum,
      MaxAllowed, Origin, TheObjectHeaps().trigger_type().PositionOf(Trigger));

  // The members: a count, then Type:Quantity for each.
  absl::StrAppendFormat(&entry, ",%d", ClassCount);
  for (int i = 0; i < ClassCount; i++) {
    absl::StrAppendFormat(&entry, ",%s:%d", base::At(Members, i).type->IniName,
                          base::At(Members, i).quantity);
  }

  // The script: a count, then Mission:Argument for each, both as numbers.
  absl::StrAppendFormat(&entry, ",%d", MissionCount);
  for (int i = 0; i < MissionCount; i++) {
    absl::StrAppendFormat(&entry, ",%d:%d", base::At(MissionList, i).Mission,
                          base::At(MissionList, i).Data.Value);
  }
  return entry;
}

}  // namespace ra
