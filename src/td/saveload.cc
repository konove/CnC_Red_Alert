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

/* $Header:   F:\projects\c&c\vcs\code\saveload.cpv   2.18   16 Oct 1995
 * 16:48:44   JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : SAVELOAD.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : August 23, 1994 *
 *                                                                                             *
 *                  Last Update : June 24, 1995 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 */

#include "td/saveload.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "absl/log/log.h"
#include "port/ex_string.h"
#include "port/safe_string.h"
#include "sdllib/misc.h"
#include "td/aircraft.h"
#include "td/anim.h"
#include "td/base.h"
#include "td/building.h"
#include "td/bullet.h"
#include "td/cell.h"
#include "td/conquer.h"
#include "td/externs.h"
#include "td/factory.h"
#include "td/globals.h"
#include "td/heap.h"
#include "td/house.h"
#include "td/infantry.h"
#include "td/ini.h"
#include "td/layer.h"
#include "td/logic.h"
#include "td/mapedit.h"
#include "td/mouse.h"
#include "td/object.h"
#include "td/overlay.h"
#include "td/randomstate.h"
#include "td/savepipe.h"
#include "td/scenario.h"
#include "td/score.h"
#include "td/serialize.h"
#include "td/smudge.h"
#include "td/support.h"
#include "td/target.h"
#include "td/team.h"
#include "td/teamtype.h"
#include "td/template.h"
#include "td/terrain.h"
#include "td/trigger.h"
#include "td/unit.h"
#include "td/vector.h"
#include "tech/archive.h"
#include "tech/rawfile.h"
#include "tech/xpipe.h"
#include "tech/xstraw.h"

/*
********************************** Defines **********************************
*/

// Write the theater/map, object heaps, ordered layers, and globals as fields.
bool Save_Game(int id, char* descr) {
  RawFileClass file;
  char name[kMaxFname + kMaxExt];
  int i;
  int32_t version = 0;
  unsigned scenario;
  HousesType house;
  char descr_buf[kDescripMax]{};

  scenario = Scenario;              // get current scenario #
  house = PlayerPtr->Class->House;  // get current house

  /*
  **	Generate the filename to save
  */
  sprintf(name, "SAVEGAME.%03d", id);

  /*
  **	Open the file
  */
  file.Open(name, FileAccess::kWrite);
  if (!file.Is_Open()) {
    return false;
  }

  /*
  **	Save the description, scenario #, and house
  **	(scenario # & house are saved separately from the actual Scenario &
  **	PlayerPtr globals for convenience; we can quickly find out which
  **	house & scenario this save-game file is for by reading these values.
  **	Also, PlayerPtr is stored in a coded form in Save_Misc_Values(),
  **	which may or may not be a HousesType number; so, saving 'house'
  **	here ensures we can always pull out the house for this file.)
  */
  snprintf(descr_buf, sizeof(descr_buf) - 1, "%s\r\n",
           descr);                        // put CR-LF after text
  descr_buf[strlen(descr_buf) + 1] = 26;  // put CTRL-Z after NULL

  if (file.Write(descr_buf, kDescripMax) != kDescripMax) {
    file.Close();
    return false;
  }

  if (file.Write(&scenario, sizeof(scenario)) != sizeof(scenario)) {
    file.Close();
    return false;
  }

  if (file.Write(&house, sizeof(house)) != sizeof(house)) {
    file.Close();
    return false;
  }

  /*
  **	Save the save-game version, for loading verification
  */
  version = kSaveGameVersion;

  if (file.Write(&version, sizeof(version)) != sizeof(version)) {
    file.Close();
    return false;
  }

  FilePipe sink(file);
  SaveGamePipe checked_sink(sink);
  ArchiveWriter writer(checked_sink);
  writer.Section(FourCC("FRAM"));
  writer(Frame);
  const bool saved = [&] {
    Call_Back();
    /*
    **	Save the map.  The map must be saved first, since it saves the Theater.
    */
    if (!Map.Save(writer)) {
      return false;
    }

    Call_Back();
    /*
    **	Save all game objects.  This code saves every object that's stored in a
    **	TFixedIHeap class.
    */
    if (!Houses.Save(writer) || !TeamTypes.Save(writer) ||
        !Teams.Save(writer) || !Triggers.Save(writer) ||
        !Aircraft.Save(writer) || !Anims.Save(writer) ||
        !Buildings.Save(writer) || !Bullets.Save(writer) ||
        !Infantry.Save(writer) || !Overlays.Save(writer) ||
        !Smudges.Save(writer) || !Templates.Save(writer) ||
        !Terrains.Save(writer) || !Units.Save(writer) ||
        !Factories.Save(writer)) {
      return false;
    }

    Call_Back();
    /*
    **	Save the Logic & Map layers
    */
    Logic.Serialize(writer);
    if (!checked_sink.ok()) {
      return false;
    }

    for (i = 0; i < LAYER_COUNT; i++) {
      MouseClass::Layer[i].Serialize(writer);
      if (!checked_sink.ok()) {
        return false;
      }
    }

    /*
    **	Save the Score
    */
    Score.Serialize(writer);
    if (!checked_sink.ok()) {
      return false;
    }

    /*
    **	Save the AI Base
    */
    Base.Serialize(writer);
    if (!checked_sink.ok()) {
      return false;
    }

    /*
    **	Save miscellaneous variables.
    */
    if (!Save_Misc_Values(writer)) {
      return false;
    }

    return true;
  }();
  return saved && checked_sink.ok();
}

