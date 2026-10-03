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

/* $Header: /CounterStrike/OVERLAY.CPP 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : OVERLAY.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : May 17, 1994 *
 *                                                                                             *
 *                  Last Update : July 24, 1995 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * OverlayClass::Read_INI -- Reads the overlay data from an INI
 *file.                        * OverlayClass::Init -- Resets the overlay object
 *system.                                   * OverlayClass::Mark -- Marks the
 *overlay down on the map.                                  *
 *   OverlayClass::OverlayClass -- Overlay object constructor. *
 *   OverlayClass::delete -- Returns a overlay object to the pool. *
 *   OverlayClass::new -- Allocates a overlay object from pool *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "ra/objects/overlay.h"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <span>
#include <vector>

#include "absl/log/check.h"
#include "base/buffer.h"
#include "base/numeric.h"
#include "base/pooled.h"
#include "base/strings/number_parse.h"
#include "engine/codec/block_codec.h"
#include "engine/codec/lcw_sink.h"
#include "engine/codec/lcw_source.h"
#include "engine/file/ini_file.h"
#include "engine/stream/span_sink.h"
#include "engine/stream/span_source.h"
#include "game/core/object_heap.h"
#include "ra/core/coord.h"
#include "ra/core/defines.h"
#include "ra/core/object_heaps.h"
#include "ra/core/type_heaps.h"
#include "ra/data/ini_types.h"
#include "ra/editor/mapedit.h"
#include "ra/map/world.h"
#include "ra/net/session.h"
#include "ra/objects/building.h"
#include "ra/objects/cell.h"
#include "ra/objects/object.h"

namespace ra {

// Scratch space for packing and unpacking the OverlayPack INI block.
static char overlay_pack_buffer[32000];

HousesType OverlayClass::ToOwn = HOUSE_NONE;

/***********************************************************************************************
 * OverlayClass::Init -- Resets the overlay object system. *
 *                                                                                             *
 *    This routine resets the overlay object system. It is called * prior to
 *loading a new scenario. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/24/1994 JLB : Created. *
 *=============================================================================================*/
void OverlayClass::Init() { TheObjectHeaps().overlay().FreeAll(); }

/***********************************************************************************************
 * OverlayClass::new -- Allocates a overlay object from pool *
 *                                                                                             *
 *    This routine is used to allocate a overlay object from the * overlay
 *object pool. *
 *                                                                                             *
 * INPUT:   size  -- The size of a overlay object (not used). *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to an available overlay object. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/17/1994 JLB : Created. *
 *=============================================================================================*/
void* OverlayClass::operator new(std::size_t /*unused*/) noexcept {
  void* ptr = TheObjectHeaps().overlay().Allocate();
  if (ptr) {
    static_cast<OverlayClass*>(ptr)->IsActive = true;
  }
  return ptr;
}

/***********************************************************************************************
 * OverlayClass::delete -- Returns a overlay object to the pool. *
 *                                                                                             *
 *    This routine will return a overlay object to the overlay object * pool. A
 *overlay so returned is available for allocation again. *
 *                                                                                             *
 * INPUT:   ptr   -- Pointer to the object to be returned. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/17/1994 JLB : Created. *
 *=============================================================================================*/
void OverlayClass::operator delete(void* ptr) {
  if (ptr) {
    static_cast<OverlayClass*>(ptr)->IsActive = false;
  }
  TheObjectHeaps().overlay().Free(static_cast<OverlayClass*>(ptr));
}

/***********************************************************************************************
 * OverlayClass::OverlayClass -- Overlay object constructor. *
 *                                                                                             *
 *    This is the constructor for a overlay object. *
 *                                                                                             *
 * INPUT:   type  -- The overlay object this is to become. *
 *                                                                                             *
 *          pos   -- The position on the map to place the object. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/17/1994 JLB : Created. *
 *=============================================================================================*/
OverlayClass::OverlayClass(OverlayType type, Cell pos, HousesType house)
    : ObjectClass(RTTI_OVERLAY, TheObjectHeaps().overlay().SlotIndexOf(this)),
      Class(TheTypeHeaps().overlay().ActiveAt(static_cast<int>(type))) {
  if (pos != Cell::None()) {
    ToOwn = house;
    Unlimbo(pos.Center());
    ToOwn = HOUSE_NONE;
  }
}

