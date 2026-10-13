/*
 **	Command & Conquer(tm)
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
// computer to build, and creating and destroying their teams.

#include "td/ai/teamtype.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>
#include <utility>

#include "absl/log/check.h"
#include "absl/strings/match.h"
#include "absl/strings/str_format.h"
#include "base/array.h"
#include "base/enum_array.h"
#include "base/numeric.h"
#include "base/pooled.h"
#include "base/strings/number_parse.h"
#include "base/strings/tokenizer.h"
#include "engine/file/ini_file.h"
#include "game/core/object_heap.h"
#include "magic_enum/magic_enum.hpp"
#include "td/ai/team.h"
#include "td/ai/trigger.h"
#include "td/core/config.h"
#include "td/core/defines.h"
#include "td/core/object_heaps.h"
#include "td/map/world.h"
#include "td/objects/house.h"
#include "td/objects/type.h"
#include "td/startup/conquer.h"

namespace td {

namespace {

// The name of each team mission in a scenario INI and in the scenario editor.
constexpr base::EnumArray<TeamMissionType, const char*> kMissionNames = {
    "Attack Base", "Attack Units", "Attack Civil.", "Rampage",
    "Defend Base", "Move",         "Move to Cell",  "Retreat",
    "Guard",       "Loop",         "Attack Tarcom", "Unload",
};

// Returns the infantry, unit or aircraft type called `name`, or nullptr if
// there is none. A name that several kinds share goes to the aircraft before
// the unit and to the unit before the infantry.
const TechnoTypeClass* MemberTypeFromName(const std::string_view name) {
  if (const AircraftType aircraft = AircraftTypeClass::From_Name(name);
      aircraft != AIRCRAFT_NONE) {
    return &AircraftTypeClass::As_Reference(aircraft);
  }
  if (const UnitType unit = UnitTypeClass::From_Name(name); unit != UNIT_NONE) {
    return &UnitTypeClass::As_Reference(unit);
  }
  if (const InfantryType infantry = InfantryTypeClass::From_Name(name);
      infantry != INFANTRY_NONE) {
    return &InfantryTypeClass::As_Reference(infantry);
  }
  return nullptr;
}

}  // namespace

TeamType::~TeamType() = default;

void TeamType::FreeAll() { TheObjectHeaps().team_type().FreeAll(); }

void TeamType::ReadIni(const engine::file::IniFile& ini) {
  for (const auto& [name, entry] : ini.Entries(kIniSection)) {
    auto* team_type = base::NewPooled<TeamType>();
    // The heap is full: this entry and the rest are dropped.
    if (team_type == nullptr) {
      break;
    }
    team_type->ReadIniEntry(name, entry);
  }

  // A scenario in the old format has no [TeamTypes] section.
  if (TheObjectHeaps().team_type().Count() == 0) {
    ReadOldIni(ini);
  }
}

void TeamType::ReadIniEntry(std::string_view name, std::string_view entry) {
  Validate();

  Set_Name(name);

  base::Tokenizer tokens(entry);
  ReadLeadingFields(tokens);
  fear_ = StoredCount(base::ParseIntegerOr<int>(tokens.Next(), 0));

  // The members: a count, then that many Class:Num pairs.
  const int listed_count = base::ParseIntegerOr<int>(tokens.Next(), -1);
  member_count_ = 0;
  if (listed_count < 0 || listed_count > kMaxMemberTypes) {
    mission_count_ = 0;
    return;
  }
  for (int listed = 0; listed < listed_count; listed++) {
    const std::string_view type_name = tokens.Next(",:");
    const std::string_view quantity_text = tokens.Next(",:");
    if (type_name.empty() || quantity_text.empty()) {
      member_count_ = 0;
      mission_count_ = 0;
      return;
    }

    // An unknown type (a unit this build does not have) is dropped and the
    // next member takes its place.
    const TechnoTypeClass* const member_type = MemberTypeFromName(type_name);
    if (member_type != nullptr) {
      base::At(members_, member_count_++) = {
          .quantity =
              static_cast<uint8_t>(base::ParseIntegerOr<int>(quantity_text, 0)),
          .type = member_type};
    }
  }

  // The script: a count, then that many Mission:Arg pairs.
  mission_count_ = base::ParseIntegerOr<int>(tokens.Next(), -1);
  if (mission_count_ < 0 || mission_count_ > kMaxMissions) {
    member_count_ = 0;
    mission_count_ = 0;
    return;
  }

  for (int line = 0; line < mission_count_; line++) {
    const std::string_view mission_name = tokens.Next(",:");
    const std::string_view argument_text = tokens.Next(",:");
    if (mission_name.empty() || argument_text.empty()) {
      member_count_ = 0;
      mission_count_ = 0;
      return;
    }
    base::At(missions_, line) = {
        .mission = MissionFromName(mission_name),
        .argument = base::ParseIntegerOr<int>(argument_text, 0)};
  }

  // Older scenarios end the entry here, which leaves both options on.
  if (const std::string_view reinforceable = tokens.Next();
      !reinforceable.empty()) {
    is_reinforceable_ = base::ParseIntegerOr<int>(reinforceable, 0) != 0;
  }
  if (const std::string_view prebuilt = tokens.Next(); !prebuilt.empty()) {
    is_prebuilt_ = base::ParseIntegerOr<int>(prebuilt, 0) != 0;
  }
}

std::string TeamType::ToIniEntry() const {
  // By its type: the scenario need not have the house itself.
  const std::string_view house_name =
      house_ == HOUSE_NONE ? "None"
                           : HouseTypeClass::As_Reference(house_).IniName;

  std::string entry = absl::StrFormat(
      "%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d", house_name, is_roundabout_,
      is_learning_, is_suicide_, is_autocreate_, is_mercenary_,
      recruit_priority_, max_teams_, initial_count_, fear_, member_count_);

  // The members: Class:Num for each.
  for (int i = 0; std::cmp_less(i, member_count_); i++) {
    absl::StrAppendFormat(&entry, ",%s:%d", base::At(members_, i).type->IniName,
                          base::At(members_, i).quantity);
  }

  // The script: a count, then Mission:Arg for each, the mission by name.
  absl::StrAppendFormat(&entry, ",%d", mission_count_);
  for (int i = 0; i < mission_count_; i++) {
    absl::StrAppendFormat(&entry, ",%s:%d",
                          MissionName(base::At(missions_, i).mission),
                          base::At(missions_, i).argument);
  }

  absl::StrAppendFormat(&entry, ",%d,%d", is_reinforceable_, is_prebuilt_);
  return entry;
}

void TeamType::WriteIni(engine::file::IniFile& ini, bool refresh) {
  // The team types are written in the current format only.
  ini.Clear("Teams");

  if (refresh) {
    ini.Clear(kIniSection);
  }

  for (int index = 0; index < TheObjectHeaps().team_type().Count(); index++) {
    const TeamType* team_type = TheObjectHeaps().team_type().ActiveAt(index);
    ini.PutString(kIniSection, team_type->IniName, team_type->ToIniEntry());
  }
}

void TeamType::ReadOldIni(const engine::file::IniFile& ini) {
  for (const auto& [name, entry] : ini.Entries("Teams")) {
    auto* team_type = base::NewPooled<TeamType>();
    // The heap is full: this entry and the rest are dropped.
    if (team_type == nullptr) {
      break;
    }
    team_type->Set_Name(name);

    base::Tokenizer tokens(entry);
    team_type->ReadLeadingFields(tokens);
    tokens.Next();  // The mission's name, unused.

    // The members: Class:Num pairs up to the end of the entry, of which the
    // first kMaxMemberTypes known types are kept.
    std::string_view type_name = tokens.Next(",:");
    std::string_view quantity_text = tokens.Next(",:");
    while (!type_name.empty() && !quantity_text.empty() &&
           team_type->member_count_ < kMaxMemberTypes) {
      const TechnoTypeClass* const member_type = MemberTypeFromName(type_name);
      if (member_type != nullptr) {
        team_type->AddMember({.quantity = static_cast<uint8_t>(
                                  base::ParseIntegerOr<int>(quantity_text, 0)),
                              .type = member_type});
      }

      type_name = tokens.Next(",:");
      quantity_text = tokens.Next(",:");
    }
  }
}

TeamType* TeamType::FromName(std::string_view name) {
  if (name.empty()) {
    return nullptr;
  }

  for (int i = 0; i < TheObjectHeaps().team_type().Count(); i++) {
    if (absl::EqualsIgnoreCase(
            name, TheObjectHeaps().team_type().ActiveAt(i)->IniName)) {
      return TheObjectHeaps().team_type().ActiveAt(i);
    }
  }

  return nullptr;
}

void TeamType::Delete() {
  Validate();

  for (int i = 0; i < TheObjectHeaps().trigger().Count(); i++) {
    TriggerClass* trigger = TheObjectHeaps().trigger().ActiveAt(i);
    if (trigger->Team == this) {
      trigger->Team = nullptr;
    }
  }

  delete this;
}

TeamClass* TeamType::CreateTeam() const {
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

const char* TeamType::MissionName(const TeamMissionType mission) {
  if (!magic_enum::enum_contains(mission)) {
    return "None";
  }
  return kMissionNames.at(mission);
}

TeamMissionType TeamType::MissionFromName(std::string_view name) {
  if (!name.empty()) {
    for (const TeamMissionType order :
         magic_enum::enum_values<TeamMissionType>()) {
      if (absl::EqualsIgnoreCase(kMissionNames.at(order), name)) {
        return order;
      }
    }
  }

  return TeamMissionType::kNone;
}

const TeamType* TeamType::SuggestNewTeam(HouseClass* house, uint64_t unit_types,
                                         uint64_t infantry_types,
                                         bool alerted) {
  const TeamType* best = nullptr;
  int best_value = 0;

  for (int index = 0; index < TheObjectHeaps().team_type().Count(); index++) {
    const TeamType* candidate = TheObjectHeaps().team_type().ActiveAt(index);

    if (candidate->house_ == house->Class->House &&
        candidate->CountTeams() < (alerted || !candidate->is_autocreate_
                                       ? candidate->max_teams_
                                       : 0)) {
      // The unit and infantry types this team type wants, as bit masks.
      // Aircraft do not count.
      uint64_t units_wanted = 0;
      uint64_t infantry_wanted = 0;
      for (const TeamMember& member : candidate->members()) {
        if (const auto* const infantry =
                dynamic_cast<const InfantryTypeClass*>(member.type)) {
          infantry_wanted |= base::Bit<uint64_t>(infantry->Type);
        } else if (const auto* const unit =
                       dynamic_cast<const UnitTypeClass*>(member.type)) {
          units_wanted |= base::Bit<uint64_t>(unit->Type);
        }
      }

      // A team type the house has no members for is still a candidate, at
      // half its priority.
      int value = 0;
      if (infantry_wanted & infantry_types || units_wanted & unit_types) {
        value = candidate->recruit_priority_;
      } else {
        value = candidate->recruit_priority_ / 2;
      }

      // The first of equals wins.
      if (!best || best_value < value) {
        best_value = value;
        best = candidate;
      }
    }
  }

  return best;
}

Target TeamType::As_Target() const {
  Validate();
  return Target::Make(KIND_TEAMTYPE,
                      TheObjectHeaps().team_type().SlotIndexOf(this));
}

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

void TeamType::Validate() const {
  if constexpr (config::kCheatKeysEnabled) {
    if (!TheObjectHeaps().team_type().Owns(this)) {
      Validate_Error("TEAMTYPE");
    }
  }
}

void TeamType::AddMission(const TeamMission& mission) {
  CHECK(mission_count_ < kMaxMissions);
  base::At(missions_, mission_count_++) = mission;
}

void TeamType::AddMember(const TeamMember& member) {
  CHECK(member_count_ < kMaxMemberTypes);
  base::At(members_, member_count_++) = member;
}

void TeamType::ReadLeadingFields(base::Tokenizer& tokens) {
  // An unknown house name gives HOUSE_NONE.
  house_ = HouseTypeClass::From_Name(tokens.Next());
  is_roundabout_ = base::ParseIntegerOr<int>(tokens.Next(), 0) != 0;
  is_learning_ = base::ParseIntegerOr<int>(tokens.Next(), 0) != 0;
  is_suicide_ = base::ParseIntegerOr<int>(tokens.Next(), 0) != 0;
  // The fifth field was once "Spy".
  is_autocreate_ = base::ParseIntegerOr<int>(tokens.Next(), 0) != 0;
  is_mercenary_ = base::ParseIntegerOr<int>(tokens.Next(), 0) != 0;
  recruit_priority_ = base::ParseIntegerOr<int>(tokens.Next(), 0);
  set_max_teams(base::ParseIntegerOr<int>(tokens.Next(), 0));
  set_initial_count(base::ParseIntegerOr<int>(tokens.Next(), 0));
}

}  // namespace td
