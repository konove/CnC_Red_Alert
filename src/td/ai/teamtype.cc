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

base::EnumArray<TeamMissionType, const char*, static_cast<int>(TMISSION_COUNT)>
    TeamTypeClass::TMissions = {
        "Attack Base",
        "Attack Units",
        "Attack Civil.",
        "Rampage",
        "Defend Base",
        "Move",
        "Move to Cell",
        "Retreat",
        "Guard",
        "Loop",
        "Attack Tarcom",
        "Unload",
};

TeamTypeClass::~TeamTypeClass() = default;

int TeamTypeClass::Validate() const {
  if constexpr (config::kCheatKeysEnabled) {
    const int num = TheObjectHeaps().team_type().SlotIndexOf(this);
    if (num < 0 || num >= kTeamTypeMax) {
      Validate_Error("TEAMTYPE");
    }
    return 1;
  } else {
    return 1;
  }
}

void TeamTypeClass::Init() { TheObjectHeaps().team_type().FreeAll(); }

void TeamTypeClass::Read_INI(const engine::file::IniFile& ini) {
  for (const auto& [name, entry] : ini.Entries(INI_Name())) {
    auto* team = base::NewPooled<TeamTypeClass>();
    // The heap is full: this entry and the rest are dropped.
    if (team == nullptr) {
      break;
    }
    team->Fill_In(name, entry);
  }

  // A scenario in the old format has no [TeamTypes] section.
  if (TheObjectHeaps().team_type().Count() == 0) {
    Read_Old_INI(ini);
  }
}

void TeamTypeClass::Fill_In(std::string_view name, std::string_view entry) {
  Validate();
  std::string_view p1;  // class or mission name
  std::string_view p2;  // its count or argument
  TeamMissionStruct mission;

  Set_Name(name);

  base::Tokenizer tokens(entry);

  // An unknown house name gives HOUSE_NONE.
  House = HouseTypeClass::From_Name(tokens.Next());

  IsRoundAbout = base::ParseIntegerOr<int>(tokens.Next(), 0) != 0;

  IsLearning = base::ParseIntegerOr<int>(tokens.Next(), 0) != 0;

  IsSuicide = base::ParseIntegerOr<int>(tokens.Next(), 0) != 0;

  // The fifth field was once "Spy".
  IsAutocreate = base::ParseIntegerOr<int>(tokens.Next(), 0) != 0;

  IsMercenary = base::ParseIntegerOr<int>(tokens.Next(), 0) != 0;

  RecruitPriority = base::ParseIntegerOr<int>(tokens.Next(), 0);

  // The three counts are stored in a byte each; a number that does not fit
  // wraps.
  MaxAllowed =
      static_cast<uint8_t>(base::ParseIntegerOr<int>(tokens.Next(), 0));

  InitNum = static_cast<uint8_t>(base::ParseIntegerOr<int>(tokens.Next(), 0));

  Fear = static_cast<uint8_t>(base::ParseIntegerOr<int>(tokens.Next(), 0));

  // The members: a count, then that many Class:Num pairs.
  const int num_classes = base::ParseIntegerOr<int>(tokens.Next(), -1);
  if (num_classes < 0 || num_classes > kMaxTeamClasscount) {
    ClassCount = 0;
    MissionCount = 0;
    return;
  }

  ClassCount = 0;
  for (int i = 0; i < num_classes; i++) {
    p1 = tokens.Next(",:");
    p2 = tokens.Next(",:");
    if (p1.empty() || p2.empty()) {
      ClassCount = 0;
      MissionCount = 0;
      return;
    }
    const TechnoTypeClass* otype = nullptr;

    // A name that several kinds of type share goes to the last kind that
    // has it.
    const InfantryType i_id = InfantryTypeClass::From_Name(p1);
    if (i_id != INFANTRY_NONE) {
      otype = &InfantryTypeClass::As_Reference(i_id);
    }

    const UnitType u_id = UnitTypeClass::From_Name(p1);
    if (u_id != UNIT_NONE) {
      otype = &UnitTypeClass::As_Reference(u_id);
    }

    const AircraftType a_id = AircraftTypeClass::From_Name(p1);
    if (a_id != AIRCRAFT_NONE) {
      otype = &AircraftTypeClass::As_Reference(a_id);
    }

    // An unknown type (a unit this build does not have) is dropped and the
    // next member takes its place.
    if (otype) {
      base::At(Class, ClassCount) = otype;
      base::At(DesiredNum, ClassCount) =
          static_cast<uint8_t>(base::ParseIntegerOr<int>(p2, 0));
      ClassCount++;
    }
  }

  // The script: a count, then that many Mission:Arg pairs.
  MissionCount = base::ParseIntegerOr<int>(tokens.Next(), -1);
  if (MissionCount < 0 || MissionCount > kMaxTeamMissions) {
    ClassCount = 0;
    MissionCount = 0;
    return;
  }

  for (int i = 0; i < MissionCount; i++) {
    p1 = tokens.Next(",:");
    p2 = tokens.Next(",:");
    if (p1.empty() || p2.empty()) {
      ClassCount = 0;
      MissionCount = 0;
      return;
    }
    mission.Mission = Mission_From_Name(p1);
    mission.Argument = base::ParseIntegerOr<int>(p2, 0);
    base::At(MissionList, i) = mission;
  }

  // Older scenarios end the entry here, which leaves both options on.
  std::string_view ptr = tokens.Next();
  if (!ptr.empty()) {
    IsReinforcable = base::ParseIntegerOr<int>(ptr, 0) != 0;
  }
  ptr = tokens.Next();
  if (!ptr.empty()) {
    IsPrebuilt = base::ParseIntegerOr<int>(ptr, 0) != 0;
  }
}

