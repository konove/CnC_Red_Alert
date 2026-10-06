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

#include "ra/saveload/saveload.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
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
#include "base/array.h"
#include "base/buffer.h"
#include "base/numeric.h"
#include "base/strings/safe_string.h"
#include "base/types.h"
#include "engine/codec/block_codec.h"
#include "engine/codec/lzo_sink.h"
#include "engine/codec/lzo_source.h"
#include "engine/crypto/blowfish.h"
#include "engine/crypto/blowfish_sink.h"
#include "engine/crypto/blowfish_source.h"
#include "engine/crypto/sha.h"
#include "engine/crypto/sha1_sink.h"
#include "engine/crypto/sha1_source.h"
#include "engine/file/disk_file.h"
#include "engine/file/disk_stream.h"
#include "engine/file/file_access.h"
#include "engine/file/game_file.h"
#include "engine/file/ini_file.h"
#include "engine/file/search_paths.h"
#include "engine/platform/env.h"
#include "engine/platform/platform.h"
#include "engine/stream/archive.h"
#include "engine/stream/byte_sink.h"
#include "engine/stream/seek_origin.h"
#include "engine/stream/stream_sink.h"
#include "engine/stream/stream_source.h"
#include "engine/stream/tee_sink.h"
#include "game/core/game_clock.h"
#include "game/core/object_heap.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/ai/carryover.h"
#include "ra/ai/prebuilt_base.h"
#include "ra/ai/scenario.h"
#include "ra/ai/team.h"
#include "ra/ai/team_type.h"
#include "ra/ai/trigger.h"
#include "ra/ai/trigger_type.h"
#include "ra/assets/assets.h"
#include "ra/assets/installation.h"
#include "ra/assets/palette.h"
#include "ra/assets/palettes.h"
#include "ra/assets/theme.h"
#include "ra/core/defines.h"
#include "ra/core/object_heaps.h"
#include "ra/core/special_flags.h"
#include "ra/core/target.h"
#include "ra/data/mission_id.h"
#include "ra/data/rules.h"
#include "ra/data/text.h"
#include "ra/data/text_ids.h"
#include "ra/debug/debug_state.h"
#include "ra/editor/mapedit.h"
#include "ra/map/layer.h"
#include "ra/map/mouse.h"
#include "ra/map/sidebar.h"
#include "ra/map/vortex.h"
#include "ra/map/world.h"
#include "ra/net/session.h"
#include "ra/objects/aircraft.h"
#include "ra/objects/anim.h"
#include "ra/objects/building.h"
#include "ra/objects/bullet.h"
#include "ra/objects/cell.h"
#include "ra/objects/factory.h"
#include "ra/objects/house.h"
#include "ra/objects/infantry.h"
#include "ra/objects/object.h"
#include "ra/objects/overlay.h"
#include "ra/objects/smudge.h"
#include "ra/objects/template.h"
#include "ra/objects/terrain.h"
#include "ra/objects/type.h"
#include "ra/objects/unit.h"
#include "ra/objects/vessel.h"
#include "ra/saveload/serialize.h"
#include "ra/screens/expand.h"
#include "ra/screens/goptions.h"
#include "ra/screens/score.h"
#include "ra/startup/conquer.h"
#include "ra/startup/game_state.h"
#include "ra/startup/startup.h"

