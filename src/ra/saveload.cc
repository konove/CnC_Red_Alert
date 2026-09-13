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

/* $Header: /counterstrike/SAVELOAD.CPP 9     3/17/97 1:04a Steve_tall $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
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
 *                  Last Update : July 8, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Field-wise saved-game orchestration and post-load fixups.
 */

#include "ra/saveload.h"

#include <array>
#include <cassert>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <utility>
#include <vector>

#include "absl/log/log.h"
#include "magic_enum/magic_enum.hpp"
#include "port/ex_string.h"
#include "port/safe_string.h"
#include "ra/aircraft.h"
#include "ra/anim.h"
#include "ra/base.h"
#include "ra/building.h"
#include "ra/bullet.h"
#include "ra/carry.h"
#include "ra/ccfile.h"
#include "ra/ccini.h"
#include "ra/ccptr.h"
#include "ra/cell.h"
#include "ra/conquer.h"
#include "ra/expand.h"
#include "ra/externs.h"
#include "ra/factory.h"
#include "ra/globals.h"
#include "ra/goptions.h"
#include "ra/heap.h"
#include "ra/house.h"
#include "ra/infantry.h"
#include "ra/inline.h"
#include "ra/jshell.h"
#include "ra/layer.h"
#include "ra/link.h"
#include "ra/logic.h"
#include "ra/mapedit.h"
#include "ra/mission_id.h"
#include "ra/mouse.h"
#include "ra/object.h"
#include "ra/overlay.h"
#include "ra/palette.h"
#include "ra/rules.h"
#include "ra/scenario.h"
#include "ra/score.h"
#include "ra/serialize.h"
#include "ra/session.h"
#include "ra/smudge.h"
#include "ra/special.h"
#include "ra/startup.h"
#include "ra/target.h"
#include "ra/team.h"
#include "ra/teamtype.h"
#include "ra/template.h"
#include "ra/terrain.h"
#include "ra/theme.h"
#include "ra/trigger.h"
#include "ra/trigtype.h"
#include "ra/type.h"
#include "ra/unit.h"
#include "ra/vector.h"
#include "ra/vector_dynamic.h"
#include "ra/vessel.h"
#include "ra/vortex.h"
#include "tech/bfiofile.h"
#include "tech/blowfish.h"
#include "tech/blowpipe.h"
#include "tech/blwstraw.h"
#include "tech/lzopipe.h"
#include "tech/lzostraw.h"
#include "tech/rawfile.h"
#include "tech/shapipe.h"
#include "tech/shastraw.h"
#include "tech/teepipe.h"
#include "tech/xpipe.h"
#include "tech/xstraw.h"

#define SAVE_BLOCK_SIZE 4096

/*
********************************** Defines **********************************
*/
static int Reconcile_Players();

// Section tags bracket every top-level block of the save body. They cost
// four bytes each and turn a field-list mismatch into an error that names
// the block instead of garbage further down the stream.
static void Put_Section(Pipe& pipe, uint32_t tag) {
  ArchiveWriter(pipe).Section(tag);
}
static bool Get_Section(Straw& straw, uint32_t tag) {
  ArchiveReader reader(straw);
  return reader.Section(tag);
}

// Trigger lists load after the trigger heap, so targets can resolve directly.
template <class Archive>
static void SerializeTriggerList(Archive& ar, DynamicVectorClass<TriggerClass*>& list) {
  auto count = static_cast<int32_t>(list.Count());
  ar(count);
  if constexpr (Archive::kIsReading) {
    if (!ar.ok() || count < 0 || count > Triggers.Length()) {
      ar.Fail("invalid trigger list count");
      return;
    }
    list.Clear();
  }
  for (int i = 0; i < count; ++i) {
    TARGET target = kTargetNone;
    if constexpr (!Archive::kIsReading) {
      target = list[i]->As_Target();
    }
    ar(target);
    if constexpr (Archive::kIsReading) {
      if (!ar.ok() || !Is_Target_Trigger(target) ||
          Target_Value(target) >= static_cast<unsigned>(Triggers.Length())) {
        ar.Fail("invalid saved trigger target");
        return;
      }
      list.Add(As_Trigger(target));
    }
  }
}

template <class Archive>
static void SerializeTriggerLists(Archive& ar) {
  SerializeTriggerList(ar, MapTriggers);
  SerializeTriggerList(ar, LogicTriggers);
  for (HousesType house : magic_enum::enum_values<HousesType>()) {
    SerializeTriggerList(ar, HouseTriggers[house]);
  }
}