void TeamTypeClass::Write_INI(engine::file::IniFile& ini, bool refresh) {
  // The team types are written in the current format only.
  ini.Clear("Teams");

  if (refresh) {
    ini.Clear(INI_Name());
  }

  for (int index = 0; index < TheObjectHeaps().team_type().Count(); index++) {
    const TeamTypeClass* team = TheObjectHeaps().team_type().ActiveAt(index);

    // By its type: the scenario need not have the house itself.
    const std::string_view house_name =
        team->House == HOUSE_NONE
            ? "None"
            : HouseTypeClass::As_Reference(team->House).IniName;

    std::string entry = absl::StrFormat(
        "%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d", house_name, team->IsRoundAbout,
        team->IsLearning, team->IsSuicide, team->IsAutocreate,
        team->IsMercenary, team->RecruitPriority, team->MaxAllowed,
        team->InitNum, team->Fear, team->ClassCount);

    // The members: Class:Num for each.
    for (int i = 0; std::cmp_less(i, team->ClassCount); i++) {
      absl::StrAppendFormat(&entry, ",%s:%d", base::At(team->Class, i)->IniName,
                            base::At(team->DesiredNum, i));
    }

    // The script: a count, then Mission:Arg for each, the mission by name.
    absl::StrAppendFormat(&entry, ",%d", team->MissionCount);
    for (int i = 0; i < team->MissionCount; i++) {
      absl::StrAppendFormat(
          &entry, ",%s:%d",
          Name_From_Mission(base::At(team->MissionList, i).Mission),
          base::At(team->MissionList, i).Argument);
    }

    absl::StrAppendFormat(&entry, ",%d,%d", team->IsReinforcable,
                          team->IsPrebuilt);

    ini.PutString(INI_Name(), team->IniName, entry);
  }
}

