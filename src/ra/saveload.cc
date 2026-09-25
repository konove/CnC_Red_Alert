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

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/strings/match.h"
#include "absl/strings/str_format.h"
#include "engine/base/array.h"
#include "engine/base/buffer.h"
#include "engine/base/strings/safe_string.h"
#include "engine/base/types.h"
#include "engine/platform/env.h"
#include "engine/platform/platform.h"
#include "engine/stream/archive.h"
#include "engine/stream/byte_sink.h"
#include "engine/stream/byte_source.h"
#include "engine/stream/seek_origin.h"
#include "engine/stream/stream_sink.h"
#include "engine/stream/stream_source.h"
#include "engine/stream/tee_sink.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/aircraft.h"
#include "ra/anim.h"
#include "ra/assets.h"
#include "ra/base.h"
#include "ra/building.h"
#include "ra/bullet.h"
#include "ra/carry.h"
#include "ra/ccini.h"
#include "ra/ccptr.h"
#include "ra/cell.h"
#include "ra/conquer.h"
#include "ra/debug_state.h"
#include "ra/defines.h"
#include "ra/expand.h"
#include "ra/factory.h"
#include "ra/game_clock.h"
#include "ra/game_state.h"
#include "ra/goptions.h"
#include "ra/heap.h"
#include "ra/house.h"
#include "ra/infantry.h"
#include "ra/inline.h"
#include "ra/installation.h"
#include "ra/jshell.h"
#include "ra/layer.h"
#include "ra/mapedit.h"
#include "ra/mission_id.h"
#include "ra/mouse.h"
#include "ra/object.h"
#include "ra/object_heaps.h"
#include "ra/overlay.h"
#include "ra/palette.h"
#include "ra/palettes.h"
#include "ra/rules.h"
#include "ra/scenario.h"
#include "ra/score.h"
#include "ra/serialize.h"
#include "ra/session.h"
#include "ra/sidebar.h"
#include "ra/smudge.h"
#include "ra/special.h"
#include "ra/startup.h"
#include "ra/target.h"
#include "ra/team.h"
#include "ra/teamtype.h"
#include "ra/template.h"
#include "ra/terrain.h"
#include "ra/text_ids.h"
#include "ra/theme.h"
#include "ra/trigger.h"
#include "ra/trigtype.h"
#include "ra/type.h"
#include "ra/unit.h"
#include "ra/vector_dynamic.h"
#include "ra/vessel.h"
#include "ra/vortex.h"
#include "ra/world.h"
#include "tech/block_codec.h"
#include "tech/blowfish.h"
#include "tech/blowfish_sink.h"
#include "tech/blowfish_source.h"
#include "tech/disk_file.h"
#include "tech/disk_stream.h"
#include "tech/file_access.h"
#include "tech/game_file.h"
#include "tech/lzo_sink.h"
#include "tech/lzo_source.h"
#include "tech/search_paths.h"
#include "tech/sha.h"
#include "tech/sha1_sink.h"
#include "tech/sha1_source.h"

#define SAVE_BLOCK_SIZE 4096

/*
********************************** Defines **********************************
*/
static bool Reconcile_Players();

// Section tags bracket every top-level block of the save body. They cost
// four bytes each and turn a field-list mismatch into an error that names
// the block instead of garbage further down the stream.
static void Put_Section(ByteSink& pipe, uint32_t tag) {
  ArchiveWriter(pipe).Section(tag);
}
static bool Get_Section(ByteSource& straw, uint32_t tag) {
  ArchiveReader reader(straw);
  return reader.Section(tag);
}

// Trigger lists load after the trigger heap, so targets can resolve directly.
template <class Archive>
static void SerializeTriggerList(Archive& ar, DynamicVectorClass<TriggerClass*>& list) {
  auto count = static_cast<int32_t>(list.Count());
  ar(count);
  if constexpr (Archive::kIsReading) {
    if (!ar.ok() || count < 0 || count > TheObjectHeaps().trigger().Length()) {
      ar.Fail("invalid trigger list count");
      return;
    }
    list.Clear();
    // Clear() releases the storage and Add() grows it back ten slots at a
    // time, copying every element already there each time. The count is known
    // here, so take the storage in one step.
    if (count > 0) {
      list.Resize(count);
    }
  }
  for (int i = 0; i < count; ++i) {
    TARGET target = kTargetNone;
    if constexpr (!Archive::kIsReading) {
      target = list.at(i)->As_Target();
    }
    ar(target);
    if constexpr (Archive::kIsReading) {
      if (!ar.ok() || !Is_Target_Trigger(target) ||
          Target_Value(target) >= TheObjectHeaps().trigger().Length()) {
        ar.Fail("invalid saved trigger target");
        return;
      }
      list.Add(As_Trigger(target));
    }
  }
}

