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

/* $Header:   F:\projects\c&c\vcs\code\teamtype.cpv   2.17   16 Oct 1995
 * 16:48:52   JOE_BOSTIC  $ */
/***************************************************************************
 **   C O N F I D E N T I A L --- W E S T W O O D    S T U D I O S        **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Command & Conquer                        *
 *                                                                         *
 *                    File Name : TEAMTYPE.CPP                             *
 *                                                                         *
 *                   Programmer : Bill Randolph                            *
 *                                                                         *
 *                   Start Date : December 7, 1994                         *
 *                                                                         *
 *                  Last Update : July 21, 1995 [JLB]                      *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 *   TeamTypeClass::TeamTypeClass -- class constructor                     *
 *   TeamTypeClass::~TeamTypeClass -- class destructor                     *
 *   TeamTypeClass::Init -- pre-scenario initialization                    *
 *   TeamTypeClass::Read_INI -- reads INI data                             *
 *   TeamTypeClass::Write_INI -- writes INI data                           *
 *   TeamTypeClass::Read_Old_INI -- reads old INI format                   *
 *   TeamTypeClass::As_Pointer -- gets ptr for team type with given name   *
 *   TeamTypeClass::Remove -- removes this team from the system            *
 *   TeamTypeClass::Mission_From_Name -- returns mission for given name    *
 *   TeamTypeClass::Name_From_Mission -- returns name for given mission    *
 *   TeamTypeClass::operator new -- 'new' operator                         *
 *   TeamTypeClass::operator delete -- 'delete' operator                   *
 *   TeamTypeClass::Suggested_New_Team -- Suggests a new team to create.   *
 *   TeamTypeClass::Validate -- validates teamtype pointer
 **
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "td/teamtype.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/strings/match.h"
#include "absl/strings/str_format.h"
#include "base/array.h"
#include "base/enum_array.h"
#include "base/numeric.h"
#include "port/safe_string.h"
#include "port/tokenizer.h"
#include "td/config.h"
#include "td/conquer.h"
#include "td/defines.h"
#include "td/externs.h"
#include "td/heap.h"
#include "td/house.h"
#include "td/profile.h"
#include "td/target.h"
#include "td/team.h"
#include "td/trigger.h"
#include "td/type.h"
#include "tech/number_parse.h"

/*
********************************** Globals **********************************
*/
base::EnumArray<TeamMissionType, const char*, static_cast<int>(TMISSION_COUNT)>
    TeamTypeClass::TMissions = {
        "Attack Base",
        "Attack Units",
        "Attack Civil.",
        "Rampage",
        "Defend Base",
        //	"Harvest",
        "Move",
        "Move to Cell",
        "Retreat",
        "Guard",
        "Loop",
        "Attack Tarcom",
        "Unload",
};

/***********************************************************************************************
 * TeamTypeClass::Validate -- validates teamtype pointer
 **
 *                                                                                             *
 * INPUT: * none.
 **
 *                                                                                             *
 * OUTPUT: * 1 = ok, 0 = error
 **
 *                                                                                             *
 * WARNINGS: * none.
 **
 *                                                                                             *
 * HISTORY: * 08/09/1995 BRR : Created. *
 *=============================================================================================*/
int TeamTypeClass::Validate() const {
  if constexpr (config::kCheatKeysEnabled) {
    const int num = TeamTypes.ID(this);
    if (num < 0 || num >= kTeamTypeMax) {
      Validate_Error("TEAMTYPE");
    }
    return 1;
  } else {
    return 1;
  }
}

/***************************************************************************
 * TeamTypeClass::Init -- pre-scenario initialization                      *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   12/07/1994 BR : Created.                                              *
 *=========================================================================*/
void TeamTypeClass::Init() { TeamTypes.Free_All(); }