template <class Archive>
static void SerializeCarryover(Archive& ar) {
  auto count = static_cast<int32_t>(Carryover.size());
  ar(count);
  if constexpr (Archive::kIsReading) {
    if (!ar.ok() || count < 0 || count > MAP_CELL_TOTAL) {
      ar.Fail("invalid carryover count");
      return;
    }
    Carryover.clear();
    for (int i = 0; i < count; ++i) {
      CarryoverClass object;
      ar(object);
      if (!ar.ok()) {
        return;
      }
      Carryover.push_back(object);
    }
  } else {
    for (auto& object : Carryover) {
      ar(object);
    }
  }
}

/***********************************************************************************************
 * Put_All -- Store all save game data to the pipe. *
 *                                                                                             *
 *    This is the bulk processor of the game related save game data. All the
 *game object       * and state data is stored to the pipe specified. *
 *                                                                                             *
 * INPUT:   pipe  -- Reference to the pipe that will receive the save game data.
 **
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/08/1996 JLB : Created. *
 *=============================================================================================*/
static void Put_All(Pipe& pipe, int save_net) {
  ArchiveWriter writer(pipe);
  /*
  **	Frame goes first: every frame-based timer re-anchors to it when read.
  */
  Put_Section(pipe, FourCC("FRAM"));
  writer(Frame);

  /*
  **	Save the scenario global information.
  */
  Put_Section(pipe, FourCC("SCEN"));
  writer(Scen);

  /*
  **	Save the map.  The map must be saved first, since it saves the Theater.
  */
  if (!save_net) {
    Call_Back();
  }
  Put_Section(pipe, FourCC("MAP_"));
  Map.Save(pipe);

  if (!save_net) {
    Call_Back();
  }

  /*
  **	Save all game objects.  This code saves every object that's stored in a
  **	TFixedIHeap class.
  */
  Put_Section(pipe, FourCC("HOUS"));
  Houses.Save(pipe);
  if (!save_net) {
    Call_Back();
  }
  Put_Section(pipe, FourCC("TMTY"));
  TeamTypes.Save(pipe);
  if (!save_net) {
    Call_Back();
  }
  Put_Section(pipe, FourCC("TEAM"));
  Teams.Save(pipe);
  if (!save_net) {
    Call_Back();
  }
  Put_Section(pipe, FourCC("TRTY"));
  TriggerTypes.Save(pipe);
  if (!save_net) {
    Call_Back();
  }
  Put_Section(pipe, FourCC("TRIG"));
  Triggers.Save(pipe);
  if (!save_net) {
    Call_Back();
  }
  Put_Section(pipe, FourCC("AIRC"));
  Aircraft.Save(pipe);
  if (!save_net) {
    Call_Back();
  }
  Put_Section(pipe, FourCC("ANIM"));
  Anims.Save(pipe);

  if (!save_net) {
    Call_Back();
  }

  Put_Section(pipe, FourCC("BLDG"));
  Buildings.Save(pipe);
  if (!save_net) {
    Call_Back();
  }
  Put_Section(pipe, FourCC("BULL"));
  Bullets.Save(pipe);
  if (!save_net) {
    Call_Back();
  }
  Put_Section(pipe, FourCC("INFT"));
  Infantry.Save(pipe);
  if (!save_net) {
    Call_Back();
  }
  Put_Section(pipe, FourCC("OVRL"));
  Overlays.Save(pipe);
  if (!save_net) {
    Call_Back();
  }
  Put_Section(pipe, FourCC("SMDG"));
  Smudges.Save(pipe);
  if (!save_net) {
    Call_Back();
  }
  Put_Section(pipe, FourCC("TMPL"));
  Templates.Save(pipe);
  if (!save_net) {
    Call_Back();
  }
  Put_Section(pipe, FourCC("TERR"));
  Terrains.Save(pipe);
  if (!save_net) {
    Call_Back();
  }
  Put_Section(pipe, FourCC("UNIT"));
  Units.Save(pipe);
  if (!save_net) {
    Call_Back();
  }
  Put_Section(pipe, FourCC("FACT"));
  Factories.Save(pipe);
  if (!save_net) {
    Call_Back();
  }
  Put_Section(pipe, FourCC("VESL"));
  Vessels.Save(pipe);

  if (!save_net) {
    Call_Back();
  }

  /*
  **	Save the Logic & Map layers
  */
  Put_Section(pipe, FourCC("LOGC"));
  writer(Logic);

  Put_Section(pipe, FourCC("TRGV"));
  SerializeTriggerLists(writer);
  if (!save_net) {
    Call_Back();
  }

  Put_Section(pipe, FourCC("LAYR"));
  for (int i = 0; std::cmp_less(i, magic_enum::enum_count<LayerType>()); i++) {
    writer(MouseClass::Layer[i]);
  }

  if (!save_net) {
    Call_Back();
  }

  /*
  **	Save the Score
  */
  Put_Section(pipe, FourCC("SCOR"));
  writer(Score);
  if (!save_net) {
    Call_Back();
  }

  /*
  **	Save the AI Base
  */
  Put_Section(pipe, FourCC("BASE"));
  writer(Base);
  if (!save_net) {
    Call_Back();
  }

  Put_Section(pipe, FourCC("CARY"));
  SerializeCarryover(writer);
  if (!save_net) {
    Call_Back();
  }

  /*
  **	Save miscellaneous variables.
  */
  Put_Section(pipe, FourCC("MISC"));
  Save_Misc_Values(pipe);

  if (!save_net) {
    Call_Back();
  }

  /*
  **	Save multiplayer values
  */
  if (save_net) {
    Put_Section(pipe, FourCC("MPLY"));
    Save_MPlayer_Values(pipe);
  }

  pipe.Flush();
}

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
 *   02/27/1996 JLB : Uses simpler game control value save operation.      *
 *=========================================================================*/
