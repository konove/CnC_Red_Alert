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

/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
 *             ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer Red Alert               *
 *                                                                                             *
 *                    File Name : INTERPAL.H               *
 *                                                                                             *
 *                   Programmer : Steve Tall               *
 *                                                                                             *
 *                   Start Date : December 7th 1995               *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Overview:                * Header file for palette interpolation
 * functionality used for scaling 320x200 animations  * to 640x400 screen
 * resolution.                                                            *
 *                                                                                             *
 * Functions:                * Read_Interpolation_Palette -- reads an
 * interpolation palette table from disk             *
 *   Write_Interpolation_Palette -- writes an interpolation palette to disk
 *              * Increase_Palette_Luminance -- increase the contrast of a
 * palette                         * Interpolate_2X_Scale -- Stretch a 320x200
 * graphic buffer into 640x400                    *
 *                                                                                             *
 *=============================================================================================*/

#ifndef INTERPAL_H
#define INTERPAL_H

// Forward declarations
class GraphicBufferClass;
class GraphicViewPortClass;

// Constants
#define SIZE_OF_PALETTE 256

// Palette interpolation functions
void Read_Interpolation_Palette(char const* palette_file_name);
void Write_Interpolation_Palette(char const* palette_file_name);
void Increase_Palette_Luminance(unsigned char* InterpolationPalette,
                                int RedPercentage, int GreenPercentage,
                                int BluePercentage, unsigned cap);
void Interpolate_2X_Scale(GraphicBufferClass* source,
                          GraphicViewPortClass* dest,
                          char const* palette_file_name);

// C linkage for assembly functions and global data
extern "C" {
extern unsigned char PaletteInterpolationTable[SIZE_OF_PALETTE]
                                              [SIZE_OF_PALETTE];
extern unsigned char* InterpolationPalette;
void __cdecl Asm_Create_Palette_Interpolation_Table();
}

// Global state
extern bool InterpolationPaletteChanged;

#endif  // INTERPAL_H
