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

// Team types: the scenario's recipes for the computer's task forces. A team
// type names a house, the kinds and numbers of units that make up one team,
// and the script of missions the team carries out. The [TeamTypes] section of
// a scenario INI declares them; TeamClass (team.h) is a team in the field
// built from one.

#ifndef CNC_RED_ALERT_RA_AI_TEAMTYPE_H_
#define CNC_RED_ALERT_RA_AI_TEAMTYPE_H_

#include <cstddef>
#include <string>
#include <string_view>

#include "absl/base/attributes.h"
#include "base/enum_array.h"
#include "engine/file/ini_file.h"
#include "engine/gfx/pixel_buffer.h"
#include "ra/core/defines.h"
#include "ra/core/target.h"
#include "ra/objects/house.h"
#include "ra/objects/type.h"

class ArchiveReader;
class ArchiveWriter;

namespace ra {

// One step of a team's script. The values are the numbers a scenario INI
// stores, so their order is part of the file format. TeamMission_Needs() says
// which of them take an argument.
enum class TeamMissionType {
  TMISSION_NONE = -1,
  TMISSION_ATTACK,        // Attack the given quarry type.
  TMISSION_ATT_WAYPT,     // Attack whatever is at the given waypoint.
  TMISSION_FORMATION,     // Change the team's formation.
  TMISSION_MOVE,          // Move to the given waypoint.
  TMISSION_MOVECELL,      // Move to the given cell number.
  TMISSION_GUARD,         // Guard the area for the given tenths of a minute.
  TMISSION_LOOP,          // Jump to the given line of the script.
  TMISSION_ATTACKTARCOM,  // Attack the target the team already has.
  TMISSION_UNLOAD,        // Unload at the current location.
  TMISSION_DEPLOY,        // Deploy a mobile building (an MCV).
  TMISSION_HOUND_DOG,     // Follow the nearest friendly unit.
  TMISSION_DO,            // Give each member the given unit mission and stay
                          // on this step.
  TMISSION_SET_GLOBAL,    // Set the given scenario global.
  TMISSION_INVULNERABLE,  // Make the members invulnerable (iron curtain).
  TMISSION_LOAD,          // Load onto the team's transport.
  TMISSION_SPY,           // Send a spy into the building at the given waypoint.
  TMISSION_PATROL,        // Move to the given waypoint, engaging on the way.
};
using enum TeamMissionType;

// One line of a team type's script: a mission and its argument.
class TeamMissionClass {
 public:
  // Saved-game support. The union travels through its widest member.
  template <class Archive>
  void Serialize(Archive& ar) {
    ar(Mission, Data.Value);
  }

  // Returns the line the team editor shows for this mission: `index`, a tab,
  // the mission's name and its argument. `index` is the line's number in the
  // script, shown because TMISSION_LOOP refers to lines by number. The text is
  // empty in a build without the scenario editor or the cheat keys.
  [[nodiscard]] std::string Description(int index) const;
  // Draws Description(index) as a list box entry in the given rectangle,
  // highlighted when `selected`.
  void Draw_It(PixelView& view, int index, int x, int y, int width, int height,
               bool selected, FontType font, TextPrintType flags) const;

  TeamMissionType Mission;
  // The mission's argument; TeamMission_Needs(Mission) says which member is
  // meant. A mission that takes none leaves it 0.
  union {
    FormationType Formation;  // TMISSION_FORMATION
    QuarryType Quarry;        // TMISSION_ATTACK
    MissionType Mission;      // TMISSION_DO
    int Value;  // A waypoint, cell, script line, global or duration.
  } Data;
};

// One kind of member a team type asks for, and how many of it.
class TeamMemberClass {
 public:
  int Quantity = 0;
  // An infantry, unit, aircraft or vessel type; nullptr only in an unused
  // entry.
  const TechnoTypeClass* Class = nullptr;

  // Saved-game support; defined in ioobj.cc.
  template <class Archive>
  void Serialize(Archive& ar);
};

// A recipe for a team: its house, members, script and creation rules. Team
// types live in the team type heap (TheObjectHeaps().team_type()), are created
// by Read_INI() or the scenario editor and are all dropped by Init() when a
// scenario starts.
//
// Example:
//   TeamTypeClass* type = TeamTypeClass::From_Name("atk1");
//   if (type != nullptr) {
//     type->Create_One_Of();
//   }
class TeamTypeClass : public AbstractTypeClass {
 public:
  // The most kinds of member, and the most script lines, a team type holds.
  // Both size arrays that are saved.
  static constexpr int kMaxTeamClasscount = 5;
  static constexpr int kMaxTeamMissions = 20;