bool Save_Game(int id, const char* descr, bool /*unused*/) {
  char name[kMaxFname + kMaxExt];
  unsigned scenario;
  HousesType house;
  int save_net = 0;  // 1 = save network/modem game

  scenario = Scen.Scenario;         // get current scenario #
  house = PlayerPtr->Class->House;  // get current house

  /*
  **	Generate the filename to save.  If 'id' is -1, it means save a
  ** network/modem game; otherwise, use 'id' as the file extension.
  */
  if (id == -1) {
    port::SafeCopy(name, kNetSaveFileName);
    save_net = 1;
  } else {
    sprintf(name, "SAVEGAME.%03d", id);
  }

  /*
  **	Open the file
  */
  BufferIOFileClass file(name);

  FilePipe fpipe(&file);

  /*
  **	Save the description, scenario #, and house
  **	(scenario # & house are saved separately from the actual Scenario &
  **	PlayerPtr globals for convenience; we can quickly find out which
  **	house & scenario this save-game file is for by reading these values.
  **	Also, PlayerPtr is stored in a coded form in Save_Misc_Values(),
  **	which may or may not be a HousesType number; so, saving 'house'
  **	here ensures we can always pull out the house for this file.)
  */
  char descr_buf[kDescripMax];
  memset(descr_buf, '\0', sizeof(descr_buf));
  sprintf(descr_buf, "%s\r\n", descr);    // put CR-LF after text
  descr_buf[strlen(descr_buf) + 1] = 26;  // put CTRL-Z after nullptr
  fpipe.Put(descr_buf, kDescripMax);

  /*
  **	Magic and version come right after the description so the load dialog
  **	can reject a foreign or stale file without decrypting anything.
  */
  {
    ArchiveWriter header(fpipe);
    uint32_t magic = kSaveGameMagic;
    int32_t version = kSaveGameVersion;
    auto scenario32 = static_cast<int32_t>(scenario);
    header(magic, version, scenario32, house);
  }

  int pos = static_cast<int>(file.Seek(0, SEEK_CUR));

  /*
  **	Store a dummy message digest.
  */
  char digest[20];
  fpipe.Put(digest, sizeof(digest));

  /*
  **	Dump the save game data to the file. The data is compressed
  **	and then encrypted. The message digest is calculated in the
  **	process by using the data just as it is written to disk.
  */
  SHAPipe sha;
  BlowPipe bpipe(BlowPipe::ENCRYPT);
  LZOPipe pipe(LZOPipe::COMPRESS, SAVE_BLOCK_SIZE);
  //	LZWPipe pipe(LZWPipe::COMPRESS, SAVE_BLOCK_SIZE);
  //	LCWPipe pipe(LCWPipe::COMPRESS, SAVE_BLOCK_SIZE);
  bpipe.Key(&FastKey, BlowfishEngine::MAX_KEY_LENGTH);

  sha.SetSink(fpipe);
  bpipe.SetSink(sha);
  pipe.SetSink(bpipe);

  // Tee the field-wise body before compression. The dump has Section tags
  // but no save header, encryption, or digest, so it can be compared directly.
  RawFileClass dump_file;
  FilePipe dump_pipe(dump_file);
  bool dump_open = false;
  const char* dump_path = std::getenv("RA_SAVE_DUMP");
  if (dump_path != nullptr && dump_path[0] != '\0') {
    dump_file.Open(dump_path, FileAccess::kWrite);
    dump_open = dump_file.Is_Open() != 0;
    if (!dump_open) {
      DLOG(WARNING) << "Cannot open RA_SAVE_DUMP: " << dump_path;
    }
  }
  TeePipe tee(pipe, dump_open ? &dump_pipe : nullptr);
  Put_All(tee, save_net);
  if (!tee.copy_ok()) {
    DLOG(WARNING) << "Incomplete RA_SAVE_DUMP: " << dump_path;
  }
  dump_file.Close();

  /*
  **	Output the real final message digest. This is the one that is of
  **	the data image as it exists on the disk.
  */
  pipe.Flush();
  file.Seek(pos, SEEK_SET);
  sha.Result(digest);
  fpipe.Put(digest, sizeof(digest));

  pipe.End();

  return true;
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
 *   12/28/1994 BR : Created.
 ** 1/20/97  V.Grippi Added expansion CD check                            *
 *=========================================================================*/
bool Load_Game(int id) {
  char name[kMaxFname + kMaxExt];
  int i;
  HousesType house = HOUSE_NONE;
  char descr_buf[kDescripMax];
  int load_net = 0;  // 1 = save network/modem game

  /*
  **	Generate the filename to load.  If 'id' is -1, it means save a
  ** network/modem game; otherwise, use 'id' as the file extension.
  */
  if (id == -1) {
    port::SafeCopy(name, kNetSaveFileName);
    load_net = 1;
  } else {
    sprintf(name, "SAVEGAME.%03d", id);
  }

  /*
  **	Open the file
  */
  RawFileClass file(name);
  if (!file.Is_Available()) {
    return false;
  }

  FileStraw fstraw(file);

  Call_Back();

  /*
  **	Read & discard the save-game's header info
  */
  if (fstraw.Get(descr_buf, kDescripMax) != kDescripMax) {
    return false;
  }

  {
    ArchiveReader header(fstraw);
    uint32_t magic = 0;
    int32_t version = 0;
    int32_t scenario32 = 0;
    header(magic, version, scenario32, house);
    if (!header.ok() || magic != kSaveGameMagic ||
        version != kSaveGameVersion) {
      return false;
    }
    GameVersion = static_cast<unsigned long>(version);
  }
  /*
  **	Get the message digest that is embedded in the file.
  */
  char digest[20];
  fstraw.Get(digest, sizeof(digest));

  /*
  **	Remember the file position since we must seek back here to
  **	perform the real saved game read.
  */
  long pos = file.Seek(0, SEEK_CUR);

  /*
  **	Pass the rest of the file through the hash straw so that
  **	the digest can be compaired to the one in the file.
  */
  SHAStraw sha;
  sha.SetSource(fstraw);
  for (;;) {
    if (sha.Get(staging_buffer, sizeof(staging_buffer)) !=
        sizeof(staging_buffer)) {
      break;
    }
  }
  char actual[20];
  sha.Result(actual);
  sha.SetSource(nullptr);

  Call_Back();

  /*
  **	Compare the two digests. If they differ then return a failure condition
  **	before any damage could be done.
  */
  if (memcmp(actual, digest, sizeof(digest)) != 0) {
    return false;
  }

  /*
  **	Set up the pipe so that the scenario data can be read.
  */
  file.Seek(pos, SEEK_SET);
  BlowStraw bstraw(BlowStraw::DECRYPT);
  LZOStraw straw(LZOStraw::DECOMPRESS, SAVE_BLOCK_SIZE);
  //	LZWStraw straw(LZWStraw::DECOMPRESS, SAVE_BLOCK_SIZE);
  //	LCWStraw straw(LCWStraw::DECOMPRESS, SAVE_BLOCK_SIZE);

  bstraw.Key(&FastKey, BlowfishEngine::MAX_KEY_LENGTH);
  bstraw.SetSource(fstraw);
  straw.SetSource(bstraw);

  /*
  **	Clear the scenario so we start fresh; this calls the Init_Clear()
  *routine *	for the Map, and all object arrays.  It has the following
  *important *	effects: *	- Every cell is cleared to 0's, via
  *MapClass::Init_Clear() *	- All heap elements' are cleared *	- The
  *Houses are Initialized, which also clears their HouseTriggers *	  array
  **	- The map's Layers & Logic Layer are cleared to empty
  **	- The list of currently-selected objects is cleared
  */
  Clear_Scenario();

  if (!Get_Section(straw, FourCC("FRAM"))) {
    return false;
  }
  ArchiveReader reader(straw);
  reader(Frame);
  if (!reader.ok()) {
    return false;
  }

  /*
  **	Load the scenario global information.
  */
  if (!Get_Section(straw, FourCC("SCEN"))) {
    return false;
  }
  reader(Scen);
  if (!reader.ok()) {
    return false;
  }

  /*
  **	Fixup the Sessionclass scenario info so we can work out which
  ** CD to request later
  */
  if (load_net) {
    CCFileClass scenario_file(Scen.ScenarioName);
    if (!scenario_file.Is_Available()) {
      int cd = -1;
      if (IsMissionCounterstrike(Scen.ScenarioName)) {
        cd = 2;
        if (Expansion_AM_Present()) {
          int current_drive = CCFileClass::Get_CD_Drive();
          int index = Get_CD_Index(current_drive, 1 * 60);
          if (index == 3) {
            cd = 3;
          }
        }
      }
      if (IsMissionAftermath(Scen.ScenarioName)) {
        cd = 3;
#ifdef BOGUSCD
        cd = -1;
#endif
      }
      RequiredCD = cd;
      if (!Force_CD_Available(RequiredCD)) {
        Emergency_Exit(EXIT_FAILURE);
      }

      /*
      ** Update the internal list of scenarios to include the counterstrike
      ** list.
      */
      Session.Read_Scenario_Descriptions();
    } else {
      /*
      ** The scenario is available so set RequiredCD to whatever is currently
      ** in the drive.
      */
      int current_drive = CCFileClass::Get_CD_Drive();
      RequiredCD = Get_CD_Index(current_drive, 1 * 60);
    }
  }

  /*
  **	Load the map.  The map comes first, since it loads the Theater & init's
  **	mixfiles.  The map calls all the type-class's Init routines, telling
  *them *	what the Theater is; this must be done before any objects are
  *created, so *	they'll be properly created.
  */
  if (!Get_Section(straw, FourCC("MAP_"))) {
    return false;
  }
  if (!Map.Load(straw)) {
    return false;
  }

  Call_Back();

  /*
  **	Load the object data.
  */
  if (!Get_Section(straw, FourCC("HOUS")) || !Houses.Load(straw)) {
    return false;
  }
  if (!Get_Section(straw, FourCC("TMTY")) || !TeamTypes.Load(straw)) {
    return false;
  }
  if (!Get_Section(straw, FourCC("TEAM")) || !Teams.Load(straw)) {
    return false;
  }
  if (!Get_Section(straw, FourCC("TRTY")) || !TriggerTypes.Load(straw)) {
    return false;
  }
  if (!Get_Section(straw, FourCC("TRIG")) || !Triggers.Load(straw)) {
    return false;
  }
  if (!Get_Section(straw, FourCC("AIRC")) || !Aircraft.Load(straw)) {
    return false;
  }
  if (!Get_Section(straw, FourCC("ANIM")) || !Anims.Load(straw)) {
    return false;
  }
  if (!Get_Section(straw, FourCC("BLDG")) || !Buildings.Load(straw)) {
    return false;
  }
  if (!Get_Section(straw, FourCC("BULL")) || !Bullets.Load(straw)) {
    return false;
  }

  Call_Back();

  if (!Get_Section(straw, FourCC("INFT")) || !Infantry.Load(straw)) {
    return false;
  }
  if (!Get_Section(straw, FourCC("OVRL")) || !Overlays.Load(straw)) {
    return false;
  }
  if (!Get_Section(straw, FourCC("SMDG")) || !Smudges.Load(straw)) {
    return false;
  }
  if (!Get_Section(straw, FourCC("TMPL")) || !Templates.Load(straw)) {
    return false;
  }
  if (!Get_Section(straw, FourCC("TERR")) || !Terrains.Load(straw)) {
    return false;
  }
  if (!Get_Section(straw, FourCC("UNIT")) || !Units.Load(straw)) {
    return false;
  }
  if (!Get_Section(straw, FourCC("FACT")) || !Factories.Load(straw)) {
    return false;
  }
  if (!Get_Section(straw, FourCC("VESL")) || !Vessels.Load(straw)) {
    return false;
  }

  /*
  **	Load the Logic & Map Layers
  */
  if (!Get_Section(straw, FourCC("LOGC"))) {
    return false;
  }
  reader(Logic);
  if (!reader.ok()) {
    return false;
  }

  if (!Get_Section(straw, FourCC("TRGV"))) {
    return false;
  }
  SerializeTriggerLists(reader);
  if (!reader.ok()) {
    return false;
  }

  if (!Get_Section(straw, FourCC("LAYR"))) {
    return false;
  }
  for (i = 0; std::cmp_less(i, magic_enum::enum_count<LayerType>()); i++) {
    reader(MouseClass::Layer[i]);
    if (!reader.ok()) {
      return false;
    }
  }

  Call_Back();

  /*
  **	Load the Score
  */
  if (!Get_Section(straw, FourCC("SCOR"))) {
    return false;
  }
  reader(Score);
  if (!reader.ok()) {
    return false;
  }

  /*
  **	Load the AI Base
  */
  if (!Get_Section(straw, FourCC("BASE"))) {
    return false;
  }
  reader(Base);
  if (!reader.ok()) {
    return false;
  }

  if (!Get_Section(straw, FourCC("CARY"))) {
    return false;
  }
  SerializeCarryover(reader);
  if (!reader.ok()) {
    return false;
  }
  Call_Back();

  /*
  **	Load miscellaneous variables, including the map size & the Theater
  */
  if (!Get_Section(straw, FourCC("MISC"))) {
    return false;
  }
  if (!Load_Misc_Values(straw)) {
    return false;
  }

  /*
  **	Load multiplayer values
  */
  if (load_net) {
    if (!Get_Section(straw, FourCC("MPLY"))) {
      return false;
    }
    if (!Load_MPlayer_Values(straw)) {
      return false;
    }
  }

  file.Close();
  Whom = PlayerPtr->Class->House;
  if (Map.PendingObjectPtr) {
    Map.PendingObject = &Map.PendingObjectPtr->Class_Of();
    assert(Map.PendingObject != nullptr);
    Map.Set_Cursor_Shape(Map.PendingObject->Occupy_List(true));
#ifdef BG
    Map.Set_Placement_List(Map.PendingObject->Placement_List(true));
#endif
  } else {
    Map.PendingObject = nullptr;
    Map.Set_Cursor_Shape(nullptr);
  }
  Map.Init_IO();
  Map.Flag_To_Redraw(true);

  /*
  **	Fixup any expediency data that can be inferred from the physical
  **	data loaded.
  */
  Post_Load_Game(load_net);

  Call_Back();

  /*
  **	Set the required CD to be in the drive according to the scenario
  **	loaded.
  */
  if (RequiredCD != -2 && !load_net) {
    /*
    **	Determines if this an ant mission. Since the ant mission looks no
    *different from *	a regular mission, examining of the scenario name is the
    *only way to tell.
    */
    AntsEnabled = toupper(Scen.ScenarioName[0]) == 'S' &&
                  toupper(Scen.ScenarioName[1]) == 'C' &&
                  toupper(Scen.ScenarioName[2]) == 'A' &&
                  toupper(Scen.ScenarioName[3]) == '0' &&
                  toupper(Scen.ScenarioName[5]) == 'E' &&
                  toupper(Scen.ScenarioName[6]) == 'A';

    if (Scen.Scenario == 1) {
      RequiredCD = -1;
    } else {
      if (Scen.Scenario > 19 || AntsEnabled) {
        RequiredCD = 2;
        if (Scen.Scenario >= 36) {
          RequiredCD = 3;
#ifdef BOGUSCD
          RequiredCD = -1;
#endif
        }
      } else {
        if (PlayerPtr->Class->House != HOUSE_USSR &&
            PlayerPtr->Class->House != HOUSE_UKRAINE) {
          RequiredCD = 0;
        } else {
          RequiredCD = 1;
        }
      }
    }

  } else {
    if (load_net) {
      CCFileClass scenario_file(Scen.ScenarioName);

      /*
      ** Fix up the session class variables
      */
      for (int s = 0; s < Session.Scenarios.Count(); s++) {
        if (Session.Scenarios[s]->Description() == Scen.Description) {
          memcpy(Session.Options.ScenarioDescription, Scen.Description,
                 sizeof(Session.Options.ScenarioDescription));
          memcpy(Session.ScenarioFileName, Scen.ScenarioName,
                 sizeof(Session.ScenarioFileName));
          Session.ScenarioFileLength = static_cast<int>(scenario_file.Size());
          memcpy(Session.ScenarioDigest, Session.Scenarios[s]->Get_Digest(),
                 sizeof(Session.ScenarioDigest));
          Session.ScenarioIsOfficial = Session.Scenarios[s]->Get_Official();
          Scen.Scenario = s;
          Session.Options.ScenarioIndex = s;
          break;
        }
      }
    }
  }

  if (!Force_CD_Available(RequiredCD)) {
    // Prog_End();
    Emergency_Exit(EXIT_FAILURE);
  }

  ScenarioInit = 0;

  if (load_net) {
    if (!Reconcile_Players()) {  // (must do after Decode pointers)
      return false;
    }
    //!!!!!!!!!! put Fixup_Player_Units() here
    Session.LoadGame = true;
  }

  SidebarClass::Reload_Sidebar();  // re-load sidebar art.

  /*
  **	Rescan the scenario file for any rules updates.
  */
  CCINIClass ini;
  CCFileClass fc(Scen.ScenarioName);
  ini.Load(fc, true);

  /*
  **	Reset the rules values to their initial settings.
  */
  Rule.General(RuleINI);
  Rule.Recharge(RuleINI);
  Rule.AI(RuleINI);
  RulesClass::Powerups(RuleINI);
  RulesClass::Land_Types(RuleINI);
  RulesClass::Themes(RuleINI);
  Rule.IQ(RuleINI);
  RulesClass::Objects(RuleINI);
  Rule.Difficulty(RuleINI);
  Rule.General(AftermathINI);
  Rule.Recharge(AftermathINI);
  Rule.AI(AftermathINI);
  RulesClass::Powerups(AftermathINI);
  RulesClass::Land_Types(AftermathINI);
  RulesClass::Themes(AftermathINI);
  Rule.IQ(AftermathINI);
  RulesClass::Objects(AftermathINI);
  Rule.Difficulty(AftermathINI);

  /*
  **	Override any rules values specified in this
  **	particular scenario file.
  */
  Rule.General(ini);
  Rule.Recharge(ini);
  Rule.AI(ini);
  RulesClass::Powerups(ini);
  RulesClass::Land_Types(ini);
  RulesClass::Themes(ini);
  Rule.IQ(ini);
  RulesClass::Objects(ini);
  Rule.Difficulty(ini);
  if (load_net) {
    bool readini = false;
    switch (Session.Type) {
      case GAME_NORMAL:
        readini = false;
        break;
      case GAME_SKIRMISH:
        readini = Is_Aftermath_Installed();
        break;
      default:
        readini = bAftermathMultiplayer;
        break;
    }
    if (readini) {
      /*
      ** Find out if the CD in the current drive is the Aftermath disc.
      */
      if (Get_CD_Index(CCFileClass::Get_CD_Drive(), 60) != 3) {
        GamePalette.Set(kFadePaletteFast, Call_Back);
        // force Aftermath CD in drive.
        if (!Force_CD_Available(3)) {
          Emergency_Exit(EXIT_FAILURE);
        }
      }
      CCINIClass mpini;
      CCFileClass mplayer_ini("MPLAYER.INI");
      if (mpini.Load(mplayer_ini, false)) {
        Rule.General(mpini);
        Rule.Recharge(mpini);
        Rule.AI(mpini);
        RulesClass::Powerups(mpini);
        RulesClass::Land_Types(mpini);
        RulesClass::Themes(mpini);
        Rule.IQ(mpini);
        RulesClass::Objects(mpini);
        Rule.Difficulty(mpini);
      }
    }
  }
  if (Scen.TransitTheme == THEME_NONE) {
    Theme.Queue_Song(magic_enum::enum_values<ThemeType>().front());
  } else {
    Theme.Queue_Song(Scen.TransitTheme);
  }
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
 *   03/12/1996 JLB : Simplified.                                          *
 *=========================================================================*/
template <class Archive>
static void SerializeMisc(Archive& ar) {
  HousesType house = HOUSE_NONE;
  if constexpr (!Archive::kIsReading) {
    house = PlayerPtr->Class->House;
  }
  ar(house);
  if constexpr (Archive::kIsReading) {
    if (!ar.ok() || !magic_enum::enum_contains(house)) {
      ar.Fail("invalid player house");
      return;
    }
    PlayerPtr = HouseClass::As_Pointer(house);
    if (PlayerPtr == nullptr) {
      ar.Fail("player house is absent");
      return;
    }
  }
  SerializeObjectList(ar, CurrentObject);
  ar(ChronalVortex, IsTanyaDead, SaveTanya);
}

template <class Archive>
static void SerializeMultiplayer(Archive& ar) {
  ar(Session, BuildLevel, Debug_Unshroud, Seed, Whom, Special, Options);
}

bool Save_Misc_Values(Pipe& file) {
  ArchiveWriter writer(file);
  SerializeMisc(writer);
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
 * HISTORY: * 06/24/1995 BRR : Created. * 03/12/1996 JLB : Simplified. *
 *=============================================================================================*/
bool Load_Misc_Values(Straw& file) {
  ArchiveReader reader(file);
  SerializeMisc(reader);
  return reader.ok();
}

/***************************************************************************
 * Save_MPlayer_Values -- Saves multiplayer-specific values                *
 *                                                                         *
 * This routine saves multiplayer values that need to be restored for a * save
 *game.  In addition to saving the random # seed for this scenario, 	* it
 * saves the contents of the actual random number generator; this 	*
 * ensures that the random # sequencer will pick up where it left off when
 ** the game was saved.
 ** This routine also saves the header for a Recording file, so it must
 ** save some data not needed specifically by a save-game file (ie Seed).
 **
 *                                                                         *
 * INPUT:                                                                  *
 *		file		file to save to
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		true = success, false = failure
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   09/28/1995 BRR : Created.                                             *
 *=========================================================================*/
bool Save_MPlayer_Values(Pipe& file) {
  ArchiveWriter writer(file);
  SerializeMultiplayer(writer);
  return true;
}

/***************************************************************************
 * Load_MPlayer_Values -- Loads multiplayer-specific values                *
 *                                                                         *
 * INPUT:                                                                  *
 *		file			file to load from
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		true = success, false = failure
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   09/28/1995 BRR : Created.                                             *
 *=========================================================================*/
bool Load_MPlayer_Values(Straw& file) {
  ArchiveReader reader(file);
  SerializeMultiplayer(reader);
  return reader.ok();
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
bool Get_Savefile_Info(int id, char* buf, size_t buf_size, unsigned* scenp,
                       HousesType* housep) {
  char name[kMaxFname + kMaxExt];
  char descr_buf[kDescripMax];

  /*
  **	Generate the filename to load
  */
  sprintf(name, "SAVEGAME.%03d", id);
  BufferIOFileClass file(name);

  FileStraw straw(file);

  /*
  **	Read in the description, scenario #, and the house
  */
  if (straw.Get(descr_buf, kDescripMax) != kDescripMax) {
    return false;
  }

  descr_buf[strlen(descr_buf) - 2] = '\0';  // trim off CR/LF
  port::SafeCopy(buf, descr_buf, buf_size);

  ArchiveReader header(straw);
  uint32_t magic = 0;
  int32_t version = 0;
  int32_t scenario32 = 0;
  HousesType house = HOUSE_NONE;
  header(magic, version, scenario32, house);
  if (!header.ok() || magic != kSaveGameMagic || version != kSaveGameVersion) {
    return false;
  }
  *scenp = static_cast<unsigned>(scenario32);
  *housep = house;
  return true;
}

/***************************************************************************
 * Reconcile_Players -- Reconciles loaded data with the 'Players' vector
 **
 *                                                                         *
 * This function is for supporting loading a saved multiplayer game. * When the
 *game is loaded, we have to figure out which house goes with		* which
 *entry in the Players vector.  We also have to figure out if 		*
 * everyone who was originally in the game is still with us, and if not, * turn
 *their stuff over to the computer.
 **
 *                                                                         *
 * So, this function does the following:
 **
 * - For every name in 'Players', makes sure that name is in the House * array;
 *if not, it's a fatal error.
 **
 * - For every human-controlled house, makes sure there's a player
 ** with that name; if not, it turns that house over to the computer. *
 * - Fills in the Player's house ID
 **
 *                                                                         *
 * This assumes that each player MUST keep their name the same as it was
 ** when the game was saved!  It's also assumed that the network
 ** connections have not been formed yet, since Player[i]->Player.ID will
 ** be invalid until this routine has been called.
 **
 *                                                                         *
 * INPUT:                                                                  *
 *		none.
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		true = OK, false = error
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   09/29/1995 BRR : Created.                                             *
 *=========================================================================*/
static int Reconcile_Players() {
  int i;
  int found;
  HousesType house;
  HouseClass* housep;

  /*
  **	If there are no players, there's nothing to do.
  */
  if (Session.Players.Count() == 0) {
    return true;
  }

  /*
  **	Make sure every name we're connected to can be found in a House
  */
  for (i = 0; i < Session.Players.Count(); i++) {
    found = 0;
    for (house = HOUSE_MULTI1; house < HOUSE_MULTI1 + Session.MaxPlayers;
         house++) {
      housep = HouseClass::As_Pointer(house);
      if (!housep) {
        continue;
      }

      if (!stricmp(Session.Players[i]->Name, housep->IniName)) {
        found = 1;
        break;
      }
    }
    if (!found) {
      return false;
    }
  }

  //
  // Loop through all Houses; if we find a human-owned house that we're
  // not connected to, turn it over to the computer.
  //
  for (house = HOUSE_MULTI1; house < HOUSE_MULTI1 + Session.MaxPlayers;
       house++) {
    housep = HouseClass::As_Pointer(house);
    if (!housep) {
      continue;
    }

    //
    // Skip this house if it wasn't human to start with.
    //
    if (!housep->IsHuman) {
      continue;
    }

    //
    // Try to find this name in the Players vector; if it's found, set
    // its ID to this house.
    //
    found = 0;
    for (i = 0; i < Session.Players.Count(); i++) {
      if (!stricmp(Session.Players[i]->Name, housep->IniName)) {
        found = 1;
        Session.Players[i]->Player.ID = house;
        break;
      }
    }

    /*
    **	If this name wasn't found, remove it
    */
    if (!found) {
      /*
      **	Turn the player's house over to the computer's AI
      */
      housep->IsHuman = false;
      housep->IsStarted = true;
      //			housep->Smartness = IQ_MENSA;
      housep->IQ = Rule.MaxIQ;
      port::SafeCopy(housep->IniName, Text_String(TXT_COMPUTER));

      Session.NumPlayers--;
    }
  }

  //
  // If all went well, our Session.NumPlayers value should now equal the value
  // from the saved game, minus any players we removed.
  //
  return Session.NumPlayers == Session.Players.Count();
}