// Load heaps before ordered object lists; rebuild runtime placement/UI state last.
bool Load_Game(int id) {
  RawFileClass file;
  char name[kMaxFname + kMaxExt];
  int i;
  int32_t version = 0;
  unsigned scenario;
  HousesType house;
  char descr_buf[kDescripMax];

  /*
  **	Generate the filename to load
  */
  sprintf(name, "SAVEGAME.%03d", id);

  /*
  **	Open the file
  */
  file.Open(name, FileAccess::kRead);
  if (!file.Is_Open()) {
    return false;
  }

  /*
  **	Read & discard the save-game's header info
  */
  if (file.Read(descr_buf, kDescripMax) != kDescripMax) {
    file.Close();
    return false;
  }

  if (file.Read(&scenario, sizeof(scenario)) != sizeof(scenario)) {
    file.Close();
    return false;
  }

  if (file.Read(&house, sizeof(house)) != sizeof(house)) {
    file.Close();
    return false;
  }

  Call_Back();
  /*
  **	Clear the scenario so we start fresh; this calls the Init_Clear()
  *routine *	for the Map, and all object arrays.  It has the following
  *important *	effects: *	- Every cell is cleared to 0's, via
  *MapClass::Init_Clear() *	- All heap elements' are cleared *	- The
  *Houses are Initialized, which also clears their HouseTriggers *	  array
  **	- The map's Layers & Logic Layer are cleared to empty
  **	- The list of currently-selected objects is cleared
  */

  /*
  **	Read in & verify the save-game ID code
  */
  if (file.Read(&version, sizeof(version)) != sizeof(version)) {
    file.Close();
    return false;
  }

  if (version != kSaveGameVersion) {
    file.Close();
    return false;
  }

  Clear_Scenario();
  FileStraw source(file);
  ArchiveReader reader(source);
  if (!reader.Section(FourCC("FRAM"))) {
    return false;
  }
  reader(Frame);
  if (!reader.ok()) {
    return false;
  }

  Call_Back();
  /*
  **	Set the required CD to be in the drive according to the scenario
  **	loaded.
  */
  if (RequiredCD != -2) {
    if (scenario >= 20 && scenario < 60 && GameToPlay == GAME_NORMAL) {
      RequiredCD = 2;
    } else {
      if (scenario >= 60) {
        /*
        ** This is a gateway bonus scenario
        */
        RequiredCD = -1;
      } else {
        if (house == HOUSE_GOOD) {
          RequiredCD = 0;
        } else {
          RequiredCD = 1;
        }
      }
    }
  }
  if (!Force_CD_Available(RequiredCD)) {
    Prog_End();
    exit(EXIT_FAILURE);
  }

  Call_Back();

  /*
  **	Load the map.  The map comes first, since it loads the Theater & init's
  **	mixfiles.  The map calls all the type-class's Init routines, telling
  *them *	what the Theater is; this must be done before any objects are
  *created, so *	they'll be properly created.
  */
  if (!Map.Load(reader)) {
    DLOG(ERROR) << "Cannot load saved map: " << reader.error();
    return false;
  }

  Call_Back();
  /*
  **	Load the object data.
  */
  if (!Houses.Load(reader) || !TeamTypes.Load(reader) || !Teams.Load(reader) ||
      !Triggers.Load(reader) || !Aircraft.Load(reader) || !Anims.Load(reader) ||
      !Buildings.Load(reader) || !Bullets.Load(reader) ||
      !Infantry.Load(reader) || !Overlays.Load(reader) ||
      !Smudges.Load(reader) || !Templates.Load(reader) ||
      !Terrains.Load(reader) || !Units.Load(reader) ||
      !Factories.Load(reader)) {
    DLOG(ERROR) << "Cannot load saved heaps: " << reader.error();
    file.Close();
    return false;
  }

  // Loading shells must not run the gameplay constructor's count increment.
  // Rebuild from active teams so recruitment and transient-type cleanup agree.
  for (auto& count : TeamClass::Number) {
    count = 0;
  }
  for (int j = 0; j < Teams.Count(); ++j) {
    ++TeamClass::Number[TeamTypes.ID(Teams.Ptr(j)->Class)];
  }

  // add triggers
  for (int j = 0; j < Triggers.Count(); j++) {
    TriggerClass* trig = Triggers.Ptr(j);
    if (trig->House != HOUSE_NONE) {
      HouseTriggers[trig->House].Add(trig);
    }
  }

  Call_Back();
  /*
  **	Load the Logic & Map Layers
  */
  Logic.Serialize(reader);
  if (!reader.ok()) {
    DLOG(ERROR) << "Cannot load saved state: " << reader.error();
    file.Close();
    return false;
  }
  for (i = 0; i < LAYER_COUNT; i++) {
    MouseClass::Layer[i].Serialize(reader);
    if (!reader.ok()) {
      file.Close();
      return false;
    }
  }

  Call_Back();
  /*
  **	Load the Score
  */
  Score.Serialize(reader);
  if (!reader.ok()) {
    DLOG(ERROR) << "Cannot load saved state: " << reader.error();
    file.Close();
    return false;
  }

  /*
  **	Load the AI Base
  */
  Base.Serialize(reader);
  if (!reader.ok()) {
    DLOG(ERROR) << "Cannot load saved state: " << reader.error();
    file.Close();
    return false;
  }

  /*
  **	Load miscellaneous variables, including the map size & the Theater
  */
  if (!Load_Misc_Values(reader)) {
    DLOG(ERROR) << "Cannot load saved globals: " << reader.error();
    file.Close();
    return false;
  }

  file.Close();
  Whom = PlayerPtr->Class->House;
  switch (Whom) {
    case HOUSE_GOOD: ScenPlayer = SCEN_PLAYER_GDI; break;
    case HOUSE_BAD: ScenPlayer = SCEN_PLAYER_NOD; break;
    case HOUSE_JP: ScenPlayer = SCEN_PLAYER_JP; break;
    default: break;
  }
  Set_Scenario_Name(ScenarioName, Scenario, ScenPlayer, ScenDir, ScenVar);
  // Placement type resources need every object heap to be loaded first.
  if (Map.PendingObjectPtr) {
    Map.PendingObject = &Map.PendingObjectPtr->Class_Of();
    Map.Set_Cursor_Shape(Map.PendingObject->Occupy_List(true));
  } else {
    Map.PendingObject = nullptr;
    Map.Set_Cursor_Shape(nullptr);
  }
  Map.Init_IO();
  Map.Flag_To_Redraw(true);

  ScenarioInit = 0;

#ifdef DEMO
  if (Scenario != 10 && Scenario != 1 && Scenario != 6) {
    return (false);
  }
#endif

  Call_Back();
  return true;
}

