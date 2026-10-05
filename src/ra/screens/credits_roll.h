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

// The scrolling credits shown from the main menu and after the last mission.
#ifndef CNC_RED_ALERT_RA_SCREENS_CREDITS_ROLL_H_
#define CNC_RED_ALERT_RA_SCREENS_CREDITS_ROLL_H_

namespace ra {

// Rolls the credits from CREDITS.TXT over the slideshow, with the credits
// score playing, until the last line has scrolled off or the player presses
// Esc. Returns at once if the file is missing.
//
// Leaves the screen black, the title palette overwritten and the music
// stopped, so the caller redraws and restarts whatever it had.
void ShowCredits();

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_SCREENS_CREDITS_ROLL_H_