namespace ra {

#define SAVE_BLOCK_SIZE 4096

/*
********************************** Defines **********************************
*/
static bool Reconcile_Players();

// Section tags bracket every top-level block of the save body. They cost
// four bytes each and turn a field-list mismatch into an error that names
// the block instead of garbage further down the stream.

// Logs why Load_Game() gave up on the saved block `tag` and returns false.
static bool Rejected(const ArchiveReader& reader, std::string_view tag) {
  LOG(ERROR) << "Cannot load saved game, block " << tag << ": "
             << reader.error();
  return false;
}

// Trigger lists load after the trigger heap, so targets can resolve directly.
template <class Archive>
// Appending invalidates the elements, not the vector the reference names.
// NOLINTBEGIN(clang-diagnostic-lifetime-safety-invalidation)
static void SerializeTriggerList(Archive& ar,
                                 std::vector<TriggerClass*>& list) {
  // NOLINTEND(clang-diagnostic-lifetime-safety-invalidation)
  auto count = static_cast<int32_t>(list.size());
  ar(count);
  if constexpr (Archive::kIsReading) {
    if (!ar.ok() || count < 0 ||
        count > TheObjectHeaps().trigger().Capacity()) {
      ar.Fail("invalid trigger list count");
      return;
    }
    list.clear();
    list.reserve(base::ToSize(count));
  }
  for (int i = 0; i < count; ++i) {
    Target target = Target{};
    if constexpr (!Archive::kIsReading) {
      target = base::At(list, i)->As_Target();
    }
    ar(target);
    if constexpr (Archive::kIsReading) {
      if (!ar.ok() || target.kind() != RTTI_TRIGGER ||
          target.value() >= TheObjectHeaps().trigger().Capacity()) {
        ar.Fail("invalid saved trigger target");
        return;
      }
      list.push_back(target.AsTrigger());
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
      Carryover object;
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
  writer.Section(FourCC("FRAM"));
  int64_t frame = game::CurrentFrame();
  writer(frame);

  /*
  **	Save the scenario global information.
  */
  writer.Section(FourCC("SCEN"));
  writer(TheScenario());

  /*
  **	Save the map.  The map must be saved first, since it saves the Theater.
  */
  if (!save_net) {
    ServiceBackgroundTasks();
  }
  writer.Section(FourCC("MAP_"));
  TheMap().Save(writer);

  if (!save_net) {
    ServiceBackgroundTasks();
  }

  /*
  **	Save all game objects.  This code saves every object that's stored in a
  **	ObjectHeap class.
  */
  writer.Section(FourCC("HOUS"));
  TheObjectHeaps().house().Save(writer);
  if (!save_net) {
    ServiceBackgroundTasks();
  }
  writer.Section(FourCC("TMTY"));
  TheObjectHeaps().team_type().Save(writer);
  if (!save_net) {
    ServiceBackgroundTasks();
  }
  writer.Section(FourCC("TEAM"));
  TheObjectHeaps().team().Save(writer);
  if (!save_net) {
    ServiceBackgroundTasks();
  }
  writer.Section(FourCC("TRTY"));
  TheObjectHeaps().trigger_type().Save(writer);
  if (!save_net) {
    ServiceBackgroundTasks();
  }
  writer.Section(FourCC("TRIG"));
  TheObjectHeaps().trigger().Save(writer);
  if (!save_net) {
    ServiceBackgroundTasks();
  }
  writer.Section(FourCC("AIRC"));
  TheObjectHeaps().aircraft().Save(writer);
  if (!save_net) {
    ServiceBackgroundTasks();
  }
  writer.Section(FourCC("ANIM"));
  TheObjectHeaps().anim().Save(writer);

  if (!save_net) {
    ServiceBackgroundTasks();
  }

  writer.Section(FourCC("BLDG"));
  TheObjectHeaps().building().Save(writer);
  if (!save_net) {
    ServiceBackgroundTasks();
  }
  writer.Section(FourCC("BULL"));
  TheObjectHeaps().bullet().Save(writer);
  if (!save_net) {
    ServiceBackgroundTasks();
  }
  writer.Section(FourCC("INFT"));
  TheObjectHeaps().infantry().Save(writer);
  if (!save_net) {
    ServiceBackgroundTasks();
  }
  writer.Section(FourCC("OVRL"));
  TheObjectHeaps().overlay().Save(writer);
  if (!save_net) {
    ServiceBackgroundTasks();
  }
  writer.Section(FourCC("SMDG"));
  TheObjectHeaps().smudge().Save(writer);
  if (!save_net) {
    ServiceBackgroundTasks();
  }
  writer.Section(FourCC("TMPL"));
  TheObjectHeaps().tmplate().Save(writer);
  if (!save_net) {
    ServiceBackgroundTasks();
  }
  writer.Section(FourCC("TERR"));
  TheObjectHeaps().terrain().Save(writer);
  if (!save_net) {
    ServiceBackgroundTasks();
  }
  writer.Section(FourCC("UNIT"));
  TheObjectHeaps().unit().Save(writer);
  if (!save_net) {
    ServiceBackgroundTasks();
  }
  writer.Section(FourCC("FACT"));
  TheObjectHeaps().factory().Save(writer);
  if (!save_net) {
    ServiceBackgroundTasks();
  }
  writer.Section(FourCC("VESL"));
  TheObjectHeaps().vessel().Save(writer);

  if (!save_net) {
    ServiceBackgroundTasks();
  }

  /*
  **	Save the Logic & Map layers
  */
  writer.Section(FourCC("LOGC"));
  writer(TheWorld().logic());

  writer.Section(FourCC("TRGV"));
  SerializeTriggerLists(writer);
  if (!save_net) {
    ServiceBackgroundTasks();
  }

  writer.Section(FourCC("LAYR"));
  for (const LayerType layer : magic_enum::enum_values<LayerType>()) {
    writer(MouseClass::Layer.at(layer));
  }

  if (!save_net) {
    ServiceBackgroundTasks();
  }

  /*
  **	Save the Score
  */
  writer.Section(FourCC("SCOR"));
  writer(TheWorld().score());
  if (!save_net) {
    ServiceBackgroundTasks();
  }

  /*
  **	Save the AI Base
  */
  writer.Section(FourCC("BASE"));
  writer(TheWorld().base());
  if (!save_net) {
    ServiceBackgroundTasks();
  }

  writer.Section(FourCC("CARY"));
  SerializeCarryover(writer);
  if (!save_net) {
    ServiceBackgroundTasks();
  }

  /*
  **	Save miscellaneous variables.
  */
  writer.Section(FourCC("MISC"));
  Save_Misc_Values(writer);

  if (!save_net) {
    ServiceBackgroundTasks();
  }

  /*
  **	Save multiplayer values
  */
  if (save_net) {
    writer.Section(FourCC("MPLY"));
    Save_MPlayer_Values(writer);
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
 *   The cell saves its own CellTrigger pointer, converted to a Target.    *
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
  char name[engine::platform::kMaxFname + engine::platform::kMaxExt];
  int save_net = 0;  // 1 = save network/modem game

  const int scenario = TheScenario().Scenario;   // get current scenario #
  HousesType house = ThePlayer()->Class->House;  // get current house

  /*
  **	Generate the filename to save.  If 'id' is -1, it means save a
  ** network/modem game; otherwise, use 'id' as the file extension.
  */
  if (id == -1) {
    base::SafeCopy(name, kNetSaveFileName);
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
  const std::string dump_path =
      engine::platform::GetEnv("RA_SAVE_DUMP").value_or("");
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
 * - ObjectHeap's Load() routine loads the # of objects stored, and loads  *
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
  char name[engine::platform::kMaxFname + engine::platform::kMaxExt];
  HousesType house = HOUSE_NONE;
  char descr_buf[kDescripMax];
  int load_net = 0;  // 1 = save network/modem game

  /*
  **	Generate the filename to load.  If 'id' is -1, it means save a
  ** network/modem game; otherwise, use 'id' as the file extension.
  */
  if (id == -1) {
    base::SafeCopy(name, kNetSaveFileName);
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
      LOG(ERROR) << "Cannot load " << name << ": not a version "
                 << kSaveGameVersion << " Red Alert save";
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
    LOG(ERROR) << "Cannot load " << name << ": digest mismatch";
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

  // One reader for the whole body, so the first failure's reason survives.
  ArchiveReader reader(straw);
  if (!reader.Section(FourCC("FRAM"))) {
    return Rejected(reader, "FRAM");
  }
  int64_t frame = 0;
  reader(frame);
  game::TheGameClock().set_frame(frame);
  if (!reader.ok()) {
    return Rejected(reader, "FRAM");
  }

  /*
  **	Load the scenario global information.
  */
  if (!reader.Section(FourCC("SCEN"))) {
    return Rejected(reader, "SCEN");
  }
  reader(TheScenario());
  if (!reader.ok()) {
    return Rejected(reader, "SCEN");
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
  if (!reader.Section(FourCC("MAP_"))) {
    return Rejected(reader, "MAP_");
  }
  if (!TheMap().Load(reader)) {
    return Rejected(reader, "MAP_");
  }

  ServiceBackgroundTasks();

  /*
  **	Load the object data.
  */
  if (!reader.Section(FourCC("HOUS")) ||
      !TheObjectHeaps().house().Load(reader)) {
    return Rejected(reader, "HOUS");
  }
  if (!reader.Section(FourCC("TMTY")) ||
      !TheObjectHeaps().team_type().Load(reader)) {
    return Rejected(reader, "TMTY");
  }
  if (!reader.Section(FourCC("TEAM")) ||
      !TheObjectHeaps().team().Load(reader)) {
    return Rejected(reader, "TEAM");
  }
  if (!reader.Section(FourCC("TRTY")) ||
      !TheObjectHeaps().trigger_type().Load(reader)) {
    return Rejected(reader, "TRTY");
  }
  if (!reader.Section(FourCC("TRIG")) ||
      !TheObjectHeaps().trigger().Load(reader)) {
    return Rejected(reader, "TRIG");
  }
  if (!reader.Section(FourCC("AIRC")) ||
      !TheObjectHeaps().aircraft().Load(reader)) {
    return Rejected(reader, "AIRC");
  }
  if (!reader.Section(FourCC("ANIM")) ||
      !TheObjectHeaps().anim().Load(reader)) {
    return Rejected(reader, "ANIM");
  }
  if (!reader.Section(FourCC("BLDG")) ||
      !TheObjectHeaps().building().Load(reader)) {
    return Rejected(reader, "BLDG");
  }
  if (!reader.Section(FourCC("BULL")) ||
      !TheObjectHeaps().bullet().Load(reader)) {
    return Rejected(reader, "BULL");
  }

  ServiceBackgroundTasks();

  if (!reader.Section(FourCC("INFT")) ||
      !TheObjectHeaps().infantry().Load(reader)) {
    return Rejected(reader, "INFT");
  }
  if (!reader.Section(FourCC("OVRL")) ||
      !TheObjectHeaps().overlay().Load(reader)) {
    return Rejected(reader, "OVRL");
  }
  if (!reader.Section(FourCC("SMDG")) ||
      !TheObjectHeaps().smudge().Load(reader)) {
    return Rejected(reader, "SMDG");
  }
  if (!reader.Section(FourCC("TMPL")) ||
      !TheObjectHeaps().tmplate().Load(reader)) {
    return Rejected(reader, "TMPL");
  }
  if (!reader.Section(FourCC("TERR")) ||
      !TheObjectHeaps().terrain().Load(reader)) {
    return Rejected(reader, "TERR");
  }
  if (!reader.Section(FourCC("UNIT")) ||
      !TheObjectHeaps().unit().Load(reader)) {
    return Rejected(reader, "UNIT");
  }
  if (!reader.Section(FourCC("FACT")) ||
      !TheObjectHeaps().factory().Load(reader)) {
    return Rejected(reader, "FACT");
  }
  if (!reader.Section(FourCC("VESL")) ||
      !TheObjectHeaps().vessel().Load(reader)) {
    return Rejected(reader, "VESL");
  }

  /*
  **	Load the Logic & Map Layers
  */
  if (!reader.Section(FourCC("LOGC"))) {
    return Rejected(reader, "LOGC");
  }
  reader(TheWorld().logic());
  if (!reader.ok()) {
    return Rejected(reader, "LOGC");
  }

  if (!reader.Section(FourCC("TRGV"))) {
    return Rejected(reader, "TRGV");
  }
  SerializeTriggerLists(reader);
  if (!reader.ok()) {
    return Rejected(reader, "TRGV");
  }

  if (!reader.Section(FourCC("LAYR"))) {
    return Rejected(reader, "LAYR");
  }
  for (const LayerType layer : magic_enum::enum_values<LayerType>()) {
    reader(MouseClass::Layer.at(layer));
    if (!reader.ok()) {
      return Rejected(reader, "LAYR");
    }
  }

  ServiceBackgroundTasks();

  /*
  **	Load the Score
  */
  if (!reader.Section(FourCC("SCOR"))) {
    return Rejected(reader, "SCOR");
  }
  reader(TheWorld().score());
  if (!reader.ok()) {
    return Rejected(reader, "SCOR");
  }

  /*
  **	Load the AI Base
  */
  if (!reader.Section(FourCC("BASE"))) {
    return Rejected(reader, "BASE");
  }
  reader(TheWorld().base());
  if (!reader.ok()) {
    return Rejected(reader, "BASE");
  }

  if (!reader.Section(FourCC("CARY"))) {
    return Rejected(reader, "CARY");
  }
  SerializeCarryover(reader);
  if (!reader.ok()) {
    return Rejected(reader, "CARY");
  }
  ServiceBackgroundTasks();

  /*
  **	Load miscellaneous variables, including the map size & the Theater
  */
  if (!reader.Section(FourCC("MISC"))) {
    return Rejected(reader, "MISC");
  }
  if (!Load_Misc_Values(reader)) {
    return Rejected(reader, "MISC");
  }

  /*
  **	Load multiplayer values
  */
  if (load_net) {
    if (!reader.Section(FourCC("MPLY"))) {
      return Rejected(reader, "MPLY");
    }
    if (!Load_MPlayer_Values(reader)) {
      return Rejected(reader, "MPLY");
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
    // ShutDownEngine();
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
  engine::file::IniFile ini;
  if (const auto fc = OpenGameFile(TheScenario().ScenarioName)) {
    ini.LoadVerified(*fc);
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
      engine::file::IniFile mpini;
      if (const auto mplayer_ini = OpenGameFile("MPLAYER.INI");
          mplayer_ini && mpini.Load(*mplayer_ini)) {
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
     TheWorld().whom(), TheSpecialFlags(), TheOptions());
  if constexpr (Archive::kIsReading) {
    TheDebugState().set_unshroud(unshroud);
  }
}

bool Save_Misc_Values(ArchiveWriter& writer) {
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
bool Load_Misc_Values(ArchiveReader& reader) {
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
bool Save_MPlayer_Values(ArchiveWriter& writer) {
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
bool Load_MPlayer_Values(ArchiveReader& reader) {
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
  char name[engine::platform::kMaxFname + engine::platform::kMaxExt];
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
  base::SafeCopy(buf.first(std::min(buf_size, buf.size())), descr_buf);

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
      base::SafeCopy(housep->IniName, TextString(TXT_COMPUTER));

      TheSession().NumPlayers--;
    }
  }

  //
  // If all went well, our Session.NumPlayers value should now equal the
  // value from the saved game, minus any players we removed.
  //
  return TheSession().NumPlayers == TheSession().Players.Count();
}

}  // namespace ra
