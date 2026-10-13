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

#ifndef CNC_RED_ALERT_TD_AI_TEAM_TYPE_H_
#define CNC_RED_ALERT_TD_AI_TEAM_TYPE_H_

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

#include "absl/base/attributes.h"
#include "base/strings/tokenizer.h"
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
// TeamType::MissionName() gives; a saved game stores the numbers.
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
struct TeamMission {
  TeamMissionType mission = TeamMissionType::kNone;
  int argument = 0;

  // Saved-game support. Loading fails on a mission that is not one of the
  // enum's.
  template <class Archive>
  void Serialize(Archive& ar) {
    ar(mission, argument);
    if constexpr (Archive::kIsReading) {
      if (mission != TeamMissionType::kNone &&
          !magic_enum::enum_contains(mission)) {
        ar.Fail("invalid team mission");
      }
    }
  }
};

// One kind of member a team type asks for, and how many of it.
struct TeamMember {
  uint8_t quantity = 0;
  // An infantry, unit or aircraft type; nullptr only in an unused entry.
  const TechnoTypeClass* type = nullptr;
};

// A recipe for a team: its house, members, script and creation rules. Team
// types live in the team type heap (TheObjectHeaps().team_type()), are created
// by ReadIni(), the scenario editor or a special reinforcement, and are all
// dropped by FreeAll() when a scenario starts.
//
// Example:
//   TeamType* type = TeamType::FromName("atk1");
//   if (type != nullptr) {
//     type->CreateTeam();
//   }
class TeamType : public AbstractTypeClass {
 public:
  // The most kinds of member, and the most script lines, a team type holds.
  // Both size arrays that are saved.
  static constexpr int kMaxMemberTypes = 5;
  static constexpr int kMaxMissions = 20;

  TeamType() : AbstractTypeClass(0, "") {}
  TeamType(const TeamType&) = delete;
  TeamType& operator=(const TeamType&) = delete;
  TeamType(TeamType&&) = delete;
  TeamType& operator=(TeamType&&) = delete;
  ~TeamType() override;

  // Frees every team type, in preparation for a new scenario.
  static void FreeAll();

  // Creates a team type for every entry of the [TeamTypes] section of `ini`.
  // If that leaves the heap empty, reads the older [Teams] section instead.
  static void ReadIni(const engine::file::IniFile& ini);
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
  void ReadIniEntry(std::string_view name, std::string_view entry);
  // Returns this team type's [TeamTypes] value, in the format ReadIniEntry()
  // describes, always with its last two fields.
  [[nodiscard]] std::string ToIniEntry() const;
  // Writes every team type to the [TeamTypes] section of `ini` and deletes
  // the old-format [Teams] section. With `refresh` the section is emptied
  // first; without it the entries are added to the ones already there.
  static void WriteIni(engine::file::IniFile& ini, bool refresh);
  // Creates a team type for every entry of the [Teams] section of `ini`, the
  // format of the earliest scenarios:
  //   House,Roundabout,Learning,Suicide,Autocreate,Mercenary,RecruitPriority,
  //   MaxAllowed,InitNum,MissionName,Class:Num,...
  // It has no script; the mission name is ignored.
  static void ReadOldIni(const engine::file::IniFile& ini);
  // The scenario INI section that lists the team types.
  static constexpr const char* kIniSection = "TeamTypes";
  // Saved-game support; defined in ioobj.cc.
  template <class Archive>
  void Serialize(Archive& ar);

  // Returns the team type with this INI name, ignoring case, or nullptr if
  // there is none or `name` is empty.
  static TeamType* FromName(std::string_view name);

  // Deletes this team type, first clearing every trigger's reference to it.
  // Teams of the type are left alone. The scenario editor's delete.
  void Delete();
  // Creates a team of this type for its house. Returns nullptr when
  // MaxAllowed teams of the type already exist (a limit scenario setup
  // ignores) or the team heap is full.
  // The heap owns the new object; many callers create without keeping it.
  // NOLINTNEXTLINE(modernize-use-nodiscard)
  TeamClass* CreateTeam() const;
  // Returns the number of teams of this type that exist now.
  [[nodiscard]] int CountTeams() const;
  // Deletes every team of this type; their members go back to being loose
  // units.
  void DestroyTeams() const;

  // Returns the INI and editor name of a team mission, "None" for
  // TeamMissionType::kNone or a value that is not a mission.
  static const char* MissionName(TeamMissionType mission);
  // Returns the mission with this name, ignoring case, or
  // TeamMissionType::kNone if there is none.
  static TeamMissionType MissionFromName(std::string_view name);
  // Picks the team type the computer should create for `house` now: among the
  // house's own with fewer than MaxAllowed teams in the field, the one with
  // the highest recruit priority, where a team type that wants none of the
  // unit and infantry types the house has counts for half its priority. The
  // first of equals wins. `unit_types` and `infantry_types` are bit masks, by
  // type number, of the unit and infantry types the house has. Autocreate team
  // types take part only when the house is `alerted`. Returns nullptr when
  // there is no candidate.
  static const TeamType* SuggestNewTeam(HouseClass* house, uint64_t unit_types,
                                        uint64_t infantry_types, bool alerted);

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
  // this object is not in the team type heap.
  void Validate() const;

