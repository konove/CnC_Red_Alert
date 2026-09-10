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
 * Functions: * Code_All_Pointers -- Code all pointers. * Decode_All_Pointers --
 *Decodes all pointers.                                              *
 *   Get_Savefile_Info -- gets description, scenario #, house * Load_Game --
 *loads a saved game                                                           *
 *   Load_Misc_Values -- Loads miscellaneous variables. * Load_Misc_Values --
 *loads miscellaneous variables                                         *
 *   Read_Object -- reads an object from disk, in a safe way * Save_Game --
 *saves a game to disk                                                         *
 *   Save_Misc_Values -- saves miscellaneous variables * Write_Object
 *-- reads an object from disk, in a safe way                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "td/saveload.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

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
#include "td/savepipe.h"
#include "td/scenario.h"
#include "td/score.h"
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

/***************************************************************************
 * Save_Game -- saves a game to disk                                       *
 *                                                                         *
 * Saving the Map:                                                         *
 *     DisplayClass::Save() invokes CellClass's Write() for every cell     *
 *     that needs to be saved.  A cell needs to be saved if it contains    *
 *     any special data at all, such as a TIcon, or an Occupier.           *
 *   The cell saves its own CellTrigger pointer, converted to a TARGET.    *
 *                                                                         *
 * Saving game objects:                                                    *
 *   - Any object stored in an ArrayOf class needs to be saved.  The ArrayOf*
 *     Save() routine invokes each object's Write() routine, if that       *
 *     object's IsActive is set.                                           *
 *                                                                         *
 * Saving the layers:                                                      *
 *   The Map's Layers (Ground, Air, etc) of things that are on the map,    *
 *     and the Logic's Layer of things to process both need to be saved.   *
 *     LayerClass::Save() writes the entire layer array to disk            *
 *                                                                         *
 * Saving the houses:                                                      *
 *   Each house needs to be saved, to record its Credits, Power, etc.      *
 *                                                                         *
 * Saving miscellaneous data:                                              *
 *   There are a lot of miscellaneous variables to save, such as the       *
 *     map's dimensions, the player's house, etc.                          *
 *                                                                         *
 * INPUT:                                                                  *
 *      id      numerical ID, for the file extension                       *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      true = OK, false = error                                           *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   12/28/1994 BR : Created.                                              *
 *=========================================================================*/
