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

/* $Header:   F:\projects\c&c\vcs\code\template.cpv   2.18   16 Oct 1995
 * 16:51:46   JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : TEMPLATE.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : May 17, 1994 *
 *                                                                                             *
 *                  Last Update : January 23, 1995 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * TemplateClass::As_Target -- Converts a template object into a
 *target number.              * TemplateClass::Init -- Resets the template
 *object system.                                 * TemplateClass::Mark -- Lifts
 *or drops a template object.                                  *
 *   TemplateClass::Read_INI -- Reads the scenario control INI file. *
 *   TemplateClass::Select -- Select the template object. *
 *   TemplateClass::TemplateClass -- Default constructor for template class
 *objects.           * TemplateClass::TemplateClass -- Template object
 *constructor.                              * TemplateClass::Unlimbo -- Places a
 *template object into the game/map system.              *
 *   TemplateClass::Write_INI -- Writes the template objects to the INI file. *
 *   TemplateClass::delete -- Returns a template object to the pool. *
 *   TemplateClass::new -- Allocates a template object from pool *
 *   TemplateClass::Validate -- validates template pointer
 **
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "td/objects/template.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <span>
#include <string_view>
#include <utility>

#include "absl/strings/str_format.h"
#include "base/array.h"
#include "base/numeric.h"
#include "base/pooled.h"
#include "base/strings/number_parse.h"
#include "base/strings/tokenizer.h"
#include "engine/file/ini_file.h"
#include "engine/gfx/tile.h"
#include "game/core/object_heap.h"
#include "td/core/config.h"
#include "td/core/defines.h"
#include "td/core/object_heaps.h"
#include "td/editor/mapedit.h"
#include "td/map/world.h"
#include "td/objects/cell.h"
#include "td/objects/type.h"
#include "td/startup/conquer.h"

namespace td {

/***********************************************************************************************
 * TemplateClass::Validate -- validates template pointer
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
int TemplateClass::Validate() const {
  if constexpr (config::kCheatKeysEnabled) {
    const int num = TheObjectHeaps().tmplate().SlotIndexOf(this);
    if (num < 0 || num >= kTemplateMax) {
      Validate_Error("TEMPLATE");
    }
    return 1;
  } else {
    return 1;
  }
}

/***********************************************************************************************
 * TemplateClass::Read_INI -- Reads the scenario control INI file. *
 *                                                                                             *
 *    This routine reads the scenario control INI file and creates all *
 *    templates specified therein. *
 *                                                                                             *
 * INPUT:   ini      -- The parsed scenario INI file. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/24/1994 JLB : Created. *
 *=============================================================================================*/
void TemplateClass::Read_INI(const engine::file::IniFile& ini) {
  char buf[128];  // Working string staging buffer.

  const int count = ini.EntryCount(INI_Name());
  for (int i = 0; i < count; ++i) {
    const char* key = ini.EntryName(INI_Name(), i);  // Entry key.
    const Cell cell = Cell::FromIndex(
        base::ParseIntegerOr<int16_t>(key, 0));  // Cell of building.
    ini.GetString(INI_Name(), key, "", buf);
    base::Tokenizer tokens(buf, ",\r\n");
    const TemplateType temp =
        TemplateTypeClass::From_Name(tokens.Next());  // Terrain type.
    if (temp != TEMPLATE_NONE) {
      base::NewPooled<TemplateClass>(temp, cell);
    }
  }
}

/***********************************************************************************************
 * TemplateClass::Write_INI -- Writes the template objects to the INI file. *
 *                                                                                             *
 *    This routine is used to write all the template objects out to the INI file
 *specified.    * It is used by the scenario editor when saving the game. *
 *                                                                                             *
 * INPUT:   ini   -- The INI database to write into.            *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/28/1994 JLB : Created. *
 *=============================================================================================*/
void TemplateClass::Write_INI(engine::file::IniFile& ini) {
  char uname[10];
  char buf[127];

  /*
  **	First, clear out all existing template data from the ini file.
  */
  ini.Clear(INI_Name());

  /*
  **	Find all templates and write them to the file.
  */
  for (int index = 0; index < MAP_CELL_TOTAL; index++) {
    CellClass* ptr = &TheMap().at(Cell::FromIndex(index));
    if (ptr->TType != TEMPLATE_NONE && ptr->TIcon == 0) {
      absl::SNPrintF(uname, sizeof(uname), "%03d", index);
      absl::SNPrintF(buf, sizeof(buf), "%s",
                     TemplateTypeClass::As_Reference(ptr->TType).IniName);
      ini.PutString(INI_Name(), uname, buf);
    }
  }
}

/***********************************************************************************************
 * TemplateClass::TemplateClass -- Default constructor for template class
 *objects.             *
 *                                                                                             *
 *    This is the default constructor for a template class object. This
 *construction method    * should NEVER be used by the game except as a
 *consequence of declaring an array of        * uninitialized template objects.
 **
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 01/23/1995 JLB : Created. *
 *=============================================================================================*/

/***********************************************************************************************
 * TemplateClass::As_Target -- Converts a template object into a target number.
 **
 *                                                                                             *
 *    This routine will convert a template object into a target number. Because
 *templates      * never exist as a template object in the game system, this
 *routine will never be called.  *
 *                                                                                             *
 * INPUT: *
 *                                                                                             *
 * OUTPUT: *
 *                                                                                             *
 * WARNINGS: *
 *                                                                                             *
 * HISTORY: * 09/25/1994 JLB : Created. *
 *=============================================================================================*/
