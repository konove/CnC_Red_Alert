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
#ifndef CNC_RED_ALERT_RA_SHAPE_DRAW_H_
#define CNC_RED_ALERT_RA_SHAPE_DRAW_H_

// File: Drawing and measuring game shapes.

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "ra/defines.h"
#include "ra/face.h"
#include "sdllib/shape.h"
#include "tech/rect.h"

// Draws a shape to the current logical page. Every shape draw in the game goes
// through here.
//
// x,y are pixel coordinates, interpreted according to flags, and window is the
// clipping window to draw within. fadingdata is required by SHAPE_FADING and
// ghostdata by SHAPE_GHOST; if either is omitted the display class's default
// table is substituted. rotation and scale (24.8 fixed point) take the slower
// rotate-and-scale path when either differs from its default.
void CC_Draw_Shape(std::span<const std::byte> shapefile, int shape_num, int x,
                   int y, WindowNumberType window, ShapeFlags_Type flags,
                   std::span<const uint8_t> fading_data = {},
                   std::span<const uint8_t> ghostdata = {},
                   DirType rotation = DIR_N, int32_t scale = 0x0100);

// Returns the smallest rectangle enclosing the shape's non-transparent pixels,
// positioned relative to the center of the shape's maximum extent. Used to
// tighten the map's dirty-rectangle logic.
//
// This is brute force and slow -- cache the result rather than recomputing it.
Rect Shape_Dimensions(std::span<const std::byte> shapedata, int shape_num);

// Renders the radar-map icons for a shape file, at zoomfactor pixels per map
// cell, and returns the buffer holding them.
//
// The first two bytes of the buffer are the icon width and height in cells;
// the icons themselves follow, frames of (width * height) icons each. Pass
// frames == -1 to build every frame in the shape file. Returns an empty buffer
// if shapefile is empty.
std::vector<unsigned char> Get_Radar_Icon(std::span<const std::byte> shapefile,
                                          int shape_num, int frames,
                                          int zoom_factor);

#endif  // CNC_RED_ALERT_RA_SHAPE_DRAW_H_