template <class Archive>
static void SerializeTriggerLists(Archive& ar) {
  SerializeTriggerList(ar, TheWorld().map_triggers());
  SerializeTriggerList(ar, TheWorld().logic_triggers());
  for (const HousesType house : magic_enum::enum_values<HousesType>()) {
    SerializeTriggerList(ar, TheWorld().house_triggers().at(house));
  }
}

template <class Archive>
static void SerializeCarryover(Archive& ar) {
  auto count = static_cast<int32_t>(TheWorld().carryover().size());
  ar(count);
  if constexpr (Archive::kIsReading) {
    if (!ar.ok() || count < 0 || count > MAP_CELL_TOTAL) {
      ar.Fail("invalid carryover count");
      return;
    }
    TheWorld().carryover().clear();
    for (int i = 0; i < count; ++i) {
      CarryoverClass object;
      ar(object);
      if (!ar.ok()) {
        return;
      }
      TheWorld().carryover().push_back(object);
    }
  } else {
    for (auto& object : TheWorld().carryover()) {
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
static void Put_All(ByteSink& pipe, int save_net) {
  ArchiveWriter writer(pipe);
  /*
  **	Frame goes first: every frame-based timer re-anchors to it when read.
  */
  Put_Section(pipe, FourCC("FRAM"));
  int64_t frame = CurrentFrame();
  writer(frame);

  /*
  **	Save the scenario global information.
  */
  Put_Section(pipe, FourCC("SCEN"));
  writer(TheScenario());

  /*
  **	Save the map.  The map must be saved first, since it saves the Theater.
  */
  if (!save_net) {
    ServiceBackgroundTasks();
  }
  Put_Section(pipe, FourCC("MAP_"));
  TheMap().Save(pipe);

  if (!save_net) {
    ServiceBackgroundTasks();
  }

  /*
  **	Save all game objects.  This code saves every object that's stored in a
  **	TFixedIHeap class.
  */
  Put_Section(pipe, FourCC("HOUS"));
  TheObjectHeaps().house().Save(pipe);
  if (!save_net) {
    ServiceBackgroundTasks();
  }
  Put_Section(pipe, FourCC("TMTY"));
  TheObjectHeaps().team_type().Save(pipe);
  if (!save_net) {
    ServiceBackgroundTasks();
  }
  Put_Section(pipe, FourCC("TEAM"));
  TheObjectHeaps().team().Save(pipe);
  if (!save_net) {
    ServiceBackgroundTasks();
  }
  Put_Section(pipe, FourCC("TRTY"));
  TheObjectHeaps().trigger_type().Save(pipe);
  if (!save_net) {
    ServiceBackgroundTasks();
  }
  Put_Section(pipe, FourCC("TRIG"));
  TheObjectHeaps().trigger().Save(pipe);
  if (!save_net) {
    ServiceBackgroundTasks();
  }
  Put_Section(pipe, FourCC("AIRC"));
  TheObjectHeaps().aircraft().Save(pipe);
  if (!save_net) {
    ServiceBackgroundTasks();
  }
  Put_Section(pipe, FourCC("ANIM"));
  TheObjectHeaps().anim().Save(pipe);

  if (!save_net) {
    ServiceBackgroundTasks();
  }

  Put_Section(pipe, FourCC("BLDG"));
  TheObjectHeaps().building().Save(pipe);
  if (!save_net) {
    ServiceBackgroundTasks();
  }
  Put_Section(pipe, FourCC("BULL"));
  TheObjectHeaps().bullet().Save(pipe);
  if (!save_net) {
    ServiceBackgroundTasks();
  }
  Put_Section(pipe, FourCC("INFT"));
  TheObjectHeaps().infantry().Save(pipe);
  if (!save_net) {
    ServiceBackgroundTasks();
  }
  Put_Section(pipe, FourCC("OVRL"));
  TheObjectHeaps().overlay().Save(pipe);
  if (!save_net) {
    ServiceBackgroundTasks();
  }
  Put_Section(pipe, FourCC("SMDG"));
  TheObjectHeaps().smudge().Save(pipe);
  if (!save_net) {
    ServiceBackgroundTasks();
  }
  Put_Section(pipe, FourCC("TMPL"));
  TheObjectHeaps().tmplate().Save(pipe);
  if (!save_net) {
    ServiceBackgroundTasks();
  }
  Put_Section(pipe, FourCC("TERR"));
  TheObjectHeaps().terrain().Save(pipe);
  if (!save_net) {
    ServiceBackgroundTasks();
  }
  Put_Section(pipe, FourCC("UNIT"));
  TheObjectHeaps().unit().Save(pipe);
  if (!save_net) {
    ServiceBackgroundTasks();
  }
  Put_Section(pipe, FourCC("FACT"));
  TheObjectHeaps().factory().Save(pipe);
  if (!save_net) {
    ServiceBackgroundTasks();
  }
  Put_Section(pipe, FourCC("VESL"));
  TheObjectHeaps().vessel().Save(pipe);

  if (!save_net) {
    ServiceBackgroundTasks();
  }

  /*
  **	Save the Logic & Map layers
  */
  Put_Section(pipe, FourCC("LOGC"));
  writer(TheWorld().logic());

  Put_Section(pipe, FourCC("TRGV"));
  SerializeTriggerLists(writer);
  if (!save_net) {
    ServiceBackgroundTasks();
  }

  Put_Section(pipe, FourCC("LAYR"));
  for (const LayerType layer : magic_enum::enum_values<LayerType>()) {
    writer(MouseClass::Layer.at(layer));
  }

  if (!save_net) {
    ServiceBackgroundTasks();
  }

  /*
  **	Save the Score
  */
  Put_Section(pipe, FourCC("SCOR"));
  writer(TheWorld().score());
  if (!save_net) {
    ServiceBackgroundTasks();
  }

  /*
  **	Save the AI Base
  */
  Put_Section(pipe, FourCC("BASE"));
  writer(TheWorld().base());
  if (!save_net) {
    ServiceBackgroundTasks();
  }

  Put_Section(pipe, FourCC("CARY"));
  SerializeCarryover(writer);
  if (!save_net) {
    ServiceBackgroundTasks();
  }

  /*
  **	Save miscellaneous variables.
  */
  Put_Section(pipe, FourCC("MISC"));
  Save_Misc_Values(pipe);

  if (!save_net) {
    ServiceBackgroundTasks();
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
bool Save_Game(int id, const std::string_view descr, bool /*unused*/) {
  char name[port::kMaxFname + port::kMaxExt];
  int save_net = 0;  // 1 = save network/modem game

  const int scenario = TheScenario().Scenario;   // get current scenario #
  HousesType house = ThePlayer()->Class->House;  // get current house

  /*
  **	Generate the filename to save.  If 'id' is -1, it means save a
  ** network/modem game; otherwise, use 'id' as the file extension.
  */
  if (id == -1) {
    port::SafeCopy(name, kNetSaveFileName);
    save_net = 1;
  } else {
    absl::SNPrintF(name, sizeof(name), "SAVEGAME.%03d", id);
  }

  /*
  **	Open the file
  */
  const std::unique_ptr<DiskStream> file =
      OpenDiskFile(name, FileAccess::kWrite);
  if (file == nullptr) {
    return false;
  }

  StreamSink fpipe(*file);

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
  base::FillBytes(base::ObjectBytes(descr_buf), '\0', sizeof(descr_buf));
  absl::SNPrintF(descr_buf, sizeof(descr_buf), "%s\r\n",
                 descr);                  // put CR-LF after text
  base::At(descr_buf, std::string_view(descr_buf).size() + 1) =
      26;  // put CTRL-Z after nullptr
  fpipe.Write(std::as_bytes(std::span(descr_buf)));

  /*
  **	Magic and version come right after the description so the load dialog
  **	can reject a foreign or stale file without decrypting anything.
  */
  {
    ArchiveWriter header(fpipe);
    uint32_t magic = kSaveGameMagic;
    int32_t version = kSaveGameVersion;
    int32_t scenario32 = scenario;
    header(magic, version, scenario32, house);
  }

  const base::ssize pos = file->Tell();

  /*
  **	Store a dummy message digest.
  */
  Sha1Digest digest{};
  fpipe.Write(digest);

  /*
  **	Dump the save game data to the file. The data is compressed
  **	and then encrypted. The message digest is calculated in the
  **	process by using the data just as it is written to disk.
  */
  Sha1Sink sha(fpipe);
  BlowfishSink bpipe(CipherMode::kEncrypt, sha);
  LzoSink pipe(CodecMode::kCompress, bpipe, SAVE_BLOCK_SIZE);
  bpipe.Key(base::ObjectBytes(TheAssets().mix_key())
                .first(BlowfishEngine::kMaxKeyLength));

  // Tee the field-wise body before compression. The dump has Section tags
  // but no save header, encryption, or digest, so it can be compared directly.
  const std::string dump_path = port::GetEnv("RA_SAVE_DUMP").value_or("");
  std::unique_ptr<DiskStream> dump_file;
  if (!dump_path.empty()) {
    dump_file = OpenDiskFile(dump_path, FileAccess::kWrite);
    if (dump_file == nullptr) {
      DLOG(WARNING) << "Cannot open RA_SAVE_DUMP: " << dump_path;
    }
  }
  std::optional<StreamSink> dump_pipe;
  if (dump_file != nullptr) {
    dump_pipe.emplace(*dump_file);
  }
  TeeSink tee(pipe, dump_pipe.has_value() ? &*dump_pipe : nullptr);
  Put_All(tee, save_net);
  if (!tee.copy_ok()) {
    DLOG(WARNING) << "Incomplete RA_SAVE_DUMP: " << dump_path;
  }

  /*
  **	Output the real final message digest. This is the one that is of
  **	the data image as it exists on the disk.
  */
  pipe.Flush();
  file->Seek(pos, SeekOrigin::kBegin);
  digest = sha.digest();
  fpipe.Write(digest);

  // Finish flushes the file and reports a failed write, so it runs even when
  // the tee already failed.
  const bool finished = pipe.Finish();
  return finished && tee.ok();
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
  char name[port::kMaxFname + port::kMaxExt];
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
    absl::SNPrintF(name, sizeof(name), "SAVEGAME.%03d", id);
  }

  /*
  **	Open the file
  */
  const std::unique_ptr<DiskStream> file = OpenDiskFile(name);
  if (file == nullptr) {
    return false;
  }

  StreamSource fstraw(*file);

  ServiceBackgroundTasks();

  /*
  **	Read & discard the save-game's header info
  */
  if (fstraw.Read(std::as_writable_bytes(std::span(descr_buf))) !=
      kDescripMax) {
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
  }
  /*
  **	Get the message digest that is embedded in the file.
  */
  Sha1Digest digest{};
  fstraw.Read(digest);

  /*
  **	Remember the file position since we must seek back here to
  **	perform the real saved game read.
  */
  const base::ssize pos = file->Tell();

  /*
  **	Pass the rest of the file through the hash straw so that
  **	the digest can be compaired to the one in the file.
  */
  Sha1Digest actual{};
  {
    Sha1Source sha(fstraw);
    std::array<std::byte, 4096> chunk{};
    for (;;) {
      if (sha.Read(chunk) != std::ssize(chunk)) {
        break;
      }
    }
    actual = sha.digest();
  }

  ServiceBackgroundTasks();

  /*
  **	Compare the two digests. If they differ then return a failure condition
  **	before any damage could be done.
  */
  if (actual != digest) {
    return false;
  }

  /*
  **	Set up the pipe so that the scenario data can be read.
  */
  file->Seek(pos, SeekOrigin::kBegin);
  BlowfishSource bstraw(CipherMode::kDecrypt, fstraw);
  LzoSource straw(CodecMode::kDecompress, bstraw, SAVE_BLOCK_SIZE);
  bstraw.Key(base::ObjectBytes(TheAssets().mix_key())
                 .first(BlowfishEngine::kMaxKeyLength));

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
  int64_t frame = 0;
  reader(frame);
  TheGameClock().set_frame(frame);
  if (!reader.ok()) {
    return false;
  }

  /*
  **	Load the scenario global information.
  */
  if (!Get_Section(straw, FourCC("SCEN"))) {
    return false;
  }
  reader(TheScenario());
  if (!reader.ok()) {
    return false;
  }

  /*
  **	Fixup the Sessionclass scenario info so we can work out which
  ** CD to request later
  */
  if (load_net) {
    if (!GameFileExists(TheScenario().ScenarioName)) {
      int cd = -1;
      if (IsMissionCounterstrike(TheScenario().ScenarioName)) {
        cd = 2;
        if (Expansion_AM_Present()) {
          const int current_drive = SearchPaths::current_cd_drive();
          const int index = Get_CD_Index(current_drive, 1 * 60);
          if (index == 3) {
            cd = 3;
          }
        }
      }
      if (IsMissionAftermath(TheScenario().ScenarioName)) {
        cd = 3;
#ifdef BOGUSCD
        cd = -1;
#endif
      }
      TheGameState().required_cd() = cd;
      if (!Force_CD_Available(TheGameState().required_cd())) {
        EmergencyExit(EXIT_FAILURE);
      }

      /*
      ** Update the internal list of scenarios to include the counterstrike
      ** list.
      */
      TheSession().Read_Scenario_Descriptions();
    } else {
      /*
      ** The scenario is available so set RequiredCD to whatever is currently
      ** in the drive.
      */
      const int current_drive = SearchPaths::current_cd_drive();
      TheGameState().required_cd() = Get_CD_Index(current_drive, 1 * 60);
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
  if (!TheMap().Load(straw)) {
    return false;
  }

  ServiceBackgroundTasks();

  /*
  **	Load the object data.
  */
  if (!Get_Section(straw, FourCC("HOUS")) ||
      !TheObjectHeaps().house().Load(straw)) {
    return false;
  }
  if (!Get_Section(straw, FourCC("TMTY")) ||
      !TheObjectHeaps().team_type().Load(straw)) {
    return false;
  }
  if (!Get_Section(straw, FourCC("TEAM")) ||
      !TheObjectHeaps().team().Load(straw)) {
    return false;
  }
  if (!Get_Section(straw, FourCC("TRTY")) ||
      !TheObjectHeaps().trigger_type().Load(straw)) {
    return false;
  }
  if (!Get_Section(straw, FourCC("TRIG")) ||
      !TheObjectHeaps().trigger().Load(straw)) {
    return false;
  }
  if (!Get_Section(straw, FourCC("AIRC")) ||
      !TheObjectHeaps().aircraft().Load(straw)) {
    return false;
  }
  if (!Get_Section(straw, FourCC("ANIM")) ||
      !TheObjectHeaps().anim().Load(straw)) {
    return false;
  }
  if (!Get_Section(straw, FourCC("BLDG")) ||
      !TheObjectHeaps().building().Load(straw)) {
    return false;
  }
  if (!Get_Section(straw, FourCC("BULL")) ||
      !TheObjectHeaps().bullet().Load(straw)) {
    return false;
  }

  ServiceBackgroundTasks();

  if (!Get_Section(straw, FourCC("INFT")) ||
      !TheObjectHeaps().infantry().Load(straw)) {
    return false;
  }
  if (!Get_Section(straw, FourCC("OVRL")) ||
      !TheObjectHeaps().overlay().Load(straw)) {
    return false;
  }
  if (!Get_Section(straw, FourCC("SMDG")) ||
      !TheObjectHeaps().smudge().Load(straw)) {
    return false;
  }
  if (!Get_Section(straw, FourCC("TMPL")) ||
      !TheObjectHeaps().tmplate().Load(straw)) {
    return false;
  }
  if (!Get_Section(straw, FourCC("TERR")) ||
      !TheObjectHeaps().terrain().Load(straw)) {
    return false;
  }
  if (!Get_Section(straw, FourCC("UNIT")) ||
      !TheObjectHeaps().unit().Load(straw)) {
    return false;
  }
  if (!Get_Section(straw, FourCC("FACT")) ||
      !TheObjectHeaps().factory().Load(straw)) {
    return false;
  }
  if (!Get_Section(straw, FourCC("VESL")) ||
      !TheObjectHeaps().vessel().Load(straw)) {
    return false;
  }

  /*
  **	Load the Logic & Map Layers
  */
  if (!Get_Section(straw, FourCC("LOGC"))) {
    return false;
  }
  reader(TheWorld().logic());
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
  for (const LayerType layer : magic_enum::enum_values<LayerType>()) {
    reader(MouseClass::Layer.at(layer));
    if (!reader.ok()) {
      return false;
    }
  }

  ServiceBackgroundTasks();

  /*
  **	Load the Score
  */
  if (!Get_Section(straw, FourCC("SCOR"))) {
    return false;
  }
  reader(TheWorld().score());
  if (!reader.ok()) {
    return false;
  }

  /*
  **	Load the AI Base
  */
  if (!Get_Section(straw, FourCC("BASE"))) {
    return false;
  }
  reader(TheWorld().base());
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
  ServiceBackgroundTasks();

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

  TheWorld().whom() = ThePlayer()->Class->House;
  if (TheMap().PendingObjectPtr) {
    TheMap().PendingObject = &TheMap().PendingObjectPtr->Class_Of();
    DCHECK(TheMap().PendingObject != nullptr);
    TheMap().Set_Cursor_Shape(TheMap().PendingObject->Occupy_List(true));
#ifdef BG
    TheMap().Set_Placement_List(TheMap().PendingObject->Placement_List(true));
#endif
  } else {
    TheMap().PendingObject = nullptr;
    TheMap().Set_Cursor_Shape({});
  }
  TheMap().Init_IO();
  TheMap().Flag_To_Redraw(true);

  /*
  **	Fixup any expediency data that can be inferred from the physical
  **	data loaded.
  */
  Post_Load_Game(load_net);

  ServiceBackgroundTasks();

  /*
  **	Set the required CD to be in the drive according to the scenario
  **	loaded.
  */
  if (TheGameState().required_cd() != -2 && !load_net) {
    /*
    **	Determines if this an ant mission. Since the ant mission looks no
    *different from *	a regular mission, examining of the scenario name is the
    *only way to tell.
    */
    TheWorld().ants_enabled() = toupper(TheScenario().ScenarioName[0]) == 'S' &&
                                toupper(TheScenario().ScenarioName[1]) == 'C' &&
                                toupper(TheScenario().ScenarioName[2]) == 'A' &&
                                toupper(TheScenario().ScenarioName[3]) == '0' &&
                                toupper(TheScenario().ScenarioName[5]) == 'E' &&
                                toupper(TheScenario().ScenarioName[6]) == 'A';

    if (TheScenario().Scenario == 1) {
      TheGameState().required_cd() = -1;
    } else {
      if (TheScenario().Scenario > 19 || TheWorld().ants_enabled()) {
        TheGameState().required_cd() = 2;
        if (TheScenario().Scenario >= 36) {
          TheGameState().required_cd() = 3;
#ifdef BOGUSCD
          TheGameState().required_cd() = -1;
#endif
        }
      } else {
        if (!IsSovietHouse(ThePlayer()->Class->House)) {
          TheGameState().required_cd() = 0;
        } else {
          TheGameState().required_cd() = 1;
        }
      }
    }

  } else {
    if (load_net) {
      /*
      ** Fix up the session class variables
      */
      for (int s = 0; s < TheSession().Scenarios.Count(); s++) {
        if (TheSession().Scenarios.at(s)->Description() ==
            TheScenario().Description) {
          base::CopyBytes(
              base::ObjectBytes(TheSession().Options.ScenarioDescription),
              base::ObjectBytes(TheScenario().Description),
              sizeof(TheSession().Options.ScenarioDescription));
          base::CopyBytes(base::ObjectBytes(TheSession().ScenarioFileName),
                          base::ObjectBytes(TheScenario().ScenarioName),
                          sizeof(TheSession().ScenarioFileName));
          TheSession().ScenarioFileLength =
              static_cast<decltype(TheSession().ScenarioFileLength)>(
                  GameFileSize(TheScenario().ScenarioName));
          base::CopyBytes(
              base::ObjectBytes(TheSession().ScenarioDigest),
              std::as_bytes(TheSession().Scenarios.at(s)->Get_Digest_Bytes()),
              sizeof(TheSession().ScenarioDigest));
          TheSession().ScenarioIsOfficial =
              TheSession().Scenarios.at(s)->Get_Official();
          TheScenario().Scenario = s;
          TheSession().Options.ScenarioIndex = s;
          break;
        }
      }
    }
  }

  if (!Force_CD_Available(TheGameState().required_cd())) {
    // Prog_End();
    EmergencyExit(EXIT_FAILURE);
  }

  TheWorld().scenario_init() = 0;

  if (load_net) {
    if (!Reconcile_Players()) {  // (must do after Decode pointers)
      return false;
    }
    //!!!!!!!!!! put Fixup_Player_Units() here
    TheSession().LoadGame = true;
  }

  SidebarClass::Reload_Sidebar();  // re-load sidebar art.

  /*
  **	Rescan the scenario file for any rules updates.
  */
  CCINIClass ini;
  if (const auto fc = OpenGameFile(TheScenario().ScenarioName)) {
    ini.Load(*fc, true);
  }

  /*
  **	Reset the rules values to their initial settings.
  */
  TheRules().General(TheRules().rule_ini());
  TheRules().Recharge(TheRules().rule_ini());
  TheRules().AI(TheRules().rule_ini());
  TheRules().Powerups(TheRules().rule_ini());
  TheRules().Land_Types(TheRules().rule_ini());
  RulesClass::Themes(TheRules().rule_ini());
  TheRules().IQ(TheRules().rule_ini());
  TheRules().Objects(TheRules().rule_ini());
  TheRules().Difficulty(TheRules().rule_ini());
  TheRules().General(TheRules().aftermath_ini());
  TheRules().Recharge(TheRules().aftermath_ini());
  TheRules().AI(TheRules().aftermath_ini());
  TheRules().Powerups(TheRules().aftermath_ini());
  TheRules().Land_Types(TheRules().aftermath_ini());
  RulesClass::Themes(TheRules().aftermath_ini());
  TheRules().IQ(TheRules().aftermath_ini());
  TheRules().Objects(TheRules().aftermath_ini());
  TheRules().Difficulty(TheRules().aftermath_ini());

  /*
  **	Override any rules values specified in this
  **	particular scenario file.
  */
  TheRules().General(ini);
  TheRules().Recharge(ini);
  TheRules().AI(ini);
  TheRules().Powerups(ini);
  TheRules().Land_Types(ini);
  RulesClass::Themes(ini);
  TheRules().IQ(ini);
  TheRules().Objects(ini);
  TheRules().Difficulty(ini);
  if (load_net) {
    bool readini = false;
    switch (TheSession().Type) {
      case GAME_NORMAL:
        readini = false;
        break;
      case GAME_SKIRMISH:
        readini = Is_Aftermath_Installed();
        break;
      case GameType::GAME_MODEM:
      case GameType::GAME_NULL_MODEM:
      case GameType::GAME_IPX:
      case GameType::GAME_INTERNET:
      default:
        readini = TheSession().IsAftermath;
        break;
    }
    if (readini) {
      /*
      ** Find out if the CD in the current drive is the Aftermath disc.
      */
      if (Get_CD_Index(SearchPaths::current_cd_drive(), 60) != 3) {
        ThePalettes().game_palette().Set(kFadePaletteFast, ServiceRealTime);
        // force Aftermath CD in drive.
        if (!Force_CD_Available(3)) {
          EmergencyExit(EXIT_FAILURE);
        }
      }
      CCINIClass mpini;
      if (const auto mplayer_ini = OpenGameFile("MPLAYER.INI");
          mplayer_ini && mpini.Load(*mplayer_ini, false)) {
        TheRules().General(mpini);
        TheRules().Recharge(mpini);
        TheRules().AI(mpini);
        TheRules().Powerups(mpini);
        TheRules().Land_Types(mpini);
        RulesClass::Themes(mpini);
        TheRules().IQ(mpini);
        TheRules().Objects(mpini);
        TheRules().Difficulty(mpini);
      }
    }
  }
  if (TheScenario().TransitTheme == THEME_NONE) {
    TheTheme().Queue_Song(magic_enum::enum_values<ThemeType>().front());
  } else {
    TheTheme().Queue_Song(TheScenario().TransitTheme);
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
    house = ThePlayer()->Class->House;
  }
  ar(house);
  if constexpr (Archive::kIsReading) {
    if (!ar.ok() || !magic_enum::enum_contains(house)) {
      ar.Fail("invalid player house");
      return;
    }
    ThePlayer() = HouseClass::As_Pointer(house);
    if (ThePlayer() == nullptr) {
      ar.Fail("player house is absent");
      return;
    }
  }
  SerializeObjectList(ar, TheWorld().current_object());
  ar(TheWorld().chronal_vortex(), TheWorld().is_tanya_dead(),
     TheWorld().save_tanya());
}

template <class Archive>
static void SerializeMultiplayer(Archive& ar) {
  // The switch keeps its place in the record, so a local stands in for it.
  bool unshroud = TheDebugState().unshroud();
  ar(TheSession(), TheWorld().build_level(), unshroud, TheWorld().seed(),
     TheWorld().whom(), TheSpecial(), TheOptions());
  if constexpr (Archive::kIsReading) {
    TheDebugState().set_unshroud(unshroud);
  }
}

bool Save_Misc_Values(ByteSink& file) {
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
bool Load_Misc_Values(ByteSource& file) {
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
 ** save some data not needed specifically by a save-game file (ie
 * Seed).
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
bool Save_MPlayer_Values(ByteSink& file) {
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
bool Load_MPlayer_Values(ByteSource& file) {
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
bool Get_Savefile_Info(int id, std::span<char> buf, size_t buf_size,
                       unsigned* scenp, HousesType* housep) {
  char name[port::kMaxFname + port::kMaxExt];
  char descr_buf[kDescripMax];

  /*
  **	Generate the filename to load
  */
  absl::SNPrintF(name, sizeof(name), "SAVEGAME.%03d", id);
  const std::unique_ptr<DiskStream> file = OpenDiskFile(name);
  if (file == nullptr) {
    return false;
  }

  StreamSource straw(*file);

  /*
  **	Read in the description, scenario #, and the house
  */
  if (straw.Read(std::as_writable_bytes(std::span(descr_buf))) != kDescripMax) {
    return false;
  }

  descr_buf[sizeof(descr_buf) - 1] = '\0';
  const auto length = std::string_view(descr_buf).size();
  if (length >= 2 && base::At(descr_buf, length - 2) == '\r' &&
      base::At(descr_buf, length - 1) == '\n') {
    base::At(descr_buf, length - 2) = '\0';
  }
  port::SafeCopy(buf.first(std::min(buf_size, buf.size())), descr_buf);

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
static bool Reconcile_Players() {
  int found = 0;
  HouseClass* housep = nullptr;

  /*
  **	If there are no players, there's nothing to do.
  */
  if (TheSession().Players.Count() == 0) {
    return true;
  }

  /*
  **	Make sure every name we're connected to can be found in a House
  */
  for (int i = 0; i < TheSession().Players.Count(); i++) {
    found = 0;
    for (HousesType house = HOUSE_MULTI1;
         static_cast<int>(house) <
         static_cast<int>(HOUSE_MULTI1) + TheSession().MaxPlayers;
         house++) {
      housep = HouseClass::As_Pointer(house);
      if (!housep) {
        continue;
      }

      if (absl::EqualsIgnoreCase(TheSession().Players.at(i)->Name,
                                 housep->IniName)) {
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
  for (HousesType house = HOUSE_MULTI1;
       static_cast<int>(house) <
       static_cast<int>(HOUSE_MULTI1) + TheSession().MaxPlayers;
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
    for (int i = 0; i < TheSession().Players.Count(); i++) {
      if (absl::EqualsIgnoreCase(TheSession().Players.at(i)->Name,
                                 housep->IniName)) {
        found = 1;
        TheSession().Players.at(i)->Player.ID = house;
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
      housep->IQ = TheRules().MaxIQ;
      port::SafeCopy(housep->IniName, Text_String(TXT_COMPUTER));

      TheSession().NumPlayers--;
    }
  }

  //
  // If all went well, our Session.NumPlayers value should now equal the
  // value from the saved game, minus any players we removed.
  //
  return TheSession().NumPlayers == TheSession().Players.Count();
}
