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

// The text table lookup.

#ifndef CNC_RED_ALERT_RA_CORE_INLINE_H_
#define CNC_RED_ALERT_RA_CORE_INLINE_H_

#include <iterator>
#include <span>

#include "base/array.h"
#include "engine/file/string_table.h"
#include "ra/assets/assets.h"
#include "ra/map/world.h"

namespace ra {

// Returns the text for the text number `text_id`, or nullptr when the table has
// no such entry. Three sources share the number space:
//   below 0     the scenario's name overrides, slot -text_id - 1, as
//               AbstractTypeClass::Full_Name() hands them out
//   0..999      the game's text table (the TXT_* constants)
//   1000 and up the developer text table, entry text_id - 1000
// The pointer is into the loaded table or the override and stays valid until
// the next scenario starts.
inline const char* TextString(int text_id) {
  if (text_id < 0 && -text_id <= std::ssize(TheWorld().name_override())) {
    return base::At(std::span(TheWorld().name_override()), -text_id - 1)
        .c_str();
  }

  // The developer table's numbers start at 1000 so they cannot collide with
  // the TXT_* constants.
  if (text_id < 1000) {
    return Extract_String(TheAssets().system_strings(), text_id).data();
  }
  return Extract_String(TheAssets().debug_strings(), text_id - 1000).data();
}

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_CORE_INLINE_H_