void TeamTypeClass::Read_Old_INI(const engine::file::IniFile& ini) {
  for (const auto& [name, entry] : ini.Entries("Teams")) {
    auto* team = base::NewPooled<TeamTypeClass>();
    // The heap is full: this entry and the rest are dropped.
    if (team == nullptr) {
      break;
    }
    team->Set_Name(name);

    base::Tokenizer tokens(entry);
    team->House = HouseTypeClass::From_Name(tokens.Next());

    team->IsRoundAbout = base::ParseIntegerOr<int>(tokens.Next(), 0) != 0;

    team->IsLearning = base::ParseIntegerOr<int>(tokens.Next(), 0) != 0;

    team->IsSuicide = base::ParseIntegerOr<int>(tokens.Next(), 0) != 0;

    team->IsAutocreate = base::ParseIntegerOr<int>(tokens.Next(), 0) != 0;

    team->IsMercenary = base::ParseIntegerOr<int>(tokens.Next(), 0) != 0;

    team->RecruitPriority = base::ParseIntegerOr<int>(tokens.Next(), 0);

    team->MaxAllowed =
        static_cast<uint8_t>(base::ParseIntegerOr<int>(tokens.Next(), 0));

    team->InitNum =
        static_cast<uint8_t>(base::ParseIntegerOr<int>(tokens.Next(), 0));

    tokens.Next();  // mission name, unused

    // The members: Class:Num pairs up to the end of the entry, of which the
    // first kMaxTeamClasscount known types are kept.
    int index = 0;
    std::string_view p1 = tokens.Next(",:");  // class name
    std::string_view p2 = tokens.Next(",:");  // its count
    while (!p1.empty() && !p2.empty() && index < kMaxTeamClasscount) {
      const TechnoTypeClass* otype = nullptr;

      const InfantryType i_id = InfantryTypeClass::From_Name(p1);
      if (i_id != INFANTRY_NONE) {
        otype = &InfantryTypeClass::As_Reference(i_id);
      }

      const UnitType u_id = UnitTypeClass::From_Name(p1);
      if (u_id != UNIT_NONE) {
        otype = &UnitTypeClass::As_Reference(u_id);
      }

      const AircraftType a_id = AircraftTypeClass::From_Name(p1);
      if (a_id != AIRCRAFT_NONE) {
        otype = &AircraftTypeClass::As_Reference(a_id);
      }

      if (otype) {
        base::At(team->Class, index) = otype;
        base::At(team->DesiredNum, index) =
            static_cast<uint8_t>(base::ParseIntegerOr<int>(p2, 0));
        index++;
        team->ClassCount = static_cast<uint8_t>(index);
      }

      p1 = tokens.Next(",:");
      p2 = tokens.Next(",:");
    }

    team->Fear = 0;
  }
}