bool Save_Game(int id, char* descr) {
  RawFileClass file;
  char name[_MAX_FNAME + _MAX_EXT];
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
  Code_All_Pointers();
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
    if (!Logic.Save(writer)) {
      return false;
    }

    for (i = 0; i < LAYER_COUNT; i++) {
      if (!MouseClass::Layer[i].Save(writer)) {
        return false;
      }
    }

    /*
    **	Save the Score
    */
    if (!Score.Save(writer)) {
      return false;
    }

    /*
    **	Save the AI Base
    */
    if (!Base.Save(writer)) {
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
  Decode_All_Pointers();
  return saved && checked_sink.ok();
}

/***************************************************************************
 * Load_Game -- loads a saved game                                         *
 *                                                                         *
 * This routine loads the data in the same way it was saved out.           *
 *                                                                         *
 * Loading the Map:                                                        *
 *   - DisplayClass::Load() invokes CellClass's Load() for every cell      *
 *     that was saved.                                                     *
 * - The cell loads its own CellTrigger pointer.                           *
 *                                                                         *
 * Loading game objects:                                                   *
 * - IHeap's Load() routine loads the # of objects stored, and loads       *
 *   each object.                                                          *
 * - Triggers: Add themselves to the HouseTriggers if they're associated   *
 *   with a house                                                          *
 *                                                                         *
 * Loading the layers:                                                     *
 *     LayerClass::Load() reads the entire layer array to disk             *
 *                                                                         *
 * Loading the houses:                                                     *
 *   Each house is loaded in its entirety.                                 *
 *                                                                         *
 * Loading miscellaneous data:                                             *
 *   There are a lot of miscellaneous variables to load, such as the       *
 *     map's dimensions, the player's house, etc.                          *
 *                                                                         *
 * INPUT:                                                                  *
 *      id         numerical ID, for the file extension                    *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      true = OK, false = error                                           *
 *                                                                         *
 * WARNINGS:                                                               *
 *      If this routine returns false, the entire game will be in an       *
 *      unknown state, so the scenario will have to be re-initialized.     *
 *                                                                         *
 * HISTORY:                                                                *
 *   12/28/1994 BR : Created.                                              *
 *=========================================================================*/
bool Load_Game(int id) {
  RawFileClass file;
  char name[_MAX_FNAME + _MAX_EXT];
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
    file.Close();
    return false;
  }

  // Loading shells must not run the gameplay constructor's count increment.
  // Rebuild from active teams so recruitment and transient-type cleanup agree.
  for (auto& count : TeamClass::Number) {
    count = 0;
  }
  for (int i = 0; i < Teams.Count(); ++i) {
    ++TeamClass::Number[TeamTypes.ID(Teams.Ptr(i)->Class)];
  }

  // add triggers
  for (int i = 0; i < Triggers.Count(); i++) {
    TriggerClass* trig = Triggers.Ptr(i);
    if (trig->House != HOUSE_NONE) {
      HouseTriggers[trig->House].Add(trig);
    }
  }

  Call_Back();
  /*
  **	Load the Logic & Map Layers
  */
  if (!Logic.Load(reader)) {
    file.Close();
    return false;
  }
  for (i = 0; i < LAYER_COUNT; i++) {
    if (!MouseClass::Layer[i].Load(reader)) {
      file.Close();
      return false;
    }
  }

  Call_Back();
  /*
  **	Load the Score
  */
  if (!Score.Load(reader)) {
    file.Close();
    return false;
  }

  /*
  **	Load the AI Base
  */
  if (!Base.Load(reader)) {
    file.Close();
    return false;
  }

  /*
  **	Load miscellaneous variables, including the map size & the Theater
  */
  if (!Load_Misc_Values(reader)) {
    file.Close();
    return false;
  }

  file.Close();
  Decode_All_Pointers();
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

/***************************************************************************
 * Save_Misc_Values -- saves miscellaneous variables                       *
 *                                                                         *
 * INPUT:                                                                  *
 *      file      file to use for writing                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      true = success, false = failure                                    *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   12/29/1994 BR : Created.                                              *
 *=========================================================================*/
bool Save_Misc_Values(ArchiveWriter& file) {
  int i;
  int count;         // # ptrs in 'CurrentObject'
  ObjectClass* ptr;  // for saving 'CurrentObject' ptrs

  /*
  **	Player's House.
  */
  file.Bytes(static_cast<const void*>(&PlayerPtr), sizeof(void*));

  /*
  **	Save this scenario number.
  */
  file.Bytes(&Scenario, sizeof(Scenario));

  /*
  **	Save VQ Movie names.
  */
  file.Bytes(WinMovie, sizeof(WinMovie));

  file.Bytes(LoseMovie, sizeof(LoseMovie));

  /*
  **	Save currently-selected objects list.
  **	Save the # of ptrs in the list.
  */
  count = static_cast<int>(CurrentObject.Count());
  file.Bytes(&count, sizeof(count));

  /*
  **	Save the pointers.
  */
  for (i = 0; i < count; i++) {
    ptr = CurrentObject[i];
    file.Bytes(static_cast<const void*>(&ptr), sizeof(void*));
  }

  /*
  **	Save the list of waypoints.
  */
  file.Bytes(Waypoint, sizeof(Waypoint));

  file.Bytes(&ScenDir, sizeof(ScenDir));
  file.Bytes(&ScenVar, sizeof(ScenVar));
  file.Bytes(&CarryOverMoney, sizeof(CarryOverMoney));
  file.Bytes(&CarryOverPercent, sizeof(CarryOverPercent));
  file.Bytes(&BuildLevel, sizeof(BuildLevel));
  file.Bytes(BriefMovie, sizeof(BriefMovie));
  file.Bytes(Views, sizeof(Views));
  file.Bytes(&EndCountDown, sizeof(EndCountDown));
  file.Bytes(BriefingText, sizeof(BriefingText));

  // This is new...
  file.Bytes(ActionMovie, sizeof(ActionMovie));

  return true;
}

/***********************************************************************************************
 * Load_Misc_Values -- Loads miscellaneous variables. *
 *                                                                                             *
 * INPUT:   file  -- The file to load the misc values from. *
 *                                                                                             *
 * OUTPUT:  Was the misc load process successful? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/24/1995 BRR : Created. *
 *=============================================================================================*/
bool Load_Misc_Values(ArchiveReader& file) {
  int i;
  int count;         // # ptrs in 'CurrentObject'
  ObjectClass* ptr;  // for loading 'CurrentObject' ptrs

  /*
  **	Player's House.
  */
  file.Bytes(static_cast<void*>(&PlayerPtr), sizeof(void*));
  if (!file.ok()) {
    return false;
  }

  /*
  **	Read this scenario number.
  */
  file.Bytes(&Scenario, sizeof(Scenario));
  if (!file.ok()) {
    return false;
  }

  /*
  **	Load VQ Movie names.
  */
  file.Bytes(WinMovie, sizeof(WinMovie));
  if (!file.ok()) {
    return false;
  }

  file.Bytes(LoseMovie, sizeof(LoseMovie));
  if (!file.ok()) {
    return false;
  }

  /*
  **	Load currently-selected objects list.
  **	Load the # of ptrs in the list.
  */
  file.Bytes(&count, sizeof(count));
  if (!file.ok()) {
    return false;
  }

  /*
  **	Load the pointers.
  */
  for (i = 0; i < count; i++) {
    file.Bytes(static_cast<void*>(&ptr), sizeof(void*));
    if (!file.ok()) {
      return false;
    }
    CurrentObject.Add(ptr);  // add to the list
  }

  /*
  **	Save the list of waypoints.
  */
  file.Bytes(Waypoint, sizeof(Waypoint));
  if (!file.ok()) {
    return false;
  }

  file.Bytes(&ScenDir, sizeof(ScenDir));
  file.Bytes(&ScenVar, sizeof(ScenVar));
  file.Bytes(&CarryOverMoney, sizeof(CarryOverMoney));
  file.Bytes(&CarryOverPercent, sizeof(CarryOverPercent));
  file.Bytes(&BuildLevel, sizeof(BuildLevel));
  file.Bytes(BriefMovie, sizeof(BriefMovie));
  file.Bytes(Views, sizeof(Views));
  file.Bytes(&EndCountDown, sizeof(EndCountDown));
  file.Bytes(BriefingText, sizeof(BriefingText));

  file.Bytes(ActionMovie, sizeof(ActionMovie));

  return file.ok();
}

/***********************************************************************************************
 * Code_All_Pointers -- Code all pointers. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/24/1995 BRR : Created. *
 *=============================================================================================*/
void Code_All_Pointers() {
  int i;

  /*
  **	The Map.
  */
  Map.Code_Pointers();

  /*
  **	The ArrayOf's.
  */
  TeamTypes.Code_Pointers();
  Teams.Code_Pointers();
  Triggers.Code_Pointers();
  Aircraft.Code_Pointers();
  Anims.Code_Pointers();
  Buildings.Code_Pointers();
  Bullets.Code_Pointers();
  Infantry.Code_Pointers();
  Overlays.Code_Pointers();
  Smudges.Code_Pointers();
  Templates.Code_Pointers();
  Terrains.Code_Pointers();
  Units.Code_Pointers();
  Factories.Code_Pointers();

  /*
  **	The Layers.
  */
  Logic.Code_Pointers();
  for (i = 0; i < LAYER_COUNT; i++) {
    MouseClass::Layer[i].Code_Pointers();
  }

  /*
  **	The Score.
  */
  Score.Code_Pointers();

  /*
  **	The Base.
  */
  Base.Code_Pointers();

  /*
  **	PlayerPtr.
  */
  PlayerPtr = (HouseClass*)PlayerPtr->Class->House;

  /*
  **	Currently-selected objects.
  */
  for (i = 0; i < CurrentObject.Count(); i++) {
    CurrentObject[i] = (ObjectClass*)CurrentObject[i]->As_Target();
  }
}

/***********************************************************************************************
 * Decode_All_Pointers -- Decodes all pointers. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/24/1995 BRR : Created. *
 *=============================================================================================*/
void Decode_All_Pointers() {
  int i;

  /*
  **	The Map.
  */
  Map.Decode_Pointers();

  /*
  ** Decode houses first, so we can properly decode all other objects'
  ** House pointers
  */
  Houses.Decode_Pointers();

  /*
  **	The ArrayOf's.
  */
  TeamTypes.Decode_Pointers();
  Teams.Decode_Pointers();
  Triggers.Decode_Pointers();
  Aircraft.Decode_Pointers();
  Anims.Decode_Pointers();
  Buildings.Decode_Pointers();
  Bullets.Decode_Pointers();
  Infantry.Decode_Pointers();
  Overlays.Decode_Pointers();
  Smudges.Decode_Pointers();
  Templates.Decode_Pointers();
  Terrains.Decode_Pointers();
  Units.Decode_Pointers();
  Factories.Decode_Pointers();

  /*
  **	The Layers.
  */
  Logic.Decode_Pointers();
  for (i = 0; i < LAYER_COUNT; i++) {
    MouseClass::Layer[i].Decode_Pointers();
  }

  /*
  **	The Score.
  */
  Score.Decode_Pointers();

  /*
  **	The Base.
  */
  Base.Decode_Pointers();

  /*
  **	PlayerPtr.
  */
  PlayerPtr =
      HouseClass::As_Pointer(static_cast<HousesType>((intptr_t)PlayerPtr));
  Whom = PlayerPtr->Class->House;
  switch (PlayerPtr->Class->House) {
    case HOUSE_GOOD:
      ScenPlayer = SCEN_PLAYER_GDI;
      break;

    case HOUSE_BAD:
      ScenPlayer = SCEN_PLAYER_NOD;
      break;

    case HOUSE_JP:
      ScenPlayer = SCEN_PLAYER_JP;
      break;
  }
  Check_Ptr(PlayerPtr);

  Set_Scenario_Name(ScenarioName, Scenario, ScenPlayer, ScenDir, ScenVar);

  /*
  **	Currently-selected objects.
  */
  for (i = 0; i < CurrentObject.Count(); i++) {
    CurrentObject[i] =
        As_Object(static_cast<TARGET>((uintptr_t)CurrentObject[i]));
    Check_Ptr(CurrentObject[i]);
  }

  /*
  **	Last-Minute Fixups; to resolve these pointers properly requires all
  *other *	pointers to be loaded & decoded.
  */
  if (Map.PendingObjectPtr) {
    Map.PendingObject = &Map.PendingObjectPtr->Class_Of();
    Check_Ptr(Map.PendingObject);
    Map.Set_Cursor_Shape(Map.PendingObject->Occupy_List(true));
  } else {
    Map.PendingObject = nullptr;
    Map.Set_Cursor_Shape(nullptr);
  }
}

/***********************************************************************************************
 * Read_Object -- reads an object from disk *
 *                                                                                             *
 * This routine reads in an object and fills in the virtual function table
 *pointer.            *
 *                                                                                             *
 * INPUT: * ptr            pointer to object to read * base_size      size of
 *object's absolute base class                                    * class_size
 *size of the class itself                                               * file
 *file to use for I/O                                                    *
 *      vtable         virtual function table pointer value, NULL if none *
 *                                                                                             *
 * OUTPUT: * true = OK, false = error *
 *                                                                                             *
 * WARNINGS: * This routine ASSUMES the program modules are compiled with: *
 *      -Vb-      Always make the virtual function table ptr 2 bytes long * -Vt
 *Put the virtual function table after the 1st class's data *
 *                                                                                             *
 *      ALSO, the class used to compute 'base_size' must come first in a
 *multiple-inheritence  * hierarchy.  AND, if your class multiply-inherits from
 *other classes, only ONE of those * classes can contain virtual functions!  If
 *you include virtual functions in the other  * classes, the compiler will
 *generate multiple virtual function tables, and this load/save * technique will
 *fail.                                                                   *
 *                                                                                             *
 *      Each class hierarchy is stored in memory as a chain: first the data for
 *the base-est   * class, then the virtual function table pointer for this
 *hierarchy, then the data for   * all derived classes.  If any of these derived
 *classes multiply-inherit, the base class * for the multiple inheritance is
 *stored as a separate chain following this chain.  The  * new chain will
 *contain its own virtual function table pointer, if the multiply-        *
 *      inherited hierarchy contains any virtual functions.  Thus, the
 *declaration             * class A * class B: public A * class C: public B, X *
 *      is stored as: * A data * A's Virtual Table Pointer * B data * X data *
 *         [X's Virtual Table Pointer] * C data *
 *                                                                                             *
 *      and * class A * class B: public A * class C: public X, B * is stored in
 *memory as:                                                                * X
 *data * [X's Virtual Table Pointer] * A data * A's Virtual Table Pointer * B
 *data * C data *
 *                                                                                             *
 *                                                                                             *
 * HISTORY: * 01/10/1995 BR : Created. *
 *=============================================================================================*/
bool Read_Object(void* ptr, int base_size, int class_size, ArchiveReader& file,
                 void* vtable) {
  int size;  // object size in bytes

  /*
  **	Read size of this chunk.
  */
  file.Bytes(&size, sizeof(size));
  if (!file.ok()) {
    return false;
  }

  /*
  **	Error if incorrect size.
  */
  if (size != class_size) {
    return false;
  }

  /*
  **	Read object data.
  */
  file.Bytes(ptr, class_size);
  if (!file.ok()) {
    return false;
  }

  /*
  **	Fill in VTable.
  */
  if (vtable) {
    ((void**)(static_cast<char*>(ptr) + base_size - 4))[0] = vtable;
  }

  return file.ok();
}

/***********************************************************************************************
 * Write_Object -- reads an object from disk, in a safe way *
 *                                                                                             *
 * This routine writes an object in 2 pieces, skipping the embedded * virtual
 *function table pointer. *
 *                                                                                             *
 * INPUT: * ptr            pointer to object to write * class_size      size of
 *the class itself                                               * file file to
 *use for I/O                                                    *
 *                                                                                             *
 * OUTPUT: * true = OK, false = error *
 *                                                                                             *
 * WARNINGS: * This routine ASSUMES the program modules are compiled with: *
 *      -Vb-      Always make the virtual function table ptr 2 bytes long * -Vt
 *Put the virtual function table after the 1st class's data *
 *                                                                                             *
 *    Also see warnings for Read_Object(). *
 *                                                                                             *
 * HISTORY: * 01/10/1995 BR : Created. *
 *=============================================================================================*/
bool Write_Object(void* ptr, int class_size, ArchiveWriter& file) {
  /*
  **	Save size of this chunk.
  */
  file.Bytes(&class_size, sizeof(class_size));

  /*
  **	Save object data.
  */
  file.Bytes(ptr, class_size);

  return true;
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
  char name[_MAX_FNAME + _MAX_EXT];
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

/***************************************************************************
 * Get_VTable -- gets the VTable pointer for the given object              *
 *                                                                         *
 * INPUT:                                                                  *
 *      ptr      pointer to check                                          *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      none                                                               *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   01/12/1995 BR : Created.                                              *
 *=========================================================================*/
void* Get_VTable(void* ptr, int base_size) {
  return ((void**)(static_cast<char*>(ptr) + base_size - 4))[0];
}

/***************************************************************************
 * Set_VTable -- sets the VTable pointer for the given object              *
 *                                                                         *
 * INPUT:                                                                  *
 *      ptr         pointer to check                                       *
 *      base_size   size of base class                                     *
 *      vtable      value of VTable to plug in                             *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      none                                                               *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   01/12/1995 BR : Created.                                              *
 *=========================================================================*/
void Set_VTable(void* ptr, int base_size, void* vtable) {
  ((void**)(static_cast<char*>(ptr) + base_size - 4))[0] = vtable;
}