  // Takes its ID from the heap slot it is constructed in, so it must be
  // created through operator new.
  TeamTypeClass();
  ~TeamTypeClass() override;
  TeamTypeClass(const TeamTypeClass&) = delete;
  TeamTypeClass& operator=(const TeamTypeClass&) = delete;
  TeamTypeClass(TeamTypeClass&&) = delete;
  TeamTypeClass& operator=(TeamTypeClass&&) = delete;

  // Allocates from the team type heap. Returns nullptr when the heap is full.
  void* operator new(std::size_t /*unused*/);
  void* operator new(std::size_t /*unused*/,
                     void* ptr ABSL_ATTRIBUTE_LIFETIME_BOUND) noexcept {
    return ptr;
  }
  void operator delete(void* ptr);

  // Frees every team type, in preparation for a new scenario.
  static void Init();

  // Returns this team type's [TeamTypes] value, in the current format:
  //   House,Flags,RecruitPriority,InitNum,MaxAllowed,Origin,Trigger,
  //   MemberCount,Type:Quantity,...,MissionCount,Mission:Argument,...
  // Flags packs the five options as bits (roundabout 1, suicide 2, autocreate
  // 4, prebuilt 8, reinforcable 16); Trigger is the trigger type's position in
  // its heap, -1 for none.
  [[nodiscard]] std::string Build_INI_Entry() const;
  // Creates a team type for every entry of the [TeamTypes] section of `ini`.
  // Entries past the heap's capacity are dropped.
  static void Read_INI(engine::file::IniFile& ini);
  // Names this team type `name` and sets it from `entry`, a [TeamTypes] value
  // as Build_INI_Entry() writes it. The older scenario formats (NewINIFormat
  // 0 and 1) list the five options as separate 0/1 fields and put the trigger
  // last. A member whose type name is unknown is skipped, and an origin that
  // is not a waypoint of the scenario is taken as none; a member or mission
  // count that is missing or out of range leaves the team type with no
  // members and no missions.
  void Fill_In(std::string_view name, std::string_view entry);
  // Replaces the [TeamTypes] section of `ini` with every team type.
  static void Write_INI(engine::file::IniFile& ini);
  static const char* INI_Name() { return "TeamTypes"; }
  // Saved-game support; defined in ioobj.cc.
  template <class Archive>
  void Serialize(Archive& ar);

  // Returns the team type with this INI name, ignoring case, or nullptr if
  // there is none or `name` is empty. Same as From_Name().
  static TeamTypeClass* As_Pointer(std::string_view name);

  // Creates a team of this type for its house. Returns nullptr when
  // MaxAllowed teams of the type already exist (a limit scenario setup
  // ignores) or the team heap is full.
  // The heap owns the new object; many callers create without keeping it.
  // NOLINTNEXTLINE(modernize-use-nodiscard)
  TeamClass* Create_One_Of();
  // Deletes every team of this type; their members go back to being loose
  // units.
  void Destroy_All_Of() const;
  // Drops the reference to `target` if it is this team type's trigger, which
  // is about to be deleted. `all` is unused; it is there for the signature
  // Detach_This_From_All() calls on every kind of object.
  void Detach(Target target, bool all = true);

