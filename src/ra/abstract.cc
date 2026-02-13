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

#include "ra/abstract.h"

#include <algorithm>
#include <cassert>

#include "ra/building.h"
#include "ra/ccptr.h"
#include "ra/config.h"
#include "ra/defines.h"
#include "ra/externs.h"
#include "ra/inline.h"
#include "ra/target.h"
#include "ra/type.h"

void AbstractClass::Debug_Dump(MonoClass* mono) const {
  if constexpr (config::kCheatKeysEnabled) {
    assert(IsActive);

    mono->Set_Cursor(11, 5);
    mono->Printf("%08X", As_Target());
    mono->Set_Cursor(20, 1);
    mono->Printf("%08X", Coord);
    mono->Set_Cursor(29, 1);
    mono->Printf("%3d", Height);
    if (Owner() != HOUSE_NONE) {
      mono->Set_Cursor(1, 3);
      mono->Printf("%-18s",
                   Text_String(HouseTypeClass::As_Reference(Owner()).FullName));
    }
  }
}

DirType AbstractClass::Direction(const TARGET target) const {
  return ::Direction(Center_Coord(), As_Coord(target));
}

int AbstractClass::Distance(const TARGET target) const {
  const BuildingClass* obj = As_Building(target);
  int dist = Distance(As_Coord(target));

  // Subtract the building's average radius so weapon range checks measure
  // to the building edge rather than the center coordinate.
  if (obj) {
    dist -= (obj->Class->Width() + obj->Class->Height()) * (0x100 / 4);
    dist = std::max(dist, 0);
  }

  return dist;
}
