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
#include <memory>
#include <span>
#include <string_view>

#include "absl/log/log.h"
#include "absl/strings/str_format.h"
#include "base/array.h"
#include "base/strings/safe_string.h"
#include "engine/file/disk_file.h"
#include "engine/file/disk_stream.h"
#include "engine/file/file_access.h"
#include "engine/platform/platform.h"
#include "engine/stream/archive.h"
#include "engine/stream/stream_sink.h"
#include "engine/stream/stream_source.h"
#include "td/aircraft.h"
#include "td/anim.h"
#include "td/base.h"
#include "td/building.h"
#include "td/bullet.h"
#include "td/cell.h"
#include "td/conquer.h"
#include "td/defines.h"
#include "td/factory.h"
#include "td/game_clock.h"
#include "td/game_state.h"
#include "td/heap.h"
#include "td/house.h"
#include "td/infantry.h"
#include "td/ini.h"
#include "td/layer.h"
#include "td/logic.h"
#include "td/mapedit.h"
#include "td/mouse.h"
#include "td/object.h"
#include "td/object_heaps.h"
#include "td/overlay.h"
#include "td/randomstate.h"
#include "td/scenario.h"
#include "td/score.h"
#include "td/serialize.h"
#include "td/session.h"
#include "td/smudge.h"
#include "td/startup.h"
#include "td/target.h"
#include "td/team.h"
#include "td/teamtype.h"
#include "td/template.h"
#include "td/terrain.h"
#include "td/trigger.h"
#include "td/unit.h"
#include "td/vector.h"
#include "td/world.h"

/*
********************************** Defines **********************************
*/

