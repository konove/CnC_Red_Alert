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

// Determines the direct line distance to the target in leptons.
// For building targets, adjusts the distance by subtracting the building's
// average radius to get the distance to the building edge rather than center.
// Typically used for weapon range checks.
#include "td/abstract.h"

#include <algorithm>
#include <cstring>

#include "td/building.h"
#include "td/defines.h"
#include "td/target.h"
#include "td/type.h"

int AbstractClass::Distance(const TARGET target) const {
  const BuildingClass *obj = As_Building(target);
  int dist = Distance(As_Coord(target));

  // For buildings, adjust by average radius to get distance to edge.
  if (obj) {
    dist -= (obj->Class->Width() + obj->Class->Height()) * (0x100 / 4);
    dist = std::max(dist, 0);
  }

  return dist;
}

// Constructor for AbstractTypeClass.
// Initializes the display name and INI identifier for this object type.
AbstractTypeClass::AbstractTypeClass(const int name, char const *ini) {
  Name = name;
  strncpy(IniName, ini, sizeof(IniName));
  static_cast<char &>(IniName[sizeof(IniName) - 1]) = '\0';
}

RTTIType AbstractTypeClass::What_Am_I() const { return RTTI_ABSTRACTTYPE; }
COORDINATE AbstractTypeClass::Coord_Fixup(const COORDINATE coord) const {
  return coord;
}
int AbstractTypeClass::Full_Name() const { return Name; }
unsigned short AbstractTypeClass::Get_Ownable() const { return 0xffff; }