/***************************************************************************
 * TeamTypeClass::Read_INI -- reads INI data                               *
 *                                                                         *
 * INI entry format:                                                       *
 *      TeamName = Housename,Roundabout,Learning,Suicide,Spy,Mercenary,    *
 *       RecruitPriority,MaxAllowed,InitNum,Fear,                          *
 *       ClassCount,Class:Num,Class:Num,...,                               *
 *       MissionCount,Mission:Arg,Mission:Arg,Mission:Arg,...              *
 *                                                                         *
 * INPUT:                                                                  *
 *      buffer      buffer to hold the INI data                            *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      none.                                                              *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   12/07/1994 BR : Created.                                              *
 *   02/01/1995 BR : No del team if no classes (editor needs empty teams!) *
 *=========================================================================*/
void TeamTypeClass::Read_INI(char* buffer) {
  char buf[500];        // INI entry buffer

  /*------------------------------------------------------------------------
  Set 'tbuffer' to point just past the INI buffer
  ------------------------------------------------------------------------*/
  std::vector<char> key_storage(std::string_view(buffer).size() + 2);
  auto key_cursor = std::span(key_storage);
  char* tbuffer = key_cursor.data();  // Accumulation buffer of team names.

  /*------------------------------------------------------------------------
  Read all TeamType entry names into 'tbuffer'
  ------------------------------------------------------------------------*/
  WWGetPrivateProfileString(INI_Name(), nullptr, nullptr, key_cursor, buffer);

  /*
  ----------------------- Loop for all team entries ------------------------
  */
  while (*tbuffer != '\0') {
    /*
    ....................... Create a new team type ........................
    */
    auto* team = new TeamTypeClass();  // Working team pointer.

    /*
    ......................... Get the team entry ..........................
    */
    WWGetPrivateProfileString(
        INI_Name(), tbuffer, nullptr,
        std::span(buf).first(static_cast<std::size_t>(sizeof(buf) - 1)),
        buffer);

    /*
    .......................... Fill the team in ...........................
    */
    team->Fill_In(tbuffer, buf);

    /*
    ...................... Go to the next INI entry .......................
    */
    key_cursor = key_cursor.subspan(std::string_view(tbuffer).size() + 1);
    tbuffer = key_cursor.data();
  }

  /*
  ** If no teams were read in, try reading the old INI format.
  */
  if (TeamTypes.Count() == 0) {
    Read_Old_INI(buffer);
  }
}

/***********************************************************************************************
 * TeamTypeClass::Fill_In -- fills in trigger from the given INI entry *
 *                                                                                             *
 * This routine fills in the given teamtype with the given name, and values from
 ** the given INI entry. *
 *                                                                                             *
 * (This routine is used by the scenario editor, to import teams from the
 *MASTER.INI file.)    *
 *                                                                                             *
 *    INI entry format: * TeamName =
 *Housename,Roundabout,Learning,Suicide,Spy,Mercenary,                        *
 *       RecruitPriority,MaxAllowed,InitNum,Fear, *
 *       ClassCount,Class:Num,Class:Num,..., *
 *       MissionCount,Mission:Arg,Mission:Arg,Mission:Arg,... *
 *                                                                                             *
 * INPUT: * name      mnemonic for the desired trigger * entry      INI entry to
 *parse                                                          *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 11/28/1994 BR : Created. *
 *=============================================================================================*/
