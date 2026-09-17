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
#ifndef CNC_RED_ALERT_RA_HOTKEYS_H_
#define CNC_RED_ALERT_RA_HOTKEYS_H_

// File: Keyboard commands for the tactical map.

#include "ra/defines.h"
#include "sdllib/keyboard.h"

// Handles keyboard input while the tactical map is displayed. Consumes each
// key it acts on by setting input to KN_NONE.
void Keyboard_Process(KeyNumType& input);

// Converts a keyboard code into the compass direction it represents, or
// FACING_NONE for a key that is not directional. Used for keyboard scrolling.
FacingType KN_To_Facing(unsigned input);

#endif  // CNC_RED_ALERT_RA_HOTKEYS_H_