TeamTypeClass* TeamTypeClass::As_Pointer(std::string_view name) {
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

void TeamTypeClass::Remove() {
  Validate();

  for (int i = 0; i < TheObjectHeaps().trigger().Count(); i++) {
    TriggerClass* trigger = TheObjectHeaps().trigger().ActiveAt(i);
    if (trigger->Team == this) {
      trigger->Team = nullptr;
    }
  }

  delete this;
}

TeamMissionType TeamTypeClass::Mission_From_Name(std::string_view name) {
  if (!name.empty()) {
    for (TeamMissionType order = TMISSION_ATTACKBASE; order < TMISSION_COUNT;
         order++) {
      if (absl::EqualsIgnoreCase(TMissions.at(order), name)) {
        return order;
      }
    }
  }

  return TMISSION_NONE;
}

const char* TeamTypeClass::Name_From_Mission(TeamMissionType order) {
  if (order <= TMISSION_NONE || order >= TMISSION_COUNT) {
    return "None";
  }
  return TMissions.at(order);
}

void* TeamTypeClass::operator new(std::size_t /*unused*/) noexcept {
  void* ptr = TheObjectHeaps().team_type().Allocate();
  if (ptr) {
    static_cast<TeamTypeClass*>(ptr)->IsActive = true;
  }
  return ptr;
}

void TeamTypeClass::operator delete(void* ptr) {
  if (ptr) {
    static_cast<TeamTypeClass*>(ptr)->IsActive = false;
  }
  TheObjectHeaps().team_type().Free(static_cast<TeamTypeClass*>(ptr));
}

TeamClass* TeamTypeClass::Create_One_Of() const {
  // TeamClass::Number counts the live teams of each team type, by slot.
  if (TheWorld().scenario_init() ||
      base::At(TeamClass::Number,
               TheObjectHeaps().team_type().SlotIndexOf(this)) < MaxAllowed) {
    return base::NewPooled<TeamClass>(this, HouseClass::As_Pointer(House));
  }
  return nullptr;
}

Target TeamTypeClass::As_Target() const {
  Validate();
  return Target::Make(KIND_TEAMTYPE,
                      TheObjectHeaps().team_type().SlotIndexOf(this));
}

void TeamTypeClass::Destroy_All_Of() const {
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

const TeamTypeClass* TeamTypeClass::Suggested_New_Team(HouseClass* house,
                                                       uint64_t utypes,
                                                       uint64_t itypes,
                                                       bool alerted) {
  const TeamTypeClass* best = nullptr;
  int bestvalue = 0;

  for (int index = 0; index < TheObjectHeaps().team_type().Count(); index++) {
    const TeamTypeClass* ttype = TheObjectHeaps().team_type().ActiveAt(index);

    if (ttype && ttype->House == house->Class->House &&
        base::At(TeamClass::Number, index) <
            (alerted || !ttype->IsAutocreate ? ttype->MaxAllowed : 0)) {
      // The unit and infantry types this team type wants, as bit masks.
      // Aircraft do not count.
      uint64_t uneeded = 0;
      uint64_t ineeded = 0;
      for (int ctype = 0; std::cmp_less(ctype, ttype->ClassCount); ctype++) {
        switch (base::At(ttype->Class, ctype)->What_Am_I()) {
          case RTTI_INFANTRYTYPE:
            ineeded |=
                base::Bit<uint64_t>(dynamic_cast<const InfantryTypeClass*>(
                                        base::At(ttype->Class, ctype))
                                        ->Type);
            break;

          case RTTI_UNITTYPE:
            uneeded |= base::Bit<uint64_t>(dynamic_cast<const UnitTypeClass*>(
                                               base::At(ttype->Class, ctype))
                                               ->Type);
            break;
          case RTTIType::RTTI_NONE:
          case RTTIType::RTTI_INFANTRY:
          case RTTIType::RTTI_UNIT:
          case RTTIType::RTTI_AIRCRAFT:
          case RTTIType::RTTI_AIRCRAFTTYPE:
          case RTTIType::RTTI_BUILDING:
          case RTTIType::RTTI_BUILDINGTYPE:
          case RTTIType::RTTI_TERRAIN:
          case RTTIType::RTTI_ABSTRACTTYPE:
          case RTTIType::RTTI_ANIM:
          case RTTIType::RTTI_ANIMTYPE:
          case RTTIType::RTTI_BULLET:
          case RTTIType::RTTI_BULLETTYPE:
          case RTTIType::RTTI_OVERLAY:
          case RTTIType::RTTI_OVERLAYTYPE:
          case RTTIType::RTTI_SMUDGE:
          case RTTIType::RTTI_SMUDGETYPE:
          case RTTIType::RTTI_TEAM:
          case RTTIType::RTTI_TEMPLATE:
          case RTTIType::RTTI_TEMPLATETYPE:
          case RTTIType::RTTI_TERRAINTYPE:
          case RTTIType::RTTI_OBJECT:
          case RTTIType::RTTI_SPECIAL:
          default:
            break;
        }
      }

      // A team type the house has no members for is still a candidate, at
      // half its priority.
      int value = 0;
      if (ineeded & itypes || uneeded & utypes) {
        value = ttype->RecruitPriority;
      } else {
        value = ttype->RecruitPriority / 2;
      }

      // The first of equals wins.
      if (!best || bestvalue < value) {
        bestvalue = value;
        best = ttype;
      }
    }
  }

  return best;
}

}  // namespace td