void TeamTypeClass::Fill_In(char* name, char* entry) {
  Validate();
  char* p1 = nullptr;  // parsing pointer
  char* p2 = nullptr;  // parsing pointer
  TeamMissionStruct mission;

  /*
  ------------------------------ Set its name ------------------------------
  */
  Set_Name(name);

  port::Tokenizer tokens(entry, ",");

  /*
  ---------------------------- 1st token: House ----------------------------
  */
  House = HouseTypeClass::From_Name(tokens.Next());

  /*
  -------------------------- 2nd token: RoundAbout -------------------------
  */
  IsRoundAbout = tech::ParseInteger<int>(tokens.Next()).value_or(0) != 0;

  /*
  --------------------------- 3rd token: Learning --------------------------
  */
  IsLearning = tech::ParseInteger<int>(tokens.Next()).value_or(0) != 0;

  /*
  --------------------------- 4th token: Suicide ---------------------------
  */
  IsSuicide = tech::ParseInteger<int>(tokens.Next()).value_or(0) != 0;

  /*
  ----------------------------- 5th token: Spy -----------------------------
  */
  IsAutocreate = tech::ParseInteger<int>(tokens.Next()).value_or(0) != 0;

  /*
  -------------------------- 6th token: Mercenary --------------------------
  */
  IsMercenary = tech::ParseInteger<int>(tokens.Next()).value_or(0) != 0;

  /*
  ----------------------- 7th token: RecruitPriority -----------------------
  */
  RecruitPriority = tech::ParseInteger<int>(tokens.Next()).value_or(0);

  /*
  -------------------------- 8th token: MaxAllowed -------------------------
  */
  MaxAllowed = static_cast<unsigned char>(
      tech::ParseInteger<int>(tokens.Next()).value_or(0));

  /*
  --------------------------- 9th token: InitNum ---------------------------
  */
  InitNum = static_cast<unsigned char>(
      tech::ParseInteger<int>(tokens.Next()).value_or(0));

  /*
  ------------------------- 10th token: Fear level -------------------------
  */
  Fear = static_cast<unsigned char>(
      tech::ParseInteger<int>(tokens.Next()).value_or(0));

  /*
  ------------------------ 11th token: Class count -------------------------
  */
  const int num_classes = tech::ParseInteger<int>(tokens.Next()).value_or(-1);
  if (num_classes < 0 || num_classes > kMaxTeamClasscount) {
    ClassCount = 0;
    MissionCount = 0;
    return;
  }

  /*
  -------------- Loop through entries, setting class ptr & num -------------
  */
  ClassCount = 0;
  for (int i = 0; i < num_classes; i++) {
    p1 = tokens.Next(",:");
    p2 = tokens.Next(",:");
    if (p1 == nullptr || p2 == nullptr) {
      ClassCount = 0;
      MissionCount = 0;
      return;
    }
    const TechnoTypeClass* otype = nullptr;  // ptr to type of object

    /*
    ------------------- See if this is an infantry name -------------------
    */
    const InfantryType i_id = InfantryTypeClass::From_Name(p1);  // infantry ID
    if (i_id != INFANTRY_NONE) {
      otype = &InfantryTypeClass::As_Reference(i_id);
    }

    /*
    ---------------------- See if this is a unit name ---------------------
    */
    const UnitType u_id = UnitTypeClass::From_Name(p1);  // unit ID
    if (u_id != UNIT_NONE) {
      otype = &UnitTypeClass::As_Reference(u_id);
    }

    /*
    ------------------- See if this is an aircraft name -------------------
    */
    const AircraftType a_id = AircraftTypeClass::From_Name(p1);  // aircraft ID
    if (a_id != AIRCRAFT_NONE) {
      otype = &AircraftTypeClass::As_Reference(a_id);
    }

    /*
    --------------- If the name was resolved, add this class --------------
    */
    if (otype) {
      base::At(Class, ClassCount) = otype;
      base::At(DesiredNum, ClassCount) =
          static_cast<unsigned char>(tech::ParseInteger<int>(p2).value_or(0));
      ClassCount++;
    }
  }

  /*
  ----------------------- next token: Mission count ------------------------
  */
  MissionCount = tech::ParseInteger<int>(tokens.Next()).value_or(-1);
  if (MissionCount < 0 || MissionCount > kMaxTeamMissions) {
    ClassCount = 0;
    MissionCount = 0;
    return;
  }

  for (int i = 0; i < MissionCount; i++) {
    p1 = tokens.Next(",:");
    p2 = tokens.Next(",:");
    if (p1 == nullptr || p2 == nullptr) {
      ClassCount = 0;
      MissionCount = 0;
      return;
    }
    mission.Mission = Mission_From_Name(p1);
    mission.Argument = tech::ParseInteger<int>(p2).value_or(0);
    base::At(MissionList, i) = mission;
  }

  const char* ptr = tokens.Next();
  if (ptr) {
    IsReinforcable = tech::ParseInteger<int>(ptr).value_or(0) != 0;
  }
  ptr = tokens.Next();
  if (ptr) {
    IsPrebuilt = tech::ParseInteger<int>(ptr).value_or(0) != 0;
  }
}

