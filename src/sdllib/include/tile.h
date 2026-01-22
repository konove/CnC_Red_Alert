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

#ifndef TILE_H
#define TILE_H

#include <cstdint>

typedef struct {
  short Width;      // Width of icons (pixels).
  short Height;     // Height of icons (pixels).
  short Count;      // Number of (logical) icons in this set.
  short Allocated;  // Was this iconset allocated?
#ifndef TD
  short MapWidth;   // Width of map (in icons).
  short MapHeight;  // Height of map (in icons).
#endif
  int32_t Size;       // Size of entire iconset memory block.
  int32_t Icons;      // Offset from buffer start to icon data.
  int32_t Palettes;   // Offset from buffer start to palette data.
  int32_t Remaps;     // Offset from buffer start to remap index data.
  int32_t TransFlag;  // Offset for transparency flag table.
#ifndef TD
  int32_t ColorMap;  // Offset for color control value table.
#endif
  int32_t Map;  // Icon map offset (if present).
} IControl_Type;

inline void* Get_Icon_Set_Map(const void* iconset) {
  if (iconset != nullptr) {
    return (char*)iconset + ((IControl_Type*)iconset)->Map;
  }
  return nullptr;
}

#endif  // TILE_H
