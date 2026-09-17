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

#ifndef CNC_RED_ALERT_SDLLIB_TILE_H_
#define CNC_RED_ALERT_SDLLIB_TILE_H_

#include <cstddef>
#include <cstdint>
#include <span>

#include "absl/base/attributes.h"
#include "base/buffer.h"

struct IControl_Type {
  int16_t Width;      // Width of icons (pixels).
  int16_t Height;     // Height of icons (pixels).
  int16_t Count;      // Number of (logical) icons in this set.
  int16_t Allocated;  // Was this iconset allocated?
#ifndef TD
  int16_t MapWidth;   // Width of map (in icons).
  int16_t MapHeight;  // Height of map (in icons).
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
};

inline std::span<const std::byte> Get_Icon_Set_Map(
    std::span<const std::byte> iconset) {
  if (iconset.size() < sizeof(IControl_Type)) {
    return {};
  }
  IControl_Type control{};
  base::CopyBytes(base::ObjectBytes(control), iconset, sizeof(control));
  if (control.Map < 0 || static_cast<size_t>(control.Map) > iconset.size()) {
    return {};
  }
  return iconset.subspan(static_cast<size_t>(control.Map));
}

#endif  // CNC_RED_ALERT_SDLLIB_TILE_H_
