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

/* $Header:   F:\projects\c&c\vcs\code\teamtype.h_v   2.18   16 Oct 1995
 * 16:45:40   JOE_BOSTIC  $ */
/***************************************************************************
 **   C O N F I D E N T I A L --- W E S T W O O D    S T U D I O S        **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Command & Conquer                        *
 *                                                                         *
 *                    File Name : TEAMTYPE.H                               *
 *                                                                         *
 *                   Programmer : Bill Randolph                            *
 *                                                                         *
 *                   Start Date : December 7, 1994                         *
 *                                                                         *
 *                  Last Update : December 7, 1994   [BR]                  *
 *                                                                         *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef CNC_RED_ALERT_TD_TEAMTYPE_H_
#define CNC_RED_ALERT_TD_TEAMTYPE_H_

class ArchiveReader;
class ArchiveWriter;

#include <cstddef>

#include "absl/base/attributes.h"
#include "base/enum_array.h"
#include "td/defines.h"
#include "td/house.h"
#include "td/target.h"
#include "td/type.h"

/*
********************************** Defines **********************************
*/
// #define	kTeamTypeMax			20			// max #
// of different team types

/*
**	TeamMissionType: the various missions that a team can have.
*/
enum class TeamMissionType {
  TMISSION_NONE = -1,
  TMISSION_ATTACKBASE,       // Attack nearest enemy base.
  TMISSION_ATTACKUNITS,      // Attack all enemy units.
  TMISSION_ATTACKCIVILIANS,  // Attack all civilians
  TMISSION_RAMPAGE,          // attack & destroy anything that's not mine
  TMISSION_DEFENDBASE,       // Protect my base.
                             //	TMISSION_HARVEST,
  //// stake out a Tiberium claim, defend & harvest it
  TMISSION_MOVE,          // moves to waypoint specified.
  TMISSION_MOVECELL,      // moves to cell # specified.
  TMISSION_RETREAT,       // order given by superior team, for coordinating
  TMISSION_GUARD,         // works like an infantry's guard mission
  TMISSION_LOOP,          // loop back to start of mission list
  TMISSION_ATTACKTARCOM,  // attack tarcom
  TMISSION_UNLOAD,        // Unload at current location.
  TMISSION_COUNT,
};
using enum TeamMissionType;

/*
**	This structure contains one team mission value & its argument.
*/
typedef struct TeamMissionTag {
  TeamMissionType Mission = TMISSION_NONE;
  int Argument = 0;

  template <class Archive>
  void Serialize(Archive& ar) {
    ar(Mission, Argument);
    if constexpr (Archive::kIsReading) {
      if (Mission < TMISSION_NONE || Mission >= TMISSION_COUNT) {
        ar.Fail("invalid team mission");
      }
    }
  }
} TeamMissionStruct;

/*
**	TeamTypeClass declaration
*/
class TeamTypeClass : public AbstractTypeClass {
 public:
  static constexpr int kMaxTeamClasscount = 5;
  static constexpr int kMaxTeamMissions = 20;

  /*
  **	Constructor/Destructor
  */
  TeamTypeClass() : AbstractTypeClass(0, "") {}
  ~TeamTypeClass() override = default;
  TeamTypeClass(const TeamTypeClass&) = delete;
  TeamTypeClass& operator=(const TeamTypeClass&) = delete;
  TeamTypeClass(TeamTypeClass&&) = delete;
  TeamTypeClass& operator=(TeamTypeClass&&) = delete;

  /*
  **	Initialization: clears all team types in preparation for new scenario
  */
  static void Init();

  /*
  **	File I/O routines
  */
  static void Read_INI(char* buffer);
  void Fill_In(char* name, char* entry);
  static void Write_INI(char* buffer, bool refresh);
  static void Read_Old_INI(char* buffer);
  static const char* INI_Name() { return "TeamTypes"; }
  // Field-wise saved-game support, defined in ioobj.cc.
  template <class Archive>
  void Serialize(Archive& ar);

  /*
  **	As_Pointer gets a pointer to the trigger object give its name
  */
  static TeamTypeClass* As_Pointer(char* name);

