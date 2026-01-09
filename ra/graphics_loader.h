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

// Graphics loading utilities for title screens and images.
// Provides functions to load PCX files and display them on screen.

#ifndef GRAPHICS_LOADER_H
#define GRAPHICS_LOADER_H

#include <string_view>

// Forward declarations to avoid pulling in heavy headers
class GraphicViewPortClass;

// Loads a PCX title screen image into a graphics viewport.
// Reads the PCX file, extracts the palette, and blits the image to the
// viewport. The palette parameter is updated with the image's palette data.
void Load_Title_Screen(std::string_view name, GraphicViewPortClass* video_page,
                       unsigned char* palette);

#endif  // GRAPHICS_LOADER_H
