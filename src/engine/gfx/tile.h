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

#ifndef CNC_RED_ALERT_ENGINE_GFX_TILE_H_
#define CNC_RED_ALERT_ENGINE_GFX_TILE_H_

#include <cstddef>
#include <cstdint>
#include <span>

#include "absl/base/attributes.h"
#include "base/buffer.h"

// The icon set header of Red Alert.
struct IControl_Type {
  int16_t Width;      // Width of icons (pixels).
  int16_t Height;     // Height of icons (pixels).
  int16_t Count;      // Number of (logical) icons in this set.
  int16_t Allocated;  // Was this iconset allocated?
  int16_t MapWidth;   // Width of map (in icons).
  int16_t MapHeight;  // Height of map (in icons).
  int32_t Size;       // Size of entire iconset memory block.
  int32_t Icons;      // Offset from buffer start to icon data.
  int32_t Palettes;   // Offset from buffer start to palette data.
  int32_t Remaps;     // Offset from buffer start to remap index data.
  int32_t TransFlag;  // Offset for transparency flag table.
  int32_t ColorMap;  // Offset for color control value table.
  int32_t Map;       // Icon map offset (if present).
};

// The icon set header of Tiberian Dawn, which lacks Red Alert's MapWidth,
// MapHeight and ColorMap fields.
struct TiberianDawnIconSetHeader {
  int16_t width;       // Width of icons (pixels).
  int16_t height;      // Height of icons (pixels).
  int16_t count;       // Number of (logical) icons in this set.
  int16_t allocated;   // Was this IconSet allocated?
  int32_t size;        // Size of the whole IconSet.
  int32_t icons;       // Offset of the icon data.
  int32_t palettes;    // Offset of the palette data.
  int32_t remaps;      // Offset of the remap index data.
  int32_t trans_flag;  // Offset of the transparency flags.
  int32_t map;         // Offset of the icon map, if present.
};

// Which game's header an icon set starts with.
enum class IconSetLayout { kTiberianDawn, kRedAlert };

// Returns the icon map of `iconset`, whose header is `layout`'s: the part of
// the set from the header's map offset to its end. Empty if the set is too
// short to hold the header or the offset lies outside the set.
inline std::span<const std::byte> Get_Icon_Set_Map(
    std::span<const std::byte> iconset, const IconSetLayout layout) {
  int32_t map = 0;
  if (layout == IconSetLayout::kRedAlert) {
    if (iconset.size() < sizeof(IControl_Type)) {
      return {};
    }
    IControl_Type control{};
    base::CopyBytes(base::ObjectBytes(control), iconset, sizeof(control));
    map = control.Map;
  } else {
    if (iconset.size() < sizeof(TiberianDawnIconSetHeader)) {
      return {};
    }
    TiberianDawnIconSetHeader control{};
    base::CopyBytes(base::ObjectBytes(control), iconset, sizeof(control));
    map = control.map;
  }
  if (map < 0 || static_cast<size_t>(map) > iconset.size()) {
    return {};
  }
  return iconset.subspan(static_cast<size_t>(map));
}

#endif  // CNC_RED_ALERT_ENGINE_GFX_TILE_H_
