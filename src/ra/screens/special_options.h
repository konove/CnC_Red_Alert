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

// The developer options dialog, which edits the special flags
// (ra/core/special_flags.h) during a game.

#ifndef CNC_RED_ALERT_RA_SCREENS_SPECIAL_OPTIONS_H_
#define CNC_RED_ALERT_RA_SCREENS_SPECIAL_OPTIONS_H_

namespace ra {

// Runs the developer options dialog: one check box per changeable flag, with OK
// and Cancel. OK applies the boxes; Cancel and Escape discard them. The game
// keeps running behind the dialog in multiplayer, and the dialog also closes,
// applying nothing, when that game ends.
//
// The new flags are queued as an event, so they take effect on every machine on
// the same frame, and the map is redrawn on the way out.
void RunSpecialOptionsDialog();

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_SCREENS_SPECIAL_OPTIONS_H_