/***********************************************************************************************
 * OverlayClass::Mark -- Marks the overlay down on the map. *
 *                                                                                             *
 *    This routine will place the overlay onto the map. The overlay object is
 *deleted by this  * operation. The map is updated to reflect the presence of
 *the overlay.                    *
 *                                                                                             *
 * INPUT:   mark  -- The type of marking to perform. Only MARK_DOWN is
 *supported.              *
 *                                                                                             *
 * OUTPUT:  bool; Was the overlay successfully marked? Failure occurs if it is
 *not being       * marked down. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/24/1994 JLB : Created. * 12/23/1994 JLB : Checks low level
 *legality before proceeding.                             *
 *=============================================================================================*/
bool OverlayClass::Mark(MarkType mark) {
  DCHECK_HEAP_SLOT(TheObjectHeaps().overlay(), this);
  DCHECK(IsActive);

  if (ObjectClass::Mark(mark) && (mark == MARK_DOWN)) {
    const Cell cell = Cell::Containing(Coord);
    CellClass* cellptr = &TheMap().at(cell);

    /*
    **	Walls have special logic when they are marked down.
    */
    if (Class->IsWall) {
      if (cellptr->Is_Clear_To_Build()) {
        cellptr->Overlay = Class->Type;
        cellptr->OverlayData = 0;
        cellptr->Redraw_Objects();
        cellptr->Wall_Update();
        // The original OR'd the MZONE_ enumerators (0 | 1), which only ever
        // named the normal zone; the crusher zone is what an uncrushable
        // overlay changes.
        TheMap().Zone_Reset(Class->IsCrushable
                                ? kZoneFlagNormal
                                : kZoneFlagNormal | kZoneFlagCrusher);

        /*
        **	Flag ownership of the cell if the 'global' ownership flag
        *indicates that this *	is necessary for the overlay.
        */
        if (ToOwn != HOUSE_NONE) {
          cellptr->Owner = ToOwn;
        }

      } else {
        delete this;
        return false;
      }
    } else {
      bool clear = false;
      if (!TheWorld().scenario_init()) {
        if (Class->Type == OVERLAY_WATER_CRATE) {
          clear = cellptr->Is_Clear_To_Move(SPEED_FLOAT, false, false);
        } else {
          if (Class->Type == OVERLAY_STEEL_CRATE ||
              Class->Type == OVERLAY_WOOD_CRATE) {
            clear = cellptr->Is_Clear_To_Move(SPEED_TRACK, false, false);
          } else {
            clear = cellptr->Is_Clear_To_Move(SPEED_TRACK, true, true);
          }
        }
      } else {
        clear = true;
      }

      if ((TheWorld().scenario_init() || cellptr->Overlay == OVERLAY_NONE) &&
          clear) {
        cellptr->Overlay = Class->Type;
        cellptr->OverlayData = 0;

        cellptr->Redraw_Objects();
        if (Class->Land == LAND_TIBERIUM) {
          cellptr->OverlayData = 1;
          cellptr->Tiberium_Adjust();
        }
      }
    }

    /*
    **	*****  Is this really needed?
    */
    cellptr->Recalc_Attributes();

    /*
    **	Remove the overlay and make sure the system thinks it was never
    *placed down!
    */
    TheMap().Overlap_Up(Cell::Containing(Coord), this);
    IsDown = false;
    IsInLimbo = true;

    delete this;
    return true;
  }

  return false;
}

/***********************************************************************************************
 * OverlayClass::Read_INI -- Reads the overlay data from an INI file. *
 *                                                                                             *
 *    This routine is used to load a scenario's overlay data. The overlay
 *objects are read     * from the INI file and then created on the map. *
 *                                                                                             *
 * INPUT:   buffer   -- Pointer to the INI file staging buffer. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   Requires that all the buildings be placed first, so the scan for
 *assigning wall * ownership to the nearest building will work. * HISTORY: *
 *   09/01/1994 JLB : Created. * 07/24/1995 JLB : Specifically forbid manual
 *crates in multiplayer scenarios.              *
 *=============================================================================================*/