// Write the theater/map, object heaps, ordered layers, and globals as fields.
bool Save_Game(int id, const char* descr) {
  char name[engine::platform::kMaxFname + engine::platform::kMaxExt];
  int32_t version = 0;
  char descr_buf[kDescripMax]{};

  const int scenario = TheWorld().scenario();          // get current scenario #
  const HousesType house = ThePlayer()->Class->House;  // get current house

  /*
  **	Generate the filename to save
  */
  absl::SNPrintF(name, sizeof(name), "SAVEGAME.%03d", id);

  /*
  **	Open the file
  */
  const std::unique_ptr<DiskStream> file =
      OpenDiskFile(name, FileAccess::kWrite);
  if (file == nullptr) {
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
  absl::SNPrintF(descr_buf, sizeof(descr_buf) - 1, "%s\r\n",
                 descr);                  // put CR-LF after text
  base::At(descr_buf, std::string_view(descr_buf).size() + 1) =
      26;  // put CTRL-Z after NULL

  if (file->Write(descr_buf, kDescripMax) != kDescripMax) {
    return false;
  }

  if (!file->WriteObject(scenario)) {
    return false;
  }

  if (!file->WriteObject(house)) {
    return false;
  }

  /*
  **	Save the save-game version, for loading verification
  */
  version = kSaveGameVersion;

  if (!file->WriteObject(version)) {
    return false;
  }

  StreamSink sink(*file);
  ArchiveWriter writer(sink);
  writer.Section(FourCC("FRAM"));
  int64_t frame = CurrentFrame();
  writer(frame);
  const bool saved = [&] {
    Call_Back();
    /*
    **	Save the map.  The map must be saved first, since it saves the Theater.
    */
    if (!TheMap().Save(writer)) {
      return false;
    }

    Call_Back();
    /*
    **	Save all game objects.  This code saves every object that's stored in a
    **	TFixedIHeap class.
    */
    if (!TheObjectHeaps().house().Save(writer) ||
        !TheObjectHeaps().team_type().Save(writer) ||
        !TheObjectHeaps().team().Save(writer) ||
        !TheObjectHeaps().trigger().Save(writer) ||
        !TheObjectHeaps().aircraft().Save(writer) ||
        !TheObjectHeaps().anim().Save(writer) ||
        !TheObjectHeaps().building().Save(writer) ||
        !TheObjectHeaps().bullet().Save(writer) ||
        !TheObjectHeaps().infantry().Save(writer) ||
        !TheObjectHeaps().overlay().Save(writer) ||
        !TheObjectHeaps().smudge().Save(writer) ||
        !TheObjectHeaps().tmplate().Save(writer) ||
        !TheObjectHeaps().terrain().Save(writer) ||
        !TheObjectHeaps().unit().Save(writer) ||
        !TheObjectHeaps().factory().Save(writer)) {
      return false;
    }

    Call_Back();
    /*
    **	Save the Logic & Map layers
    */
    TheWorld().logic().Serialize(writer);
    if (!writer.ok()) {
      return false;
    }

    for (auto& i : MouseClass::Layer) {
      i.Serialize(writer);
      if (!writer.ok()) {
        return false;
      }
    }

    /*
    **	Save the Score
    */
    TheWorld().score().Serialize(writer);
    if (!writer.ok()) {
      return false;
    }

    /*
    **	Save the AI Base
    */
    TheWorld().base().Serialize(writer);
    if (!writer.ok()) {
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
  // The flush reports a write the disk refused only once the buffer reached
  // it, such as one to a full disk.
  return saved && sink.Flush();
}

// Load heaps before ordered object lists; rebuild runtime placement/UI state last.
bool Load_Game(int id) {
  char name[engine::platform::kMaxFname + engine::platform::kMaxExt];
  int32_t version = 0;
  unsigned scenario = 0;
  HousesType house = HOUSE_NONE;
  char descr_buf[kDescripMax];

  /*
  **	Generate the filename to load
  */
  absl::SNPrintF(name, sizeof(name), "SAVEGAME.%03d", id);

  /*
  **	Open the file
  */
  const std::unique_ptr<DiskStream> file = OpenDiskFile(name);
  if (file == nullptr) {
    return false;
  }

  /*
  **	Read & discard the save-game's header info
  */
  if (file->Read(descr_buf, kDescripMax) != kDescripMax) {
    return false;
  }

  if (!file->ReadObject(scenario)) {
    return false;
  }

  if (!file->ReadObject(house)) {
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
  if (!file->ReadObject(version)) {
    return false;
  }

  if (version != kSaveGameVersion) {
    return false;
  }

  Clear_Scenario();
  StreamSource source(*file);
  ArchiveReader reader(source);
  if (!reader.Section(FourCC("FRAM"))) {
    return false;
  }
  int64_t frame = 0;
  reader(frame);
  TheGameClock().set_frame(frame);
  if (!reader.ok()) {
    return false;
  }

  Call_Back();
  /*
  **	Set the required CD to be in the drive according to the scenario
  **	loaded.
  */
  if (TheGameState().required_cd() != -2) {
    if (scenario >= 20 && scenario < 60 && TheSession().type() == GAME_NORMAL) {
      TheGameState().required_cd() = 2;
    } else {
      if (scenario >= 60) {
        /*
        ** This is a gateway bonus scenario
        */
        TheGameState().required_cd() = -1;
      } else {
        if (house == HOUSE_GOOD) {
          TheGameState().required_cd() = 0;
        } else {
          TheGameState().required_cd() = 1;
        }
      }
    }
  }
  if (!Force_CD_Available(TheGameState().required_cd())) {
    ShutDown();
    exit(EXIT_FAILURE);
  }

  Call_Back();

  /*
  **	Load the map.  The map comes first, since it loads the Theater & init's
  **	mixfiles.  The map calls all the type-class's Init routines, telling
  *them *	what the Theater is; this must be done before any objects are
  *created, so *	they'll be properly created.
  */
  if (!TheMap().Load(reader)) {
    DLOG(ERROR) << "Cannot load saved map: " << reader.error();
    return false;
  }

  Call_Back();
  /*
  **	Load the object data.
  */
  if (!TheObjectHeaps().house().Load(reader) ||
      !TheObjectHeaps().team_type().Load(reader) ||
      !TheObjectHeaps().team().Load(reader) ||
      !TheObjectHeaps().trigger().Load(reader) ||
      !TheObjectHeaps().aircraft().Load(reader) ||
      !TheObjectHeaps().anim().Load(reader) ||
      !TheObjectHeaps().building().Load(reader) ||
      !TheObjectHeaps().bullet().Load(reader) ||
      !TheObjectHeaps().infantry().Load(reader) ||
      !TheObjectHeaps().overlay().Load(reader) ||
      !TheObjectHeaps().smudge().Load(reader) ||
      !TheObjectHeaps().tmplate().Load(reader) ||
      !TheObjectHeaps().terrain().Load(reader) ||
      !TheObjectHeaps().unit().Load(reader) ||
      !TheObjectHeaps().factory().Load(reader)) {
    DLOG(ERROR) << "Cannot load saved heaps: " << reader.error();
    return false;
  }

  // Loading shells must not run the gameplay constructor's count increment.
  // Rebuild from active teams so recruitment and transient-type cleanup agree.
  for (auto& count : TeamClass::Number) {
    count = 0;
  }
  for (int j = 0; j < TheObjectHeaps().team().Count(); ++j) {
    ++base::At(TeamClass::Number, TheObjectHeaps().team_type().ID(
                                      TheObjectHeaps().team().Ptr(j)->Class));
  }

  // add triggers
  for (int j = 0; j < TheObjectHeaps().trigger().Count(); j++) {
    TriggerClass* trig = TheObjectHeaps().trigger().Ptr(j);
    if (trig->House != HOUSE_NONE) {
      TheWorld().house_triggers().at(trig->House).Add(trig);
    }
  }

  Call_Back();
  /*
  **	Load the Logic & Map Layers
  */
  TheWorld().logic().Serialize(reader);
  if (!reader.ok()) {
    DLOG(ERROR) << "Cannot load saved state: " << reader.error();
    return false;
  }
  for (auto& i : MouseClass::Layer) {
    i.Serialize(reader);
    if (!reader.ok()) {
      return false;
    }
  }

  Call_Back();
  /*
  **	Load the Score
  */
  TheWorld().score().Serialize(reader);
  if (!reader.ok()) {
    DLOG(ERROR) << "Cannot load saved state: " << reader.error();
    return false;
  }

  /*
  **	Load the AI Base
  */
  TheWorld().base().Serialize(reader);
  if (!reader.ok()) {
    DLOG(ERROR) << "Cannot load saved state: " << reader.error();
    return false;
  }

  /*
  **	Load miscellaneous variables, including the map size & the Theater
  */
  if (!Load_Misc_Values(reader)) {
    DLOG(ERROR) << "Cannot load saved globals: " << reader.error();
    return false;
  }

  TheWorld().whom() = ThePlayer()->Class->House;
  switch (TheWorld().whom()) {
    case HOUSE_GOOD:
      TheWorld().scen_player() = SCEN_PLAYER_GDI;
      break;
    case HOUSE_BAD:
      TheWorld().scen_player() = SCEN_PLAYER_NOD;
      break;
    case HOUSE_JP:
      TheWorld().scen_player() = SCEN_PLAYER_JP;
      break;
    case HousesType::HOUSE_NONE:
    case HousesType::HOUSE_NEUTRAL:
    case HousesType::HOUSE_MULTI1:
    case HousesType::HOUSE_MULTI2:
    case HousesType::HOUSE_MULTI3:
    case HousesType::HOUSE_MULTI4:
    case HousesType::HOUSE_MULTI5:
    case HousesType::HOUSE_MULTI6:
    case HousesType::HOUSE_COUNT:
    default: break;
  }
  Set_Scenario_Name(TheWorld().scenario_name(), TheWorld().scenario(),
                    TheWorld().scen_player(), TheWorld().scen_dir(),
                    TheWorld().scen_var());
  // Placement type resources need every object heap to be loaded first.
  if (TheMap().PendingObjectPtr) {
    TheMap().PendingObject = &TheMap().PendingObjectPtr->Class_Of();
    TheMap().Set_Cursor_Shape(TheMap().PendingObject->Occupy_List(true));
  } else {
    TheMap().PendingObject = nullptr;
    TheMap().Set_Cursor_Shape({});
  }
  TheMap().Init_IO();
  TheMap().Flag_To_Redraw(true);

  TheWorld().scenario_init() = 0;

#ifdef DEMO
  if (TheWorld().scenario() != 10 && TheWorld().scenario() != 1 &&
      TheWorld().scenario() != 6) {
    return (false);
  }
#endif

  Call_Back();
  return true;
}

template <class Archive>
static void Serialize_Misc_Values(Archive& ar) {
  ar.Section(FourCC("MISC"));
  ar(HousePtr(ThePlayer()), TheWorld().scenario(), TheWorld().win_movie(),
     TheWorld().lose_movie());
  if constexpr (Archive::kIsReading) {
    bool player_loaded = false;
    for (int32_t i = 0; i < TheObjectHeaps().house().Count(); ++i) {
      player_loaded |= ThePlayer() == TheObjectHeaps().house().Ptr(i);
    }
    if (!ar.ok() || !player_loaded) {
      ar.Fail("invalid saved player house");
      return;
    }
  }
  SerializeObjectList(ar, TheWorld().current_object());
  ar(TheWorld().waypoint(), TheWorld().scen_dir(), TheWorld().scen_var(),
     TheWorld().carry_over_money(), TheWorld().carry_over_percent(),
     TheWorld().build_level(), TheWorld().brief_movie(), TheWorld().views(),
     TheWorld().end_count_down(), TheWorld().briefing_text(),
     TheWorld().action_movie());
  if constexpr (Archive::kIsReading) {
    base::At(TheWorld().win_movie(), sizeof(TheWorld().win_movie()) - 1) = '\0';
    base::At(TheWorld().lose_movie(), sizeof(TheWorld().lose_movie()) - 1) =
        '\0';
    base::At(TheWorld().brief_movie(), sizeof(TheWorld().brief_movie()) - 1) =
        '\0';
    base::At(TheWorld().action_movie(), sizeof(TheWorld().action_movie()) - 1) =
        '\0';
    base::At(TheWorld().briefing_text(),
             sizeof(TheWorld().briefing_text()) - 1) = '\0';
    if (TheWorld().scen_dir() < SCEN_DIR_EAST ||
        TheWorld().scen_dir() >= SCEN_DIR_COUNT ||
        TheWorld().scen_var() < SCEN_VAR_A ||
        (TheWorld().scen_var() >= SCEN_VAR_COUNT &&
         TheWorld().scen_var() != SCEN_VAR_LOSE)) {
      ar.Fail("invalid saved scenario direction or variant");
    }
    for (const CELL cell : TheWorld().waypoint()) {
      if (cell < -1 || cell >= MAP_CELL_TOTAL) {
        ar.Fail("invalid saved waypoint");
      }
    }
    for (const CELL cell : TheWorld().views()) {
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
bool Get_Savefile_Info(int id, std::span<char> buf, unsigned* scenp,
                       HousesType* housep) {
  char name[engine::platform::kMaxFname + engine::platform::kMaxExt];
  int32_t version = 0;
  char descr_buf[kDescripMax];

  /*
  **	Generate the filename to load
  */
  absl::SNPrintF(name, sizeof(name), "SAVEGAME.%03d", id);

  /*
  **	If the file opens OK, read the file
  */
  if (const std::unique_ptr<DiskStream> file = OpenDiskFile(name)) {
    /*
    **	Read in the description, scenario #, and the house
    */
    if (file->Read(descr_buf, kDescripMax) != kDescripMax) {
      return false;
    }

    base::At(descr_buf, kDescripMax - 1) = '\0';
    const auto description_length = std::string_view(descr_buf).size();
    if (description_length >= 2 &&
        base::At(descr_buf, description_length - 2) == '\r' &&
        base::At(descr_buf, description_length - 1) == '\n') {
      base::At(descr_buf, description_length - 2) = '\0';
    }
    base::SafeCopy(std::span(buf).first(kDescripMax), descr_buf);

    if (!file->ReadObject(*scenp)) {
      return false;
    }

    if (!file->ReadObject(*housep)) {
      return false;
    }

    /*
    **	Read & verify the save-game version #
    */
    if (!file->ReadObject(version)) {
      return false;
    }

    if (version != kSaveGameVersion) {
      return false;
    }

    return true;
  }
  return false;
}
