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

// Team types: the scenario's recipes for the computer's task forces. A team
// type names a house, the kinds and numbers of units that make up one team,
// and the script of missions the team carries out. The [TeamTypes] section of
// a scenario INI declares them; TeamClass (team.h) is a team in the field
// built from one.

#ifndef CNC_RED_ALERT_TD_AI_TEAMTYPE_H_
#define CNC_RED_ALERT_TD_AI_TEAMTYPE_H_

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

#include "absl/base/attributes.h"
#include "base/enum_array.h"
#include "magic_enum/magic_enum.hpp"
#include "td/core/defines.h"
#include "td/objects/house.h"
#include "td/objects/type.h"

class ArchiveReader;
class ArchiveWriter;

namespace engine::file {
class IniFile;
}  // namespace engine::file

namespace td {

class TeamClass;

// One step of a team's script. A scenario INI stores the steps by the names
// TeamTypeClass::Name_From_Mission() gives; a saved game stores the numbers.
enum class TeamMissionType {
  kNone = -1,
  kAttackBase,       // Attack the nearest enemy base.
  kAttackUnits,      // Attack enemy units.
  kAttackCivilians,  // Attack civilians.
  kRampage,          // Attack and destroy anything of the enemy's.
  kDefendBase,       // Protect the house's own base; a move.
  kMove,             // Move to the given waypoint.
  kMoveCell,         // Move to the given cell number.
  kRetreat,          // Fall back; carried out as a move.
  kGuard,            // Guard the area, as a unit's guard mission does.
  kLoop,             // Jump to the given line of the script.
  kAttackTarcom,     // Attack the target the team already has.
  kUnload,           // Move to the given waypoint and unload there.
};

// One line of a team type's script: a mission and its argument. The argument
// is a waypoint for TeamMissionType::kMove and TeamMissionType::kUnload, a cell
// number for TeamMissionType::kMoveCell, a script line for
// TeamMissionType::kLoop and a packed target for
// TeamMissionType::kAttackTarcom. For the attacks other than that one, and for
// TeamMissionType::kDefendBase, TeamMissionType::kRetreat,
// TeamMissionType::kGuard and TeamMissionType::kUnload, it is also the time in
// tenths of a minute after which the team goes on to its next line.
struct TeamMissionTag {
  TeamMissionType Mission = TeamMissionType::kNone;
  int Argument = 0;

  // Saved-game support. Loading fails on a mission that is not one of the
  // enum's.
  template <class Archive>
  void Serialize(Archive& ar) {
    ar(Mission, Argument);
    if constexpr (Archive::kIsReading) {
      if (Mission != TeamMissionType::kNone &&
          !magic_enum::enum_contains(Mission)) {
        ar.Fail("invalid team mission");
      }
    }
  }
};
using TeamMissionStruct = TeamMissionTag;

// A recipe for a team: its house, members, script and creation rules. Team
// types live in the team type heap (TheObjectHeaps().team_type()), are created
// by Read_INI(), the scenario editor or a special reinforcement, and are all
// dropped by Init() when a scenario starts.
//
// Example:
//   TeamTypeClass* type = TeamTypeClass::As_Pointer("atk1");
//   if (type != nullptr) {
//     type->Create_One_Of();
//   }
class TeamTypeClass : public AbstractTypeClass {
 public:
  // The most kinds of member, and the most script lines, a team type holds.
  // Both size arrays that are saved.
  static constexpr int kMaxTeamClasscount = 5;
  static constexpr int kMaxTeamMissions = 20;

  TeamTypeClass() : AbstractTypeClass(0, "") {}
  ~TeamTypeClass() override;
  TeamTypeClass(const TeamTypeClass&) = delete;
  TeamTypeClass& operator=(const TeamTypeClass&) = delete;
  TeamTypeClass(TeamTypeClass&&) = delete;
  TeamTypeClass& operator=(TeamTypeClass&&) = delete;

  // Frees every team type, in preparation for a new scenario.
  static void Init();

  // Creates a team type for every entry of the [TeamTypes] section of `ini`.
  // If that leaves the heap empty, reads the older [Teams] section instead.
  static void Read_INI(const engine::file::IniFile& ini);
  // Names this team type `name` and sets it from `entry`, a [TeamTypes]
  // value:
  //   House,Roundabout,Learning,Suicide,Autocreate,Mercenary,RecruitPriority,
  //   MaxAllowed,InitNum,Fear,ClassCount,Class:Num,...,
  //   MissionCount,Mission:Arg,...,Reinforcable,Prebuilt
  // The house and the missions are names and the options are 0 or 1. The last
  // two fields may be missing, which leaves both options on. A member whose
  // type name is unknown is skipped; a member or mission count that is
  // missing or out of range, or a list shorter than its count, leaves the
  // team type with no members and no missions. The scenario editor also uses
  // this to import a team type from MASTER.INI.
  void Fill_In(std::string_view name, std::string_view entry);
  // Writes every team type to the [TeamTypes] section of `ini` and deletes
  // the old-format [Teams] section. With `refresh` the section is emptied
  // first; without it the entries are added to the ones already there.
  static void Write_INI(engine::file::IniFile& ini, bool refresh);
  // Creates a team type for every entry of the [Teams] section of `ini`, the
  // format of the earliest scenarios:
  //   House,Roundabout,Learning,Suicide,Autocreate,Mercenary,RecruitPriority,
  //   MaxAllowed,InitNum,MissionName,Class:Num,...
  // It has no script; the mission name is ignored.
  static void Read_Old_INI(const engine::file::IniFile& ini);
  // The scenario INI section that lists the team types.
  static const char* INI_Name() { return "TeamTypes"; }
  // Saved-game support; defined in ioobj.cc.
  template <class Archive>
  void Serialize(Archive& ar);