  /*
  **	Processing routines
  */
  void Remove();
  // the heap owns the new object; many callers create without keeping it.
  // NOLINTNEXTLINE(modernize-use-nodiscard)
  TeamClass* Create_One_Of() const ABSL_ATTRIBUTE_LIFETIME_BOUND;
  void Destroy_All_Of() const;

  /*
  **	Utility routines
  */
  static const char* Name_From_Mission(TeamMissionType order);
  static TeamMissionType Mission_From_Name(const char* name);
  static const TeamTypeClass* Suggested_New_Team(HouseClass* house,
                                                 uint64_t utypes,
                                                 uint64_t itypes, bool alerted);

  [[nodiscard]] TARGET As_Target() const;

  /*
  **	Overloaded operators
  */
  void* operator new(size_t /*unused*/) noexcept;
  void* operator new(size_t /*unused*/,
                     void* ptr ABSL_ATTRIBUTE_LIFETIME_BOUND) noexcept {
    return ptr;
  }
  void operator delete(void* ptr);

  /*
  **	Dee-buggin' support.
  */
  // debug self-check; callers run it for its assertions and ignore the count.
  // NOLINTNEXTLINE(modernize-use-nodiscard)
  int Validate() const;

  /*
  **	If this teamtype object is active, then this flag will be true.
  **	TeamType objects that are not active are either not yet created or have
  **	been deleted after fulfilling their action.
  */
  bool IsActive : 1 = true;

  /*
  **	If RoundAbout, the team avoids high-threat areas
  */
  bool IsRoundAbout : 1 = false;

  /*
  **	If Learning, the team learns from mistakes
  */
  bool IsLearning : 1 = false;

  /*
  **	If Suicide, the team won't stop until it achieves its mission or it's
  **	dead
  */
  bool IsSuicide : 1 = false;

  /*
  **	Is this team type allowed to be created automatically by the computer
  **	when the appropriate trigger indicates?
  */
  bool IsAutocreate : 1 = false;

  /*
  **	Mercenaries will change sides if they start to lose.
  */
  bool IsMercenary : 1 = false;

  /*
  **	This flag tells the computer that it should build members to fill
  **	a team of this type regardless of whether there actually is a team
  **	of this type active.
  */
  bool IsPrebuilt : 1 = true;

  /*
  **	If this team should allow recruitment of new members, then this flag
  **	will be true. A false value results in a team that fights until it
  **	is dead. This is similar to IsSuicide, but they will defend themselves.
  */
  bool IsReinforcable : 1 = true;

  /*
  **	A transient team type was created exclusively to bring on reinforcements
  **	as a result of some special event. As soon as there are no teams
  **	existing of this type, then this team type should be deleted.
  */
  bool IsTransient : 1 = false;

  /*
  **	Priority given the team for recruiting purposes; higher priority means
  **	it can steal members from other teams (scale: 0 - 15)
  */
  int RecruitPriority = 7;

  /*
  **	Initial # of this type of team
  */
  unsigned char InitNum = 0;

  /*
  **	Max # of this type of team allowed at one time
  */
  unsigned char MaxAllowed = 0;

  /*
  **	Fear level of this team
  */
  unsigned char Fear = 0;

  /*
  **	House the team belongs to
  */
  HousesType House = HOUSE_NONE;

  /*
  **	The mission list for this team
  */
  int MissionCount = 0;
  TeamMissionStruct MissionList[kMaxTeamMissions]{};

  /*
  **	Number of different classes in the team
  */
  unsigned char ClassCount = 0;

  /*
  **	Array of object types comprising the team
  */
  const TechnoTypeClass* Class[kMaxTeamClasscount]{};

  /*
  **	Desired # of each type of object comprising the team
  */
  unsigned char DesiredNum[kMaxTeamClasscount]{};

 private:
  static base::EnumArray<TeamMissionType, const char*,
                         static_cast<int>(TMISSION_COUNT)>
      TMissions;
};

extern template void TeamTypeClass::Serialize<ArchiveWriter>(ArchiveWriter&);
extern template void TeamTypeClass::Serialize<ArchiveReader>(ArchiveReader&);

#endif  // CNC_RED_ALERT_TD_TEAMTYPE_H_