/***************************************************************************
 * TeamTypeClass::Write_INI -- writes INI data                             *
 *                                                                         *
 * INI entry format:                                                       *
 *      TeamName = Housename,Roundabout,Learning,Suicide,Spy,Mercenary,    *
 *       RecruitPriority,MaxAllowed,InitNum,Fear,                          *
 *       ClassCount,Class:Num,Class:Num,...,                               *
 *       MissionCount,Mission,Arg,Mission,Arg,Mission,Arg,...              *
 *                                                                         *
 * INPUT:                                                                  *
 *      buffer      buffer to store INI data in                            *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      none.                                                              *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   12/07/1994 BR : Created.                                              *
 *=========================================================================*/
void TeamTypeClass::Write_INI(std::span<char> buffer, bool refresh) {
  char buf[500];
  const char* hname = nullptr;

  /*------------------------------------------------------------------------
  First, clear out all existing teamtypes in the old-style format.
  ------------------------------------------------------------------------*/
  WWWritePrivateProfileString("Teams", nullptr, nullptr, buffer);

  /*------------------------------------------------------------------------
  Clear out all existing teamtype data from the INI file.
  ------------------------------------------------------------------------*/
  if (refresh) {
    WWWritePrivateProfileString(INI_Name(), nullptr, nullptr, buffer);
  }

  /*------------------------------------------------------------------------
  Now write all the team data out
  ------------------------------------------------------------------------*/
  base::At(buf, 0) = 0;
  for (int index = 0; index < TeamTypes.Count(); index++) {
    /*
    .................. Get ptr to next active teamtype ....................
    */
    TeamTypeClass* team = TeamTypes.Ptr(index);

    /*
    .......................... Find house's name ..........................
    */
    if (team->House == HOUSE_NONE) {
      hname = "None";
    } else {
      hname = HouseClass::As_Pointer(team->House)->Class->IniName;
    }

    /*
    ......................... Generate INI entry ..........................
    */
    absl::SNPrintF(buf, sizeof(buf), "%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d", hname,
                   team->IsRoundAbout, team->IsLearning, team->IsSuicide,
                   team->IsAutocreate, team->IsMercenary, team->RecruitPriority,
                   team->MaxAllowed, team->InitNum, team->Fear,
                   team->ClassCount);

    /*.....................................................................
    For every class in the team, record the class's name & desired count
    .....................................................................*/
    for (int i = 0; std::cmp_less(i, team->ClassCount); i++) {
      absl::SNPrintF(base::Suffix(buf, std::string_view(buf).size()).data(),
                     sizeof(buf) - std::string_view(buf).size(), ",%s:%d",
                     base::At(team->Class, i)->IniName,
                     base::At(team->DesiredNum, i));
    }

    /*.....................................................................
    Record the # of missions, and each mission name & argument value.
    .....................................................................*/
    absl::SNPrintF(base::Suffix(buf, std::string_view(buf).size()).data(),
                   sizeof(buf) - std::string_view(buf).size(), ",%d",
                   team->MissionCount);
    for (int i = 0; i < team->MissionCount; i++) {
      absl::SNPrintF(base::Suffix(buf, std::string_view(buf).size()).data(),
                     sizeof(buf) - std::string_view(buf).size(), ",%s:%d",
                     Name_From_Mission(base::At(team->MissionList, i).Mission),
                     base::At(team->MissionList, i).Argument);
    }

    if (team->IsReinforcable) {
      port::SafeAppend(buf, ",1");
    } else {
      port::SafeAppend(buf, ",0");
    }
    if (team->IsPrebuilt) {
      port::SafeAppend(buf, ",1");
    } else {
      port::SafeAppend(buf, ",0");
    }

    WWWritePrivateProfileString(INI_Name(), team->IniName, buf, buffer);
  }
}