template <class Archive>
static void Serialize_Misc_Values(Archive& ar) {
  ar.Section(FourCC("MISC"));
  ar(HousePtr(PlayerPtr), Scenario, WinMovie, LoseMovie);
  if constexpr (Archive::kIsReading) {
    bool player_loaded = false;
    for (int32_t i = 0; i < Houses.Count(); ++i) {
      player_loaded |= PlayerPtr == Houses.Ptr(i);
    }
    if (!ar.ok() || !player_loaded) {
      ar.Fail("invalid saved player house");
      return;
    }
  }
  SerializeObjectList(ar, CurrentObject);
  ar(Waypoint, ScenDir, ScenVar, CarryOverMoney, CarryOverPercent, BuildLevel,
     BriefMovie, Views, EndCountDown, BriefingText, ActionMovie);
  if constexpr (Archive::kIsReading) {
    WinMovie[sizeof(WinMovie) - 1] = '\0';
    LoseMovie[sizeof(LoseMovie) - 1] = '\0';
    BriefMovie[sizeof(BriefMovie) - 1] = '\0';
    ActionMovie[sizeof(ActionMovie) - 1] = '\0';
    BriefingText[sizeof(BriefingText) - 1] = '\0';
    if (ScenDir < SCEN_DIR_EAST || ScenDir >= SCEN_DIR_COUNT ||
        ScenVar < SCEN_VAR_A ||
        (ScenVar >= SCEN_VAR_COUNT && ScenVar != SCEN_VAR_LOSE)) {
      ar.Fail("invalid saved scenario direction or variant");
    }
    for (CELL cell : Waypoint) {
      if (cell < -1 || cell >= MAP_CELL_TOTAL) {
        ar.Fail("invalid saved waypoint");
      }
    }
    for (CELL cell : Views) {
      if (cell < -1 || cell >= MAP_CELL_TOTAL) {
        ar.Fail("invalid saved view");
      }
    }
  }
  auto random_state = CaptureRandomState();
  ar.Section(FourCC("RNGS"));
  ar(random_state);
  if constexpr (Archive::kIsReading) {
    if (ar.ok()) {
      RestoreRandomState(random_state);
    }
  }
}

