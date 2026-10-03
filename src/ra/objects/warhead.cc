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

/* $Header: /CounterStrike/WARHEAD.CPP 1     3/03/97 10:26a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : WARHEAD.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 05/20/96 *
 *                                                                                             *
 *                  Last Update : July 19, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * WarheadTypeClass::As_Pointer -- Convert a warhead type number
 *into a pointer.             * WarheadTypeClass::Read_INI -- Fetches the
 *warhead data from the INI database.             *
 *   WarheadTypeClass::WarheadTypeClass -- Default constructor for warhead
 *objects.            * WarheadTypeClass::operator delete -- Returns warhead
 *object back to special memory pool.  * WarheadTypeClass::operator new --
 *Allocate a warhead object from the special heap.        *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "ra/objects/warhead.h"

#include <cstring>
#include <string>

#include "base/strings/tokenizer.h"
#include "engine/file/ini_file.h"
#include "game/core/object_heap.h"
#include "game/sim/enums.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/core/defines.h"
#include "ra/core/type_heaps.h"

namespace ra {

/***************************************************************************
**	This is the warhead data object array.
*/

/***********************************************************************************************
 * WarheadTypeClass::WarheadTypeClass -- Default constructor for warhead
 *objects.              *
 *                                                                                             *
 *    This default constructor for a warhead object will fill in all the default
 *values        * for a warhead. It is presumed that these values will be normal
 *unless specifically       * overridden by the INI database. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/19/1996 JLB : Created. *
 *=============================================================================================*/
WarheadTypeClass::WarheadTypeClass(const char* name)
    : ID(TheTypeHeaps().warhead().SlotIndexOf(this)), IniName(name) {
  for (const game::ArmorType armor :
       magic_enum::enum_values<game::ArmorType>()) {
    Modifier.at(armor) = Fixed(1);
  }
}

/***********************************************************************************************
 * WarheadTypeClass::operator new -- Allocate a warhead object from the special
 *heap.          *
 *                                                                                             *
 *    This will allocate a warhead object from the special heap that is
 *maintained for         * this purpose. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to the newly allocated warhead type object.
 *If there is     * insufficient memory for the allocation, then NULL is
 *returned.                     *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/19/1996 JLB : Created. *
 *=============================================================================================*/
void* WarheadTypeClass::operator new(std::size_t /*unused*/) noexcept {
  return TheTypeHeaps().warhead().Allocate();
}

/***********************************************************************************************
 * WarheadTypeClass::operator delete -- Returns warhead object back to special
 *memory pool.    *
 *                                                                                             *
 *    This routine will return the warhead object to the memory pool from whence
 *it came.      *
 *                                                                                             *
 * INPUT:   pointer  -- Pointer to the warhead object to return to the memory
 *pool.            *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/19/1996 JLB : Created. *
 *=============================================================================================*/
void WarheadTypeClass::operator delete(void* pointer) {
  TheTypeHeaps().warhead().Free(static_cast<WarheadTypeClass*>(pointer));
}

/***********************************************************************************************
 * WarheadTypeClass::As_Pointer -- Convert a warhead type number into a pointer.
 **
 *                                                                                             *
 *    This will locate the warhead type specified and return a pointer to it. *
 *                                                                                             *
 * INPUT:   warhead  -- The warhead to convert into a pointer. *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to the warhead type object that is
 *represented by the       * warhead type number specified. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/19/1996 JLB : Created. *
 *=============================================================================================*/
WarheadTypeClass* WarheadTypeClass::As_Pointer(WarheadType warhead) {
  if (warhead != WARHEAD_NONE) {
    return TheTypeHeaps().warhead().ActiveAt(static_cast<int>(warhead));
  }
  return nullptr;
}

/***********************************************************************************************
 * WarheadTypeClass::Read_INI -- Fetches the warhead data from the INI database.
 **
 *                                                                                             *
 *    Use this routine to retrieve the data specific to this warhead type class
 *object from    * the INI database specified. Typical use of this is when
 *processing the rules.ini         * file. *
 *                                                                                             *
 * INPUT:   ini   -- Reference to the INI database to fetch the values from. *
 *                                                                                             *
 * OUTPUT:  bool; Was the warhead entry found and the data retrieved? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/19/1996 JLB : Created. *
 *=============================================================================================*/
bool WarheadTypeClass::Read_INI(engine::file::IniFile& ini) {
  if (ini.HasSection(Name())) {
    SpreadFactor = ini.GetInt(Name(), "Spread", SpreadFactor);
    IsWallDestroyer = ini.GetBool(Name(), "Wall", IsWallDestroyer);
    IsWoodDestroyer = ini.GetBool(Name(), "Wood", IsWoodDestroyer);
    IsTiberiumDestroyer = ini.GetBool(Name(), "Ore", IsTiberiumDestroyer);
    ExplosionSet = ini.GetInt(Name(), "Explosion", ExplosionSet);
    InfantryDeath = ini.GetInt(Name(), "InfDeath", InfantryDeath);

    const std::string verses =
        ini.GetString(Name(), "Verses", "100%%,100%%,100%%,100%%,100%%");
    if (!verses.empty()) {
      base::Tokenizer tokens(verses, ",");
      for (const game::ArmorType armor :
           magic_enum::enum_values<game::ArmorType>()) {
        Modifier.at(armor) = Fixed::FromString(tokens.Next());
      }
    }

    IsOrganic = Modifier.at(game::ARMOR_STEEL) == 0;
    return true;
  }
  return false;
}

}  // namespace ra
