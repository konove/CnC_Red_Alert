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

/* $Header:   F:\projects\c&c\vcs\code\layer.cpv   2.18   16 Oct 1995 16:50:14
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : LAYER.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : May 31, 1994 *
 *                                                                                             *
 *                  Last Update : March 10, 1995 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * LayerClass::Submit -- Adds an object to a layer list. *
 *   LayerClass::Sort -- Perform an incremental sort pass on the layer's
 *objects.              * LayerClass::Sorted_Add -- Adds object in sorted order
 *to layer.                           *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "td/map/layer.h"

#include <algorithm>
#include <utility>

#include "base/algorithm.h"
#include "base/array.h"
#include "td/objects/object.h"

namespace td {

LayerClass::~LayerClass() = default;

int LayerClass::ID(const ObjectClass* object) const {
  return static_cast<int>(base::IndexOf(objects_, object));
}

bool LayerClass::Delete(const ObjectClass* object) {
  return base::EraseFirst(objects_, object);
}

bool LayerClass::Submit(ObjectClass* object, bool sort) {
  if (sort) {
    return Sorted_Add(object);
  }
  objects_.push_back(object);
  return true;
}

// One pass only: the layer is re-sorted a little every frame rather than
// fully, so draw order settles over several frames.
void LayerClass::Sort() {
  for (int index = 0; index < Count() - 1; index++) {
    ObjectClass*& here = base::At(objects_, index);
    ObjectClass*& next = base::At(objects_, index + 1);
    if (*next < *here) {
      std::swap(here, next);
    }
  }
}

// A linear search, not std::upper_bound: Sort() leaves the layer only
// partly sorted, and inserting anywhere else would change draw order.
bool LayerClass::Sorted_Add(ObjectClass* const object) {
  const auto later = std::ranges::find_if(
      objects_,
      [object](const ObjectClass* other) { return *other > *object; });
  objects_.insert(later, object);
  return true;
}

}  // namespace td
