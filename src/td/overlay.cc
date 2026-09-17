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

#include "td/overlay.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <span>
#include <string_view>
#include <vector>

#include "absl/strings/str_format.h"
#include "port/tokenizer.h"
#include "td/cell.h"
#include "td/config.h"
#include "td/conquer.h"
#include "td/const.h"
#include "td/defines.h"
#include "td/externs.h"
#include "td/globals.h"
#include "td/heap.h"
#include "td/inline.h"
#include "td/profile.h"
#include "td/special.h"
#include "td/type.h"
#include "td/vector.h"
#include "tech/number_parse.h"

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
    const int num = Overlays.ID(this);
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
  Overlays.Free_All();
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
void* OverlayClass::operator new(size_t /*unused*/) noexcept {
  void* ptr = Overlays.Allocate();
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
  Overlays.Free(static_cast<OverlayClass*>(ptr));
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
OverlayClass::OverlayClass(OverlayType type, CELL pos, HousesType house)
    : Class(&OverlayTypeClass::As_Reference(type)) {
  if (pos != -1) {
    ToOwn = house;
    Unlimbo(Cell_Coord(pos));
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
    const CELL cell = Coord_Cell(Coord);
    CellClass* cellptr = &Map.at(cell);

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
            CrateCount++;
          }

          /*
          **	Don't show the squish unless the gross flag is active.
          */
          if (!Special.IsGross && Class->Type != OVERLAY_SQUISH) {
            cellptr->Overlay = Class->Type;
            cellptr->OverlayData = 0;
          }
          cellptr->Redraw_Objects();
          if (Class->Land == LAND_TIBERIUM) {
            cellptr->OverlayData = 1;
            cellptr->Tiberium_Adjust();
          } else {
            if (*this == OVERLAY_CONCRETE) {
              CELL newcell = 0;

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
              if (Cell_X(cell) % 2 != 0) {
                newcell = Adjacent_Cell(cellptr->Cell_Number(), FACING_W);
              } else {
                newcell = Adjacent_Cell(cellptr->Cell_Number(), FACING_E);
              }
              if (Map.at(newcell).Overlay != OVERLAY_CONCRETE) {
                Class->Create_And_Place(newcell);
              }

              /*
              **	The display attributes must be recalculated for all
              *adjacent *	cells since their shape can be altered by the
              *presence of *	concrete at this location.
              */
              static const FacingType _face[4] = {FACING_N, FACING_E, FACING_S,
                                                  FACING_W};

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
 * INPUT:   buffer   -- Pointer to the INI file staging buffer. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/01/1994 JLB : Created. * 07/24/1995 JLB : Specifically forbid
 *manual crates in multiplayer scenarios.              *
 *=============================================================================================*/
void OverlayClass::Read_INI(char* buffer) {
  char buf[128];

  std::vector<char> key_storage(std::string_view(buffer).size() + 2);
  auto key_cursor = std::span(key_storage);
  char* tbuffer = key_cursor.data();

  WWGetPrivateProfileString(INI_Name(), nullptr, nullptr, key_cursor, buffer);
  while (*tbuffer != '\0') {
    CELL const cell = tech::ParseInteger<CELL>(tbuffer).value_or(0);
    WWGetPrivateProfileString(
        INI_Name(), tbuffer, nullptr,
        std::span(buf).first(static_cast<std::size_t>(sizeof(buf) - 1)),
        buffer);
    port::Tokenizer tokens(buf, ",\n\r");
    const OverlayType classid = OverlayTypeClass::From_Name(tokens.Next());

    /*
    **	Don't allow placement of crates in the multiplayer scenarios.
    */
    if ((classid != OVERLAY_NONE &&
         (GameToPlay == GAME_NORMAL ||
          !OverlayTypeClass::As_Reference(classid).IsCrate)) &&
        (cell >= MAP_CELL_W && cell <= MAP_CELL_TOTAL - MAP_CELL_W))
    /*
    **	Don't allow placement of overlays on the top or bottom rows of
    **	the map.
    */
    {
      new OverlayClass(classid, cell);
    }

    key_cursor = key_cursor.subspan(std::string_view(tbuffer).size() + 1);
    tbuffer = key_cursor.data();
  }
}

/***********************************************************************************************
 * OverlayClass::Write_INI -- Writes the overlay data to an INI file. *
 *                                                                                             *
 *    This is used to output the overlay data to a scenario INI file. Typically,
 *this is       * only used by the scenario editor. *
 *                                                                                             *
 * INPUT:   buffer   -- Pointer to the INI file staging buffer. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/01/1994 JLB : Created. *
 *=============================================================================================*/
void OverlayClass::Write_INI(std::span<char> buffer) {
  char uname[10];
  char buf[128];

  /*
  **	First, clear out all existing unit data from the ini file.
  */
  std::vector<char> key_storage(std::string_view(buffer.data()).size() + 2);
  auto key_cursor = std::span(key_storage);
  char* tbuffer = key_cursor.data();  // Accumulation buffer of unit IDs.
  WWGetPrivateProfileString(INI_Name(), nullptr, nullptr, key_cursor,
                            buffer.data());
  while (*tbuffer != '\0') {
    WWWritePrivateProfileString(INI_Name(), tbuffer, nullptr, buffer);
    key_cursor = key_cursor.subspan(std::string_view(tbuffer).size() + 1);
    tbuffer = key_cursor.data();
  }

  /*
  **	Write the unit data out.
  */
  for (int index = 0; index < MAP_CELL_TOTAL; index++) {
    const CellClass* cellptr = &Map.at(index);

    if (cellptr->Overlay != OVERLAY_NONE) {
      absl::SNPrintF(uname, sizeof(uname), "%03d", index);
      absl::SNPrintF(buf, sizeof(buf), "%s",
                     OverlayTypeClass::As_Reference(cellptr->Overlay).IniName);
      WWWritePrivateProfileString(INI_Name(), uname, buf, buffer);
    }
  }
}