/***************************************************************************
 * TeamTypeClass::Read_Old_INI -- reads old INI format                     *
 *                                                                         *
 *    INI entry format:                                                    *
 *      TeamName = Housename,Roundabout,Learning,Suicide,Spy,Mercenary,    *
 *       RecruitPriority,MaxAllowed,InitNum,Class:Num,Class:Num,...,Fear   *
 *                                                                         *
 * INPUT:                                                                  *
 *      buffer      buffer to hold the INI data                            *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      none.                                                              *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   12/07/1994 BR : Created.                                              *
 *   02/01/1995 BR : No del team if no classes (editor needs empty teams!) *
 *=========================================================================*/
void TeamTypeClass::Read_Old_INI(char* buffer) {
  char buf[256];        // INI entry buffer

  /*------------------------------------------------------------------------
  Set 'tbuffer' to point just past the INI buffer
  ------------------------------------------------------------------------*/
  std::vector<char> key_storage(std::string_view(buffer).size() + 2);
  auto key_cursor = std::span(key_storage);
  char* tbuffer = key_cursor.data();  // Accumulation buffer of team names.

  /*------------------------------------------------------------------------
  Read all TeamType entry names into 'tbuffer'
  ------------------------------------------------------------------------*/
  WWGetPrivateProfileString("Teams", nullptr, nullptr, key_cursor, buffer);

  /*
  ----------------------- Loop for all team entries ------------------------
  */
  while (*tbuffer != '\0') {
    /*
    ........................ Create a new trigger .........................
    */
    auto* team = new TeamTypeClass();  // Working team pointer.

    /*
    ............................ Set its name .............................
    */
    team->Set_Name(tbuffer);

    /*
    ......................... Get the team entry ..........................
    */
    WWGetPrivateProfileString(
        "Teams", tbuffer, nullptr,
        std::span(buf).first(static_cast<std::size_t>(sizeof(buf) - 1)),
        buffer);

    /*
    .......................... 1st token: House ...........................
    */
    port::Tokenizer tokens(buf, ",");
    team->House = HouseTypeClass::From_Name(tokens.Next());

    /*
    ........................ 2nd token: RoundAbout ........................
    */
    team->IsRoundAbout =
        tech::ParseInteger<int>(tokens.Next()).value_or(0) != 0;

    /*
    ......................... 3rd token: Learning .........................
    */
    team->IsLearning = tech::ParseInteger<int>(tokens.Next()).value_or(0) != 0;

    /*
    ......................... 4th token: Suicide ..........................
    */
    team->IsSuicide = tech::ParseInteger<int>(tokens.Next()).value_or(0) != 0;

    /*
    ........................... 5th token: Spy ............................
    */
    team->IsAutocreate =
        tech::ParseInteger<int>(tokens.Next()).value_or(0) != 0;

    /*
    ........................ 6th token: Mercenary .........................
    */
    team->IsMercenary = tech::ParseInteger<int>(tokens.Next()).value_or(0) != 0;

    /*
    ..................... 7th token: RecruitPriority ......................
    */
    team->RecruitPriority = tech::ParseInteger<int>(tokens.Next()).value_or(0);

    /*
    ........................ 8th token: MaxAllowed ........................
    */
    team->MaxAllowed = static_cast<unsigned char>(
        tech::ParseInteger<int>(tokens.Next()).value_or(0));

    /*
    ......................... 9th token: InitNum ..........................
    */
    team->InitNum = static_cast<unsigned char>(
        tech::ParseInteger<int>(tokens.Next()).value_or(0));

    /*
    ....................... 10th token: Mission name ......................
    */
    tokens.Next();  // mission name, unused

    /*
    ............ Loop through entries, setting class ptr & num ............
    */
    int index = 0;
    char* p1 = tokens.Next(",:");  // parsing pointer
    char* p2 = tokens.Next(",:");  // parsing pointer
    while (p1 && p2 && index < kMaxTeamClasscount) {
      const TechnoTypeClass* otype = nullptr;  // ptr to type of object

      /*
      ................. See if this is an infantry name ..................
      */
      const InfantryType i_id =
          InfantryTypeClass::From_Name(p1);  // infantry ID
      if (i_id != INFANTRY_NONE) {
        otype = &InfantryTypeClass::As_Reference(i_id);
      }

      /*
      .................... See if this is a unit name ....................
      */
      const UnitType u_id = UnitTypeClass::From_Name(p1);  // unit ID
      if (u_id != UNIT_NONE) {
        otype = &UnitTypeClass::As_Reference(u_id);
      }

      /*
      ................. See if this is an aircraft name ..................
      */
      const AircraftType a_id =
          AircraftTypeClass::From_Name(p1);  // infantry ID
      if (a_id != AIRCRAFT_NONE) {
        otype = &AircraftTypeClass::As_Reference(a_id);
      }

      /*
      ............. If the name was resolved, add this class .............
      */
      if (otype) {
        base::At(team->Class, index) = otype;
        base::At(team->DesiredNum, index) =
            static_cast<unsigned char>(tech::ParseInteger<int>(p2).value_or(0));
        index++;
        team->ClassCount = static_cast<unsigned char>(index);
      }

      /*
      ................. Go to the next entry on the line .................
      */
      p1 = tokens.Next(",:");
      p2 = tokens.Next(",:");
    }

    team->Fear = 0;

    /*
    ...................... Go to the next INI entry .......................
    */
    key_cursor = key_cursor.subspan(std::string_view(tbuffer).size() + 1);
    tbuffer = key_cursor.data();
  }
}

