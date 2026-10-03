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

/* $Header:   F:\projects\c&c\vcs\code\overlay.cpv   2.17   16 Oct 1995 16:50:44
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
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
 *file.                        * OverlayClass::Write_INI -- Writes the overlay
 *data to an INI file.                        * OverlayClass::delete -- Returns
 *a overlay object to the pool.                             * OverlayClass::Init
 *-- Resets the overlay object system.                                   *
 *   OverlayClass::new -- Allocates a overlay object from pool *
 *   OverlayClass::OverlayClass -- Overlay object constructor. *
 *   OverlayClass::Mark -- Marks the overlay down on the map. *
 *   OverlayClass::Validate -- validates overlay
 **
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "td/objects/overlay.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>

#include "absl/strings/str_format.h"
#include "base/pooled.h"
#include "base/strings/number_parse.h"
#include "base/strings/tokenizer.h"
#include "engine/file/ini_file.h"
#include "game/core/object_heap.h"
#include "game/sim/direction.h"
#include "td/ai/special.h"
#include "td/core/config.h"
#include "td/core/const.h"
#include "td/core/defines.h"
#include "td/core/inline.h"
#include "td/core/object_heaps.h"
#include "td/map/world.h"
#include "td/net/session.h"
#include "td/objects/cell.h"
#include "td/objects/type.h"
#include "td/startup/conquer.h"

namespace td {

HousesType OverlayClass::ToOwn = HOUSE_NONE;

/***********************************************************************************************
 * OverlayClass::Validate -- validates overlay
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
int OverlayClass::Validate() const {
  if constexpr (config::kCheatKeysEnabled) {
    const int num = TheObjectHeaps().overlay().SlotIndexOf(this);
    if (num < 0 || num >= kOverlayMax) {
      Validate_Error("OVERLAY");
    }
    return 1;
  } else {
    return 1;
  }
}

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
void OverlayClass::Init() {
  TheObjectHeaps().overlay().FreeAll();
  ToOwn = HOUSE_NONE;
}

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
    : Class(&OverlayTypeClass::As_Reference(type)) {
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
  Validate();
  if (ObjectClass::Mark(mark) && (mark == MARK_DOWN)) {
    const Cell cell = Cell::Containing(Coord);
    CellClass* cellptr = &TheMap().at(cell);

    /*
    **	Road placement occurs in two steps. First the foundation is
    *placed, but only *	on buildable terrain. Second, the road is
    *completed, but only if the foundation *	was previously placed.
    */
    if (*this == OVERLAY_ROAD) {
      if ((cellptr->Overlay == OVERLAY_ROAD && cellptr->OverlayData == 0) ||
          (cellptr->Overlay == OVERLAY_NONE && cellptr->Is_Generally_Clear())) {
        if (cellptr->Overlay == OVERLAY_ROAD) {
          cellptr->OverlayData = 1;
        } else {
          cellptr->OverlayData = 0;
        }
        cellptr->Overlay = Class->Type;
        cellptr->Redraw_Objects();
      }
    } else {
      /*
      **	Walls have special logic when they are marked down.
      */
      if (Class->IsWall) {
        if (cellptr->Is_Generally_Clear() &&
            cellptr->Overlay != OVERLAY_FLAG_SPOT) {
          cellptr->Overlay = Class->Type;
          cellptr->OverlayData = 0;
          cellptr->Redraw_Objects();
          cellptr->Wall_Update();

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
        if ((cellptr->Overlay == OVERLAY_NONE ||
             cellptr->Overlay == OVERLAY_SQUISH) &&
            !cellptr->Cell_Terrain() && Ground.at(cellptr->Land_Type()).Build) {
          /*
          **	Increment the global crate counter. This is used to regulate
          **	the crate generation.
          */
          if (Class->IsCrate) {
            TheWorld().crate_count()++;
          }

          /*
          **	Don't show the squish unless the gross flag is active.
          */
          if (!TheSpecial().IsGross && Class->Type != OVERLAY_SQUISH) {
            cellptr->Overlay = Class->Type;
            cellptr->OverlayData = 0;
          }
          cellptr->Redraw_Objects();
          if (Class->Land == LAND_TIBERIUM) {
            cellptr->OverlayData = 1;
            cellptr->Tiberium_Adjust();
          } else {
            if (*this == OVERLAY_CONCRETE) {
              Cell newcell{};

              /*
              **	Smudges go away when concrete is laid down.
              */
              cellptr->Smudge = SMUDGE_NONE;
              cellptr->SmudgeData = 0;
              cellptr->Concrete_Calc();

              /*
              **	Possibly add concrete to adjacent cells depending on
              *whether this *	concrete is in an odd or even row.
              */
              if (cell.x() % 2 != 0) {
                newcell = Adjacent_Cell(cellptr->Cell_Number(), game::FACING_W);
              } else {
                newcell = Adjacent_Cell(cellptr->Cell_Number(), game::FACING_E);
              }
              if (TheMap().at(newcell).Overlay != OVERLAY_CONCRETE) {
                Class->Create_And_Place(newcell);
              }

              /*
              **	The display attributes must be recalculated for all
              *adjacent *	cells since their shape can be altered by the
              *presence of *	concrete at this location.
              */
              static const game::FacingType _face[4] = {
                  game::FACING_N, game::FACING_E, game::FACING_S,
                  game::FACING_W};

              for (const auto& index : _face) {
                cellptr->Adjacent_Cell(index).Concrete_Calc();
              }
            }
          }
        }
      }

      /*
      **	*****  Is this really needed?
      */
      cellptr->Recalc_Attributes();
    }
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
 * INPUT:   ini      -- The parsed scenario INI file. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/01/1994 JLB : Created. * 07/24/1995 JLB : Specifically forbid
 *manual crates in multiplayer scenarios.              *
 *=============================================================================================*/
void OverlayClass::Read_INI(const engine::file::IniFile& ini) {
  const int count = ini.EntryCount(INI_Name());
  for (int i = 0; i < count; ++i) {
    const char* key = ini.EntryName(INI_Name(), i);  // Entry key.
    const Cell cell = Cell::FromIndex(base::ParseIntegerOr<int16_t>(key, 0));
    const std::string value = ini.GetString(INI_Name(), key, "");
    base::Tokenizer tokens(value, ",\n\r");
    const OverlayType classid = OverlayTypeClass::From_Name(tokens.Next());

    /*
    **	Don't allow placement of crates in the multiplayer scenarios.
    */
    if ((classid != OVERLAY_NONE &&
         (TheSession().type() == GAME_NORMAL ||
          !OverlayTypeClass::As_Reference(classid).IsCrate)) &&
        (cell.index() >= MAP_CELL_W &&
         cell.index() <= MAP_CELL_TOTAL - MAP_CELL_W))
    /*
    **	Don't allow placement of overlays on the top or bottom rows of
    **	the map.
    */
    {
      base::NewPooled<OverlayClass>(classid, cell);
    }
  }
}

/***********************************************************************************************
 * OverlayClass::Write_INI -- Writes the overlay data to an INI file. *
 *                                                                                             *
 *    This is used to output the overlay data to a scenario INI file. Typically,
 *this is       * only used by the scenario editor. *
 *                                                                                             *
 * INPUT:   ini   -- The INI database to write into.            *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/01/1994 JLB : Created. *
 *=============================================================================================*/
void OverlayClass::Write_INI(engine::file::IniFile& ini) {
  char uname[10];
  char buf[128];

  /*
  **	First, clear out all existing unit data from the ini file.
  */
  ini.Clear(INI_Name());

  /*
  **	Write the unit data out.
  */
  for (int index = 0; index < MAP_CELL_TOTAL; index++) {
    const CellClass* cellptr = &TheMap().at(Cell::FromIndex(index));

    if (cellptr->Overlay != OVERLAY_NONE) {
      absl::SNPrintF(uname, sizeof(uname), "%03d", index);
      absl::SNPrintF(buf, sizeof(buf), "%s",
                     OverlayTypeClass::As_Reference(cellptr->Overlay).IniName);
      ini.PutString(INI_Name(), uname, buf);
    }
  }
}

}  // namespace td
