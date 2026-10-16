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

#include "ra/ai/team_type.h"

#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "absl/log/check.h"
#include "absl/strings/match.h"
#include "absl/strings/str_format.h"
#include "base/array.h"
#include "base/enum_array.h"
#include "base/pooled.h"
#include "base/strings/number_parse.h"
#include "base/strings/tokenizer.h"
#include "engine/file/ini_file.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/window/keyboard.h"
#include "game/core/object_heap.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/ai/scenario.h"
#include "ra/ai/team.h"
#include "ra/ai/waypoint_name.h"
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
#include "ra/ui/ui_skin.h"

namespace ra {

using enum engine::window::KeyNumber;

namespace {

// The scenario INI section that lists the team types.
constexpr const auto* kIniSection = "TeamTypes";

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

}  // namespace

std::string TeamMission::Description(const int index) const {
  if constexpr (config::kCheatKeysEnabled || config::kScenarioEditorEnabled) {
    std::string text =
        absl::StrFormat("%d\t%s", index, TeamType::MissionName(mission_));

    switch (TeamMissionNeeds(mission_)) {
      case NeedType::kMission:
        text += MissionClass::Mission_Name(unit_mission());
        break;

      case NeedType::kFormation:
        text += FormationName.at(formation());
        break;

      case NeedType::kNumber:
        absl::StrAppendFormat(&text, "%d", value());
        break;

      case NeedType::kHexNumber:
        absl::StrAppendFormat(&text, "%x", static_cast<unsigned int>(value()));
        break;

      case NeedType::kQuarry:
        text += QuarryName.at(quarry());
        break;

      case NeedType::kWaypoint:
        text += WaypointLetters(value());
        break;
      case NeedType::kNone:
      case NeedType::kTheme:
      case NeedType::kMovie:
      case NeedType::kSound:
      case NeedType::kSpeech:
      case NeedType::kInfantry:
      case NeedType::kUnit:
      case NeedType::kAircraft:
      case NeedType::kStructure:
      case NeedType::kTrigger:
      case NeedType::kTeam:
      case NeedType::kHouse:
      case NeedType::kTime:
      case NeedType::kBool:
      case NeedType::kSpecial:
      default:
        break;
    }

    return text;
  }
  return "";
}

void TeamMission::Draw_It(PixelView& view, const int index, const int x,
                          const int y, const int width, const int height,
                          const bool selected, const FontType font,
                          const TextPrintType flags) const {
  // Description() writes no tab, so the stops do not matter.
  UiSkin::Draw_List_Entry(view, Description(index).c_str(), x, y, width, height,
                          font, flags, selected, UiSkin::kEditorListTabStops);
}

TeamType::TeamType()
    : AbstractTypeClass(RTTI_TEAMTYPE,
                        TheObjectHeaps().team_type().SlotIndexOf(this),
                        TXT_NONE, "") {}

TeamType::~TeamType() = default;

void* TeamType::operator new(std::size_t /*unused*/) noexcept {
  void* ptr = TheObjectHeaps().team_type().Allocate();
  if (ptr) {
    static_cast<TeamType*>(ptr)->is_active_ = true;
  }
  return ptr;
}

void TeamType::operator delete(void* ptr) {
  if (ptr) {
    static_cast<TeamType*>(ptr)->is_active_ = false;
  }
  TheObjectHeaps().team_type().Free(static_cast<TeamType*>(ptr));
}

void TeamType::FreeAll() { TheObjectHeaps().team_type().FreeAll(); }

std::string TeamType::ToIniEntry() const {
  uint32_t flags = 0;
  flags |= is_roundabout_ ? 0x0001U : 0U;
  flags |= is_suicide_ ? 0x0002U : 0U;
  flags |= is_autocreate_ ? 0x0004U : 0U;
  flags |= is_prebuilt_ ? 0x0008U : 0U;
  flags |= is_reinforceable_ ? 0x0010U : 0U;

  std::string entry =
      absl::StrFormat("%d,%d,%d,%d,%d,%d,%d", house_, flags, recruit_priority_,
                      initial_count_, max_teams_, origin_,
                      TheObjectHeaps().trigger_type().PositionOf(trigger_));

  // The members: a count, then Type:Quantity for each.
  absl::StrAppendFormat(&entry, ",%d", member_count_);
  for (int i = 0; i < member_count_; i++) {
    absl::StrAppendFormat(&entry, ",%s:%d", base::At(members_, i).type->IniName,
                          base::At(members_, i).quantity);
  }

  // The script: a count, then Mission:Argument for each, both as numbers.
  absl::StrAppendFormat(&entry, ",%d", mission_count_);
  for (int i = 0; i < mission_count_; i++) {
    absl::StrAppendFormat(&entry, ",%d:%d", base::At(missions_, i).mission(),
                          base::At(missions_, i).value());
  }
  return entry;
}

void TeamType::ReadIni(const engine::file::IniFile& ini) {
  // The scenario refers to a team type by the place of its entry, which is
  // its slot only while every earlier entry holds one. A rejected entry
  // therefore keeps its slot until all are read; freed at once, it would
  // move the later team types down a slot.
  std::vector<TeamType*> rejected;
  for (const auto& [entry, value] : ini.Entries(kIniSection)) {
    auto* team_type = base::NewPooled<TeamType>();
    if (team_type != nullptr && !team_type->ReadIniEntry(entry, value)) {
      rejected.push_back(team_type);
    }
  }
  for (TeamType* const team_type : rejected) {
    base::DeletePooled(team_type);
  }
}

bool TeamType::ReadIniEntry(const std::string_view name,
                            const std::string_view entry) {
  DCHECK_HEAP_SLOT(TheObjectHeaps().team_type(), this);

  Set_Name(name);

  base::Tokenizer tokens(entry);
  const auto house =
      static_cast<HousesType>(base::ParseIntegerOr<int>(tokens.Next(), 0));
  if (!magic_enum::enum_contains(house)) {
    return false;
  }
  house_ = house;

  // The scenario's NewINIFormat: 0 and 1 list each option and end the entry
  // with the trigger; the later ones pack the options into one bit field and
  // put the trigger after the origin.
  const int format = TheWorld().new_ini_format();
  const bool lists_options = format == 0 || format == 1;
  if (lists_options) {
    is_roundabout_ = base::ParseIntegerOr<int>(tokens.Next(), 0) != 0;
    is_suicide_ = base::ParseIntegerOr<int>(tokens.Next(), 0) != 0;
    is_autocreate_ = base::ParseIntegerOr<int>(tokens.Next(), 0) != 0;
    is_prebuilt_ = base::ParseIntegerOr<int>(tokens.Next(), 0) != 0;
    is_reinforceable_ = base::ParseIntegerOr<int>(tokens.Next(), 0) != 0;
  } else {
    const auto flags = base::ParseIntegerOr<uint32_t>(tokens.Next(), 0);
    is_roundabout_ = (flags & 0x0001) != 0;
    is_suicide_ = (flags & 0x0002) != 0;
    is_autocreate_ = (flags & 0x0004) != 0;
    is_prebuilt_ = (flags & 0x0008) != 0;
    is_reinforceable_ = (flags & 0x0010) != 0;
  }

  recruit_priority_ = base::ParseIntegerOr<int>(tokens.Next(), 0);
  set_initial_count(base::ParseIntegerOr<int>(tokens.Next(), 0));
  set_max_teams(base::ParseIntegerOr<int>(tokens.Next(), 0));
  set_origin(base::ParseIntegerOr<int>(tokens.Next(), 0));

  if (lists_options) {
    tokens.Next();  // Old-format trigger field, unused.
  } else {
    // The trigger types are read after the team types; the slot is where
    // that trigger type will be. -1 gives nullptr.
    trigger_ = TheObjectHeaps().trigger_type().SlotOrNull(
        base::ParseIntegerOr<int>(tokens.Next(), 0));
  }

  // The members: a count, then that many Type:Quantity pairs.
  const int listed_count = base::ParseIntegerOr<int>(tokens.Next(), -1);
  member_count_ = 0;
  if (listed_count < 0 || listed_count > kMaxMemberTypes) {
    mission_count_ = 0;
    return true;
  }
  for (int listed = 0; listed < listed_count; listed++) {
    const std::string_view type_name = tokens.Next(",:");
    const std::string_view quantity_text = tokens.Next(",:");
    if (type_name.empty() || quantity_text.empty()) {
      member_count_ = 0;
      mission_count_ = 0;
      return true;
    }

    // An unknown type (a unit this build does not have) is dropped and the
    // next member takes its place in Members.
    const TechnoTypeClass* const member_type = MemberTypeFromName(type_name);
    if (member_type != nullptr) {
      base::At(members_, member_count_++) = {
          .quantity = base::ParseIntegerOr<int>(quantity_text, 0),
          .type = member_type};
    }
  }

  // The script: a count, then that many Mission:Argument pairs.
  mission_count_ = base::ParseIntegerOr<int>(tokens.Next(), -1);
  if (mission_count_ < 0 || mission_count_ > kMaxMissions) {
    member_count_ = 0;
    mission_count_ = 0;
    return true;
  }
  for (int index = 0; index < mission_count_; index++) {
    const auto mission = static_cast<TeamMissionType>(
        base::ParseIntegerOr<int>(tokens.Next(",:"), 0));
    if (!magic_enum::enum_contains(mission)) {
      return false;
    }
    base::At(missions_, index).set_mission(mission);
    base::At(missions_, index)
        .set_value(base::ParseIntegerOr<int>(tokens.Next(",:"), 0));
  }

  if (format < 2) {
    // The older formats put the trigger last.
    trigger_ = TheObjectHeaps().trigger_type().SlotOrNull(
        base::ParseIntegerOr<int>(tokens.Next(), 0));
  }
  return true;
}

void TeamType::WriteIni(engine::file::IniFile& ini) {
  ini.Clear(kIniSection);

  for (int index = 0; index < TheObjectHeaps().team_type().Count(); index++) {
    const TeamType* team_type = TheObjectHeaps().team_type().ActiveAt(index);
    ini.PutString(kIniSection, team_type->IniName, team_type->ToIniEntry());
  }
}

TeamClass* TeamType::CreateTeam() {
  // A scenario's entry can leave a type with no members (none listed, or
  // only types this build does not have); a team needs one.
  if (member_count_ == 0) {
    return nullptr;
  }
  if (TheWorld().scenario_init() || CountTeams() < max_teams_) {
    return base::NewPooled<TeamClass>(this, HouseClass::As_Pointer(house_));
  }
  return nullptr;
}

int TeamType::CountTeams() const {
  int count = 0;
  for (int index = 0; index < TheObjectHeaps().team().Count(); index++) {
    if (TheObjectHeaps().team().ActiveAt(index)->Class == this) {
      count++;
    }
  }
  return count;
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

void TeamType::Detach(const Target target) {
  if (target.kind() == RTTI_TRIGGERTYPE && trigger_ != nullptr &&
      trigger_ == target.AsTriggerType()) {
    trigger_ = nullptr;
  }
}

void TeamType::Draw_It(PixelView& view, int /*unused*/, const int x,
                       const int y, const int width, const int height,
                       const bool selected, const FontType font,
                       const TextPrintType flags) const {
  if constexpr (config::kCheatKeysEnabled || config::kScenarioEditorEnabled) {
    // Pixel columns of Description()'s fields after the name: house, origin,
    // mission count, members.
    static constexpr int kTabStops[] = {35, 60, 80, 100};
    UiSkin::Draw_List_Entry(view, Description().c_str(), x, y, width, height,
                            font, flags, selected, kTabStops);
  }
}

const char* TeamType::MissionName(const TeamMissionType mission) {
  return kMissionNames.at(mission);
}

TeamType* TeamType::SuggestNewTeam(const HouseClass* const house,
                                   const bool alerted) {
  std::vector<TeamType*> choices;
  for (int index = 0; index < TheObjectHeaps().team_type().Count(); index++) {
    TeamType* candidate = TheObjectHeaps().team_type().ActiveAt(index);

    // A team type on the wrong side of the alert is treated as full.
    const int limit =
        alerted == candidate->is_autocreate() ? candidate->max_teams() : 0;
    if (candidate->house() == house->Class->House &&
        candidate->CountTeams() < limit) {
      choices.push_back(candidate);
    }
  }

  if (choices.empty()) {
    return nullptr;
  }
  return base::At(choices, RandomPick(0, static_cast<int>(choices.size()) - 1));
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

std::string TeamType::MemberDescription() const {
  std::string text;
  if constexpr (config::kCheatKeysEnabled || config::kScenarioEditorEnabled) {
    // "TYPE:count" for each kind of member, comma separated.
    for (int index = 0; index < member_count_; index++) {
      const auto& [quantity, type] = base::At(members_, index);
      absl::StrAppendFormat(&text, "%s%s:%d", index > 0 ? "," : "",
                            type->IniName, quantity);
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
        origin_ > -1 ? WaypointLetters(origin_) : "";

    // A team read from a scenario that names no house for it has none.
    const char* const house_suffix =
        house_ == HOUSE_NONE ? "" : HouseTypeClass::As_Reference(house_).Suffix;
    return absl::StrFormat("%s\t%s\t%c%s\t%d\t%s", IniName, house_suffix,
                           is_autocreate_ ? '*' : ' ', origin_letters,
                           mission_count_, MemberDescription());
  }
  return "";
}

void TeamType::set_origin(const WAYPOINT waypoint) {
  origin_ = ScenarioClass::IsWaypoint(waypoint) ? waypoint : -1;
}

void TeamType::AddMission(const TeamMission& mission) {
  CHECK(mission_count_ < kMaxMissions);
  base::At(missions_, mission_count_++) = mission;
}

void TeamType::AddMember(const TeamMember& member) {
  CHECK(member_count_ < kMaxMemberTypes);
  base::At(members_, member_count_++) = member;
}

NeedType TeamMissionNeeds(const TeamMissionType mission) {
  switch (mission) {
    case TeamMissionType::kFormation:
      return NeedType::kFormation;

    case TeamMissionType::kAttack:
      return NeedType::kQuarry;

    // A cell number, which the editor shows and reads in hex.
    case TeamMissionType::kMoveCell:
      return NeedType::kHexNumber;

    case TeamMissionType::kSetGlobal:
    case TeamMissionType::kGuard:
    case TeamMissionType::kLoop:
      return NeedType::kNumber;

    case TeamMissionType::kPatrol:
    case TeamMissionType::kMove:
    case TeamMissionType::kAttackWaypoint:
    case TeamMissionType::kSpy:
      return NeedType::kWaypoint;

    case TeamMissionType::kDo:
      return NeedType::kMission;

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
  return NeedType::kNone;
}
}  // namespace ra