/***************************************************************************
 * TeamTypeClass::As_Pointer -- gets ptr for team type with given name     *
 *                                                                         *
 * INPUT:                                                                  *
 *      name      name of teamtype                                         *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      ptr to TeamType with that name                                     *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   12/07/1994 BR : Created.                                              *
 *=========================================================================*/
TeamTypeClass* TeamTypeClass::As_Pointer(const char* name) {
  if (name == nullptr) {
    return nullptr;
  }

  for (int i = 0; i < TeamTypes.Count(); i++) {
    if (absl::EqualsIgnoreCase(name, TeamTypes.Ptr(i)->IniName)) {
      return TeamTypes.Ptr(i);
    }
  }

  return nullptr;
}

/***************************************************************************
 * TeamTypeClass::Remove -- removes this team from the system              *
 *                                                                         *
 * INPUT:                                                                  *
 *      none.                                                              *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      none.                                                              *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   12/09/1994 BR : Created.                                              *
 *=========================================================================*/
void TeamTypeClass::Remove() {
  Validate();

  /*
  **	Remove all trigger references to this team.
  */
  for (int i = 0; i < Triggers.Count(); i++) {
    TriggerClass* trigger = Triggers.Ptr(i);
    if (trigger->Team == this) {
      trigger->Team = nullptr;
    }
  }

  /*
  **	Delete myself.
  */
  delete this;
}

/***************************************************************************
 * TeamTypeClass::Mission_From_Name -- returns team mission for given name *
 *                                                                         *
 * INPUT:                                                                  *
 *      name         name to compare                                       *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      mission for that name                                              *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   12/13/1994 BR : Created.                                              *
 *=========================================================================*/
TeamMissionType TeamTypeClass::Mission_From_Name(const char* name) {

  if (name) {
    for (TeamMissionType order = TMISSION_ATTACKBASE; order < TMISSION_COUNT;
         order++) {
      if (absl::EqualsIgnoreCase(TMissions.at(order), name)) {
        return order;
      }
    }
  }

  return TMISSION_NONE;
}

/***************************************************************************
 * TeamTypeClass::Name_From_Mission -- returns name for given mission      *
 *                                                                         *
 * INPUT:                                                                  *
 *      order      mission to get name for                                 *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      name of mission                                                    *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   12/13/1994 BR : Created.                                              *
 *=========================================================================*/