  // Draws Description() as a list box entry in the given rectangle,
  // highlighted when `selected`. `index` is unused. Draws nothing in a build
  // without the scenario editor or the cheat keys.
  void Draw_It(PixelView& view, int index, int x, int y, int width, int height,
               bool selected, FontType font, TextPrintType flags) const;
  // Returns the editor's name for a team mission, which must not be
  // TMISSION_NONE.
  static const char* Name_From_Mission(TeamMissionType order);
  // Picks, at random, a team type the computer could create for `house` now:
  // one of the house's own with fewer than MaxAllowed teams in the field.
  // An alerted house picks only among its autocreate team types, any other
  // house only among the rest. Returns nullptr when there is no candidate. The
  // four type masks are unused.
  static TeamTypeClass* Suggested_New_Team(HouseClass* house,
                                                 uint64_t atypes,
                                                 uint64_t utypes,
                                                 uint64_t itypes,
                                                 uint64_t vtypes, bool alerted);
  // Returns the team type with this INI name, ignoring case, or nullptr if
  // there is none or `name` is empty.
  static TeamTypeClass* From_Name(std::string_view name);
  // Runs the scenario editor's team type dialog on this team type and blocks
  // until the player closes it. Returns true if it was confirmed with OK, and
  // false if it was cancelled, which leaves the team type as it was apart from
  // a missing house, which becomes HOUSE_GOOD either way. The members are
  // edited by a second dialog (MapEditClass::Team_Members()) that this one
  // only opens.
  bool Edit();
  // Returns the members as "TYPE:count,TYPE:count", cut to 25 characters with
  // a trailing "..." when longer. The text is empty in a build without the
  // scenario editor or the cheat keys.
  [[nodiscard]] std::string Member_Description() const;
  // Returns the line the team type list shows: the name, the house's suffix,
  // a '*' for an autocreate team followed by the origin waypoint's letters,
  // the number of missions and Member_Description(), separated by tabs. Empty
  // in a build without the scenario editor or the cheat keys.
  [[nodiscard]] std::string Description() const;

  // True while the object occupies its heap slot. operator new sets it
  // before the constructor runs, so the initializer must agree with it;
  // operator delete clears it.
  bool IsActive : 1 = true;

  // The team's members route around high-threat areas.
  bool IsRoundAbout : 1 = false;

  // The team does not stop for distractions until it achieves its mission or
  // is dead.
  bool IsSuicide : 1 = false;

  // Only a house that has been alerted (the "autocreate" trigger action)
  // creates teams of this type on its own; see Suggested_New_Team().
  bool IsAutocreate : 1 = false;

  // The computer builds the members of this team type ahead of time, whether
  // or not a team of the type exists yet.
  bool IsPrebuilt : 1 = true;

  // A team in the field keeps recruiting replacements for lost members. If
  // false it fights until it is dead; unlike a suicide team, its members
  // still defend themselves.
  bool IsReinforcable : 1 = true;

  // Created only to bring on reinforcements for one special event (the
  // paratrooper drop), not by the scenario.
  bool IsTransient : 1 = false;

  // Rank among teams when recruiting, 0 to 15: a team takes members from
  // teams of lower priority.
  int RecruitPriority = 7;

  // The scenario's "initial number" of teams of this type. Read, written and
  // saved, but nothing creates teams from it.
  unsigned char InitNum = 0;

  // The most teams of this type that may exist at one time.
  unsigned char MaxAllowed = 0;

  // Unused; kept because it is in the saved game.
  unsigned char Fear = 0;

  // The house the teams belong to. HOUSE_NONE until Fill_In() or the editor
  // sets it.
  HousesType House = HOUSE_NONE;

  // The trigger attached to each object as it joins a team of this type;
  // nullptr for none.
  TriggerType* Trigger = nullptr;

  // The waypoint where a team of this type is created or brought on as a
  // reinforcement; -1 for none, otherwise below
  // ScenarioClass::kWaypointCount.
  WAYPOINT Origin = -1;

  // The number of teams of this type that exist now.
  int Number = 0;

  // The script: the first MissionCount entries of MissionList.
  int MissionCount = 0;
  TeamMissionClass MissionList[kMaxTeamMissions]{};

  // The members wanted: the first ClassCount entries of Members.
  int ClassCount = 0;
  TeamMemberClass Members[kMaxTeamClasscount];

  // The scenario editor's name for each team mission.
  static base::EnumArray<TeamMissionType, const char*> TMissions;
};

// Returns the kind of argument a team mission takes, NEED_NONE for a mission
// that takes none; it says which member of TeamMissionClass::Data is in use
// and which control the team editor offers.
NeedType TeamMission_Needs(TeamMissionType tmtype);

extern template void TeamMemberClass::Serialize<ArchiveWriter>(ArchiveWriter&);
extern template void TeamMemberClass::Serialize<ArchiveReader>(ArchiveReader&);
extern template void TeamTypeClass::Serialize<ArchiveWriter>(ArchiveWriter&);
extern template void TeamTypeClass::Serialize<ArchiveReader>(ArchiveReader&);

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_AI_TEAMTYPE_H_
