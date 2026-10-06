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
#ifndef CNC_RED_ALERT_RA_ASSETS_SHAPE_DRAW_H_
#define CNC_RED_ALERT_RA_ASSETS_SHAPE_DRAW_H_

// File: Drawing and measuring game shapes.

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/rect.h"
#include "engine/gfx/shape.h"
#include "game/sim/direction.h"
#include "ra/core/defines.h"

namespace ra {

// Draws a shape into `view`. Every shape draw in the game goes through here.
//
// x,y are pixel coordinates, interpreted according to flags, and window is the
// clipping window within `view` to draw inside. fadingdata is required by
// ShapeFlags::kFading and ghostdata by ShapeFlags::kGhost; if either is omitted
// the display class's default table is substituted. rotation and scale (24.8
// fixed point) take the slower rotate-and-scale path when either differs from
// its default.
void CC_Draw_Shape(PixelView& view, std::span<const std::byte> shapefile,
                   int shape_num, int x, int y, WindowNumberType window,
                   engine::gfx::ShapeFlags flags,
                   std::span<const uint8_t> fading_data = {},
                   std::span<const uint8_t> ghostdata = {},
                   game::DirType rotation = game::DIR_N,
                   int32_t scale = 0x0100);

// Returns the width x height shape `pixels` scaled by `scale` (24.8 fixed
// point, 0x100 = 1.0; zero draws nothing) and turned to `rotation`: a
// (2 * width) x (2 * height) image with the shape's centre at its centre, so a
// quarter turn of a non-square shape still fits. Pixel 0 is transparent, and
// whatever falls outside the doubled frame is cropped.
std::vector<uint8_t> ScaleRotateShape(std::span<const uint8_t> pixels,
                                      int width, int height,
                                      game::DirType rotation, int32_t scale);

// Returns the offset the predator (cloak) effect samples the page at for a
// shape drawn at x in a window window_width pixels wide on game frame `frame`:
// the frame number, negated for a shape past the middle of the window.
int PredatorOffset(int frame, int x, int window_width);

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

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_ASSETS_SHAPE_DRAW_H_
