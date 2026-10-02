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

#include "ra/objects/abstract.h"

#include <algorithm>

#include "game/sim/direction.h"
#include "ra/core/defines.h"
#include "ra/core/inline.h"
#include "ra/objects/building.h"
#include "ra/objects/target.h"
#include "ra/objects/type.h"

namespace ra {

AbstractClass::~AbstractClass() = default;

game::DirType AbstractClass::Direction(const TARGET target) const {
  return ra::Direction(Center_Coord(), As_Coord(target));
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

}  // namespace ra