Target TemplateClass::As_Target() const {
  Validate();
  return Target::Make(KIND_TEMPLATE,
                      TheObjectHeaps().tmplate().SlotIndexOf(this));
}

/***********************************************************************************************
 * TemplateClass::Init -- Resets the template object system. *
 *                                                                                             *
 *    This routine resets the template object system. It is called * prior to
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
void TemplateClass::Init() { TheObjectHeaps().tmplate().FreeAll(); }

/***********************************************************************************************
 * TemplateClass::Mark -- Lifts or drops a template object. *
 *                                                                                             *
 *    This routine handles placing or removing a template object. This * entails
 *marking the map as appropriate and redisplaying affected * cells. *
 *                                                                                             *
 * INPUT:   mark  -- The marking operation to perform. *
 *                                                                                             *
 * OUTPUT:  bool; Was the template successfully marked? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/17/1994 JLB : Created. * 12/23/1994 JLB : Examines low level
 *legality before processing.                           *
 *=============================================================================================*/
bool TemplateClass::Mark(MarkType mark) {
  Validate();
  static bool noup = false;
  const auto iset = Class->Get_Image_Data();
  if (!iset.empty() && ObjectClass::Mark(mark)) {
    const auto map = Get_Icon_Set_Map(iset, IconSetLayout::kTiberianDawn);

    for (int y = 0; std::cmp_less(y, Class->Height); y++) {
      for (int x = 0; std::cmp_less(x, Class->Width); x++) {
        const Cell cell = Cell::Containing(Coord).Offset((y * MAP_CELL_W) + x);
        if (TheMap().In_Radar(cell)) {
          CellClass* cellptr = &TheMap().at(cell);
          const int number = (y * Class->Width) + x;

          /*
          **	Determine if this logical icon actually maps to a real icon. If
          *no real *	icon is associated with this logical position, then
          *don't do any action *	since none is required.
          */
          const bool real =
              base::At(map, base::ToSize(number)) != std::byte{0xff};

          if (real) {
            /*
            **	Lift the terrain object from the map.
            */
            if ((mark == MARK_UP && !noup) &&
                (cellptr->TType == Class->Type &&
                 std::cmp_equal(cellptr->TIcon, number))) {
              cellptr->TType = TEMPLATE_NONE;
              cellptr->TIcon = 0;
            }

            /*
            **	Place the terrain object down.
            */
            if (mark == MARK_DOWN) {
              if (*this == TEMPLATE_CLEAR1) {
                cellptr->TType = TEMPLATE_NONE;
                cellptr->TIcon = 0;
              } else {
                cellptr->TType = Class->Type;
                //							cellptr->TIcon
                //= real;
                cellptr->TIcon = static_cast<unsigned char>(number);
              }
            }

            cellptr->Redraw_Objects();
            cellptr->Recalc_Attributes();
          }
        }
      }
    }

    /*
    **	When marking this template down onto the map, the map template numbers
    *are update *	but the template is removed from existence. Make sure
    *that the deletion of the *	template object doesn't also lift the template
    *numbers up from the map.
    */
    if (mark == MARK_DOWN) {
      noup = true;
      delete this;
      noup = false;
    }
    return true;
  }
  return false;
}

/***********************************************************************************************
 * TemplateClass::new -- Allocates a template object from pool *
 *                                                                                             *
 *    This routine is used to allocate a template object from the * template
 *object pool. *
 *                                                                                             *
 * INPUT:   size  -- The size of a template object (not used). *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to an available template object. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/17/1994 JLB : Created. *
 *=============================================================================================*/
void* TemplateClass::operator new(std::size_t /*unused*/) noexcept {
  void* ptr = TheObjectHeaps().tmplate().Allocate();
  if (ptr) {
    static_cast<TemplateClass*>(ptr)->IsActive = true;
  }
  return ptr;
}

/***********************************************************************************************
 * TemplateClass::delete -- Returns a template object to the pool. *
 *                                                                                             *
 *    This routine will return a template object to the template object * pool.
 *A template so returned is available for allocation again. *
 *                                                                                             *
 * INPUT:   ptr   -- Pointer to the object to be returned. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/17/1994 JLB : Created. *
 *=============================================================================================*/
void TemplateClass::operator delete(void* ptr) {
  if (ptr) {
    static_cast<TemplateClass*>(ptr)->IsActive = false;
  }
  TheObjectHeaps().tmplate().Free(static_cast<TemplateClass*>(ptr));
}

/***********************************************************************************************
 * TemplateClass::TemplateClass -- Template object constructor. *
 *                                                                                             *
 *    This is the constructor for a template object. *
 *                                                                                             *
 * INPUT:   type  -- The template object this is to become. *
 *                                                                                             *
 *          pos   -- The position on the map to place the object. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/17/1994 JLB : Created. *
 *=============================================================================================*/
TemplateClass::TemplateClass(TemplateType type, Cell pos)
    : Class(&TemplateTypeClass::As_Reference(type)) {
  if (pos != Cell::None()) {
    Unlimbo(pos.Center());
  }
}

}  // namespace td
