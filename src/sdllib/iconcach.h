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

// Hooks of the Windows 95 library's icon cache (Steve Tall, November 1995),
// which copied the terrain icon
// sets into video memory so they could be drawn by the blitter. The port draws
// every set from system memory (PixelView::DrawStamp), so the hooks are empty
// and the constants and IconSetType below have no users.

#ifndef CNC_RED_ALERT_SDLLIB_ICONCACH_H_
#define CNC_RED_ALERT_SDLLIB_ICONCACH_H_

#include "sdllib/tile.h"

#define ICON_WIDTH 24         // Icons must be this width to be cached
#define ICON_HEIGHT 24        // Icons must be this height to be cached
#define MAX_CACHED_ICONS 500  // Maximum number of icons that can be cached
#define MAX_ICON_SETS 100  // Maximum number of icon sets that can be registered
#define MAX_LOOKUP_ENTRIES 3000  // Size of icon index table

// A registered icon set and where its tiles start in the cache's lookup table.
struct IconSetType {
  IControl_Type* IconSetPtr;  // Ptr to icon set data
  int IconListOffset;         // Offset into icon index table for this icon set
};

// Was: copy the cached icons back into video memory, which the system may
// have discarded while the game was in the background. Now only prints its
// name to stdout.
extern void Restore_Cached_Icons();
// Was: add `icon_data` to the sets the cache knows, and copy its icons into
// video memory straight away if `pre_cache`. Now does nothing.
extern void Register_Icon_Set(const void* icon_data, bool pre_cache);

#endif  // CNC_RED_ALERT_SDLLIB_ICONCACH_H_