const char* TeamTypeClass::Name_From_Mission(TeamMissionType order) {
  if (order <= TMISSION_NONE || order >= TMISSION_COUNT) {
    return "None";
  }
  return TMissions.at(order);
}

/***************************************************************************
 * TeamTypeClass::operator new -- 'new' operator                           *
 *                                                                         *
 * INPUT:                                                                  *
 *      none.                                                              *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      pointer to new TeamType                                            *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   11/28/1994 BR : Created.                                              *
 *=========================================================================*/
void* TeamTypeClass::operator new(size_t /*unused*/) noexcept {
  void* ptr = TeamTypes.Allocate();
  if (ptr) {
    static_cast<TeamTypeClass*>(ptr)->IsActive = true;
  }
  return ptr;
}

/***************************************************************************
 * TeamTypeClass::operator delete -- 'delete' operator                     *
 *                                                                         *
 * INPUT:                                                                  *
 *      ptr      pointer to delete                                         *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      none.                                                              *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   11/28/1994 BR : Created.                                              *
 *=========================================================================*/
void TeamTypeClass::operator delete(void* ptr) {
  if (ptr) {
    static_cast<TeamTypeClass*>(ptr)->IsActive = false;
  }
  TeamTypes.Free(static_cast<TeamTypeClass*>(ptr));
}

TeamClass* TeamTypeClass::Create_One_Of() const {
  if (ScenarioInit ||
      base::At(TeamClass::Number, TeamTypes.ID(this)) < MaxAllowed) {
    return new TeamClass(this, HouseClass::As_Pointer(House));
  }
  return nullptr;
}

TARGET TeamTypeClass::As_Target() const {
  Validate();
  return Build_Target(KIND_TEAMTYPE, TeamTypes.ID(this));
}

void TeamTypeClass::Destroy_All_Of() const {
  for (int index = 0; index < Teams.Count(); index++) {
    const TeamClass* team = Teams.Ptr(index);

    if (team->Class == this) {
      delete team;
      index--;
    }
  }
}

/***********************************************************************************************
 * TeamTypeClass::Suggested_New_Team -- Suggests a new team to create. *
 *                                                                                             *
 *    This routine will scan through the team types available and create teams
 *of the          * type that can best utilize the existing unit mix. *
 *                                                                                             *
 * INPUT:   house    -- Pointer to the house that this team is to be created
 *for.              *
 *                                                                                             *
 *          utype    -- A bit mask of the unit types available for this house. *
 *                                                                                             *
 *          itypes   -- A bit mask of the infantry types available for this
 *house.             *
 *                                                                                             *
 *          alerted  -- Is this house alerted? If true, then the Autocreate
 *teams will be      * considered in the selection process. *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to the team type that should be created. If
 *no team should  * be created, then it returns NULL. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/13/1995 JLB : Created. * 07/21/1995 JLB : Will autocreate team
 *even if no members in field.                        *
 *=============================================================================================*/
const TeamTypeClass* TeamTypeClass::Suggested_New_Team(HouseClass* house,
                                                       uint64_t utypes,
                                                       uint64_t itypes,
                                                       bool alerted) {
  const TeamTypeClass* best = nullptr;
  int bestvalue = 0;

  for (int index = 0; index < TeamTypes.Count(); index++) {
    const TeamTypeClass* ttype = TeamTypes.Ptr(index);

    if (ttype && ttype->House == house->Class->House &&
        base::At(TeamClass::Number, index) <
            (alerted || !ttype->IsAutocreate ? ttype->MaxAllowed : 0)) {
      /*
      **	Determine what kind of units this team requires.
      */
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

      /*
      **	If this team can use the types required, then consider it a
      *possible *	team type to create.
      */
      int value = 0;
      if (ineeded & itypes || uneeded & utypes) {
        value = ttype->RecruitPriority;
      } else {
        value = ttype->RecruitPriority / 2;
      }

      if (!best || bestvalue < value) {
        bestvalue = value;
        best = ttype;
      }
    }
  }

  return best;
}