bool Save_Misc_Values(ArchiveWriter& file) {
  Serialize_Misc_Values(file);
  return true;
}
bool Load_Misc_Values(ArchiveReader& file) {
  Serialize_Misc_Values(file);
  return file.ok();
}

/***************************************************************************
 * Get_Savefile_Info -- gets description, scenario #, house                *
 *                                                                         *
 * INPUT:                                                                  *
 *      id         numerical ID, for the file extension                    *
 *      buf      buffer to store description in                            *
 *      scenp      ptr to variable to hold scenario                        *
 *      housep   ptr to variable to hold house                             *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      true = OK, false = error (save-game file invalid)                  *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   01/12/1995 BR : Created.                                              *
 *=========================================================================*/
bool Get_Savefile_Info(int id, char* buf, unsigned* scenp, HousesType* housep) {
  RawFileClass file;
  char name[kMaxFname + kMaxExt];
  int32_t version = 0;
  char descr_buf[kDescripMax];

  /*
  **	Generate the filename to load
  */
  sprintf(name, "SAVEGAME.%03d", id);

  /*
  **	If the file opens OK, read the file
  */
  file.Open(name, FileAccess::kRead);
  if (file.Is_Open()) {
    /*
    **	Read in the description, scenario #, and the house
    */
    if (file.Read(descr_buf, kDescripMax) != kDescripMax) {
      file.Close();
      return false;
    }

    descr_buf[kDescripMax - 1] = '\0';
    const auto description_length = strlen(descr_buf);
    if (description_length >= 2 && descr_buf[description_length - 2] == '\r' &&
        descr_buf[description_length - 1] == '\n') {
      descr_buf[description_length - 2] = '\0';
    }
    port::SafeCopy(buf, descr_buf, kDescripMax);

    if (file.Read(scenp, sizeof(unsigned)) != sizeof(unsigned)) {
      file.Close();
      return false;
    }

    if (file.Read(housep, sizeof(HousesType)) != sizeof(HousesType)) {
      file.Close();
      return false;
    }

    /*
    **	Read & verify the save-game version #
    */
    if (file.Read(&version, sizeof(version)) != sizeof(version)) {
      file.Close();
      return false;
    }

    if (version != kSaveGameVersion) {
      file.Close();
      return false;
    }

    file.Close();

    return true;
  }
  return false;
}