void OverlayClass::Read_INI(engine::file::IniFile& ini) {
  if (TheWorld().new_ini_format() > 1) {
    const std::vector<std::byte> overlay_pack =
        ini.GetBase64Block("OverlayPack");

    if (!overlay_pack.empty()) {
      SpanSource bpipe(overlay_pack);
      LcwSource uncomp(CodecMode::kDecompress, bpipe);

      for (int cell_index = 0; cell_index < MAP_CELL_TOTAL; ++cell_index) {
        const Cell cell = Cell::FromIndex(cell_index);
        OverlayType classid = OVERLAY_NONE;

        uncomp.ReadObject(classid);

        if ((classid != OVERLAY_NONE) &&
            (TheSession().Type == GAME_NORMAL ||
             !OverlayTypeClass::As_Reference(classid).IsCrate) &&
            (cell.index() >= MAP_CELL_W &&
             cell.index() <= MAP_CELL_TOTAL - MAP_CELL_W))

        /*
        **	Don't allow placement of overlays on the top or bottom rows of
        **	the map.
        */
        {
          base::NewPooled<OverlayClass>(classid, cell);

          // Assign house ownership to cells with walls in 'em.
          if (OverlayTypeClass::As_Reference(classid).IsWall) {
            HousesType owner = HOUSE_NONE;
            int distance = 0x7FFFFFFF;
            for (int index = 0; index < TheObjectHeaps().building().Count();
                 index++) {
              const BuildingClass* building =
                  TheObjectHeaps().building().ActiveAt(index);
              const int newdist =
                  ra::Distance(building->Center_Coord(), cell.Center());
              if (newdist < distance) {
                distance = newdist;
                owner = building->Owner();
              }
            }
            TheMap().at(cell).Owner = owner;
          }
        }
      }
    }
  }

  if (TheWorld().new_ini_format() < 2 || ini.HasSection("Overlay")) {
    const int len = ini.EntryCount(INI_Name());
    for (int index = 0; index < len; index++) {
      const std::string_view entry = ini.EntryName(INI_Name(), index);
      const Cell cell =
          Cell::FromIndex(base::ParseIntegerOr<int16_t>(entry, 0));
      const OverlayType classid =
          GetOverlayType(ini, INI_Name(), entry, OVERLAY_NONE);

      /*
      **	Don't allow placement of crates in the multiplayer scenarios.
      */
      if ((classid != OVERLAY_NONE &&
           (TheSession().Type == GAME_NORMAL ||
            !OverlayTypeClass::As_Reference(classid).IsCrate)) &&
          (cell.index() >= MAP_CELL_W &&
           cell.index() <= MAP_CELL_TOTAL - MAP_CELL_W))
      /*
      **	Don't allow placement of overlays on the top or bottom rows of
      **	the map.
      */
      {
        base::NewPooled<OverlayClass>(classid, cell);

        // Assign house ownership to cells with walls in 'em.
        if (OverlayTypeClass::As_Reference(classid).IsWall) {
          HousesType owner = HOUSE_NONE;
          int distance = 0x7FFFFFFF;
          for (int j = 0; j < TheObjectHeaps().building().Count(); j++) {
            const BuildingClass* building =
                TheObjectHeaps().building().ActiveAt(j);
            const int newdist =
                ra::Distance(building->Center_Coord(), cell.Center());
            if (newdist < distance) {
              distance = newdist;
              owner = building->Owner();
            }
          }
          TheMap().at(cell).Owner = owner;
        }
      }
    }
  }
}

void OverlayClass::Write_INI(engine::file::IniFile& ini) {
  /*
  **	First, clear out all existing unit data from the ini file.
  */
  ini.Clear(INI_Name());
  ini.Clear("OverlayPack");

  SpanSink bpipe(std::as_writable_bytes(std::span(overlay_pack_buffer)));
  LcwSink comppipe(CodecMode::kCompress, bpipe);

  for (int index = 0; index < MAP_CELL_TOTAL; index++) {
    comppipe.WriteObject(TheMap().at(Cell::FromIndex(index)).Overlay);
  }
  comppipe.Finish();
  if (bpipe.bytes_written() > 0) {
    ini.PutBase64Block("OverlayPack",
                       base::ObjectBytes(overlay_pack_buffer)
                           .first(base::ToSize(bpipe.bytes_written())));
  }

  //	for (Cell index = 0; index < MAP_CELL_TOTAL; index++) {
  //		CellClass * cellptr = &Map[index];
  //		if (cellptr->Overlay != OVERLAY_NONE) {
  //			char	uname[10];
  //			sprintf(uname, "%d", index);
  //			ini.Put_Overlay(INI_Name(), uname, cellptr->Overlay);
  //		}
  //	}
}

}  // namespace ra