  // The accessors below are documented at the data members.
  [[nodiscard]] bool is_active() const { return is_active_; }
  [[nodiscard]] bool is_roundabout() const { return is_roundabout_; }
  void set_is_roundabout(const bool value) { is_roundabout_ = value; }
  [[nodiscard]] bool is_learning() const { return is_learning_; }
  void set_is_learning(const bool value) { is_learning_ = value; }
  [[nodiscard]] bool is_suicide() const { return is_suicide_; }
  void set_is_suicide(const bool value) { is_suicide_ = value; }
  [[nodiscard]] bool is_autocreate() const { return is_autocreate_; }
  void set_is_autocreate(const bool value) { is_autocreate_ = value; }
  [[nodiscard]] bool is_mercenary() const { return is_mercenary_; }
  void set_is_mercenary(const bool value) { is_mercenary_ = value; }
  [[nodiscard]] bool is_prebuilt() const { return is_prebuilt_; }
  void set_is_prebuilt(const bool value) { is_prebuilt_ = value; }
  [[nodiscard]] bool is_reinforceable() const { return is_reinforceable_; }
  void set_is_reinforceable(const bool value) { is_reinforceable_ = value; }
  [[nodiscard]] bool is_transient() const { return is_transient_; }
  void set_is_transient(const bool value) { is_transient_ = value; }
  [[nodiscard]] int recruit_priority() const { return recruit_priority_; }
  void set_recruit_priority(const int value) { recruit_priority_ = value; }
  [[nodiscard]] int initial_count() const { return initial_count_; }
  // Take any number: see StoredCount().
  void set_initial_count(const int value) {
    initial_count_ = StoredCount(value);
  }
  [[nodiscard]] int max_teams() const { return max_teams_; }
  void set_max_teams(const int value) { max_teams_ = StoredCount(value); }
  [[nodiscard]] int fear() const { return fear_; }
  [[nodiscard]] HousesType house() const { return house_; }
  void set_house(const HousesType value) { house_ = value; }

  // The script, mission_count() lines long.
  [[nodiscard]] int mission_count() const { return mission_count_; }
  [[nodiscard]] std::span<const TeamMission> missions() const
      // Clang cannot trace the reference through libstdc++ span operations.
      // NOLINTNEXTLINE(clang-diagnostic-lifetime-safety-lifetimebound-violation)
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return std::span(missions_).first(static_cast<std::size_t>(mission_count_));
  }
  // Appends a line to the script, which must have fewer than kMaxMissions.
  void AddMission(const TeamMission& mission);
  // Leaves the team type with no script.
  void ClearMissions() { mission_count_ = 0; }

  // The members wanted, member_count() kinds of them.
  [[nodiscard]] int member_count() const { return member_count_; }
  [[nodiscard]] std::span<const TeamMember> members() const
      // Clang cannot trace the reference through libstdc++ span operations.
      // NOLINTNEXTLINE(clang-diagnostic-lifetime-safety-lifetimebound-violation)
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return std::span(members_).first(static_cast<std::size_t>(member_count_));
  }
  // Appends a kind of member, of which there must be fewer than
  // kMaxMemberTypes.
  void AddMember(const TeamMember& member);
  // Leaves the team type with no members.
  void ClearMembers() { member_count_ = 0; }

 private:
  // Returns `value` as the byte a count is stored in, which a number that
  // does not fit wraps into: a scenario's -1 is 255.
  static constexpr uint8_t StoredCount(const int value) {
    return static_cast<uint8_t>(value);
  }

  // Reads the nine fields both INI formats start with: the house, the five
  // options that are not at the end of the entry, the recruit priority and
  // the two team counts.
  void ReadLeadingFields(base::Tokenizer& tokens);

  // True while the object occupies its heap slot. operator new sets it
  // before the constructor runs, so the initializer must agree with it;
  // operator delete clears it.
  bool is_active_ : 1 = true;

  // The team's members route around high-threat areas.
  bool is_roundabout_ : 1 = false;

  // The team learns from its mistakes. Read and written, but nothing acts on
  // it.
  bool is_learning_ : 1 = false;

  // The team does not stop for distractions until it achieves its mission or
  // is dead.
  bool is_suicide_ : 1 = false;

  // Only a house that has been alerted (the "autocreate" trigger action)
  // creates teams of this type on its own; see SuggestNewTeam().
  bool is_autocreate_ : 1 = false;

  // Mercenaries change sides if they start to lose. Read and written, but
  // nothing acts on it.
  bool is_mercenary_ : 1 = false;

  // The computer builds the members of this team type ahead of time, whether
  // or not a team of the type exists yet.
  bool is_prebuilt_ : 1 = true;

  // A team in the field keeps recruiting replacements for lost members. If
  // false it fights until it is dead; unlike a suicide team, its members
  // still defend themselves.
  bool is_reinforceable_ : 1 = true;

  // Created only to bring on reinforcements for one special event. The last
  // team of the type deletes the team type as it dies.
  bool is_transient_ : 1 = false;

  // Rank among teams when recruiting, 0 to 15: a team takes members from
  // teams of lower priority.
  int32_t recruit_priority_ = 7;

  // The scenario's "initial number" of teams of this type. Read, written and
  // saved, but nothing creates teams from it.
  uint8_t initial_count_ = 0;

  // The most teams of this type that may exist at one time.
  uint8_t max_teams_ = 0;

  // The team's fear level. Read, written and saved, but nothing acts on it.
  uint8_t fear_ = 0;

  // The house the teams belong to; HOUSE_NONE if the INI entry names none.
  HousesType house_ = HOUSE_NONE;

  // The script: the first mission_count_ entries of missions_.
  int32_t mission_count_ = 0;
  TeamMission missions_[kMaxMissions]{};

  // The members wanted: the first member_count_ entries of members_.
  uint8_t member_count_ = 0;
  TeamMember members_[kMaxMemberTypes]{};
};

extern template void TeamType::Serialize<ArchiveWriter>(ArchiveWriter&);
extern template void TeamType::Serialize<ArchiveReader>(ArchiveReader&);

}  // namespace td

#endif  // CNC_RED_ALERT_TD_AI_TEAM_TYPE_H_