  // Returns the team type with this INI name, ignoring case, or nullptr if
  // there is none or `name` is empty.
  static TeamTypeClass* As_Pointer(std::string_view name);

  // Deletes this team type, first clearing every trigger's reference to it.
  // Teams of the type are left alone. The scenario editor's delete.
  void Remove();
  // Creates a team of this type for its house. Returns nullptr when
  // MaxAllowed teams of the type already exist (a limit scenario setup
  // ignores) or the team heap is full.
  // The heap owns the new object; many callers create without keeping it.
  // NOLINTNEXTLINE(modernize-use-nodiscard)
  TeamClass* Create_One_Of() const;
  // Deletes every team of this type; their members go back to being loose
  // units.
  void Destroy_All_Of() const;

  // Returns the INI and editor name of a team mission, "None" for
  // TeamMissionType::kNone or a value that is not a mission.
  static const char* Name_From_Mission(TeamMissionType order);
  // Returns the mission with this name, ignoring case, or
  // TeamMissionType::kNone if there is none.
  static TeamMissionType Mission_From_Name(std::string_view name);
  // Picks the team type the computer should create for `house` now: among the
  // house's own with fewer than MaxAllowed teams in the field, the one with
  // the highest recruit priority, where a team type that wants none of the
  // unit and infantry types the house has counts for half its priority. The
  // first of equals wins. `utypes` and `itypes` are bit masks, by type
  // number, of the unit and infantry types the house has. Autocreate team
  // types take part only when the house is `alerted`. Returns nullptr when
  // there is no candidate.
  static const TeamTypeClass* Suggested_New_Team(HouseClass* house,
                                                 uint64_t utypes,
                                                 uint64_t itypes, bool alerted);

  // Returns the target value that names this team type.
  [[nodiscard]] Target As_Target() const;

  // Allocates from the team type heap. Returns nullptr when the heap is full;
  // noexcept is what makes a new-expression test for that instead of
  // constructing at null.
  void* operator new(std::size_t /*unused*/) noexcept;
  void* operator new(std::size_t /*unused*/,
                     void* ptr ABSL_ATTRIBUTE_LIFETIME_BOUND) noexcept {
    return ptr;
  }
  void operator delete(void* ptr);

  // Debug self-check: in a build with the cheat keys, stops the game when
  // this object is not in the team type heap. Always returns 1; callers run
  // it for the check and ignore the result.
  // NOLINTNEXTLINE(modernize-use-nodiscard)
  int Validate() const;

  // True while the object occupies its heap slot. operator new sets it
  // before the constructor runs, so the initializer must agree with it;
  // operator delete clears it.
  bool IsActive : 1 = true;

  // The team's members route around high-threat areas.
  bool IsRoundAbout : 1 = false;

  // The team learns from its mistakes. Read and written, but nothing acts on
  // it.
  bool IsLearning : 1 = false;

  // The team does not stop for distractions until it achieves its mission or
  // is dead.
  bool IsSuicide : 1 = false;

  // Only a house that has been alerted (the "autocreate" trigger action)
  // creates teams of this type on its own; see Suggested_New_Team().
  bool IsAutocreate : 1 = false;

  // Mercenaries change sides if they start to lose. Read and written, but
  // nothing acts on it.
  bool IsMercenary : 1 = false;

  // The computer builds the members of this team type ahead of time, whether
  // or not a team of the type exists yet.
  bool IsPrebuilt : 1 = true;

  // A team in the field keeps recruiting replacements for lost members. If
  // false it fights until it is dead; unlike a suicide team, its members
  // still defend themselves.
  bool IsReinforcable : 1 = true;

  // Created only to bring on reinforcements for one special event. The last
  // team of the type deletes the team type as it dies.
  bool IsTransient : 1 = false;

  // Rank among teams when recruiting, 0 to 15: a team takes members from
  // teams of lower priority.
  int RecruitPriority = 7;

  // The scenario's "initial number" of teams of this type. Read, written and
  // saved, but nothing creates teams from it.
  uint8_t InitNum = 0;

  // The most teams of this type that may exist at one time.
  uint8_t MaxAllowed = 0;

  // The team's fear level. Read, written and saved, but nothing acts on it.
  uint8_t Fear = 0;

  // The house the teams belong to; HOUSE_NONE if the INI entry names none.
  HousesType House = HOUSE_NONE;

  // The script: the first MissionCount entries of MissionList.
  int MissionCount = 0;
  TeamMissionStruct MissionList[kMaxTeamMissions]{};

  // The number of kinds of member: the entries in use of Class and
  // DesiredNum.
  uint8_t ClassCount = 0;

  // The type of each kind of member: an infantry, unit or aircraft type.
  const TechnoTypeClass* Class[kMaxTeamClasscount]{};

  // How many of each kind of member a full team has.
  uint8_t DesiredNum[kMaxTeamClasscount]{};

 private:
  // The name of each mission, as Name_From_Mission() returns it.
  static base::EnumArray<TeamMissionType, const char*> TMissions;
};

extern template void TeamTypeClass::Serialize<ArchiveWriter>(ArchiveWriter&);
extern template void TeamTypeClass::Serialize<ArchiveReader>(ArchiveReader&);

}  // namespace td

#endif  // CNC_RED_ALERT_TD_AI_TEAMTYPE_H_
