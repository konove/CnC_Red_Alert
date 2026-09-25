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

// File: Leftovers of the Westwood 32-bit library's MISC.H (Scott K. Bowen,
// August 1994) that the games still call: waiting a number of ticks, and the
// two exit and screen-shake hooks the games define themselves.

#ifndef CNC_RED_ALERT_ENGINE_WINDOW_MISC_H_
#define CNC_RED_ALERT_ENGINE_WINDOW_MISC_H_

// Cleans up the library systems (audio, mouse, tick timer, ...) before the
// process exits. Each game defines it in its startup.cc.
void ShutDownEngine();

// Waits `ticks` ticks of the 60 Hz tick timer, presenting a frame on each
// pass so that the window stays responsive meanwhile. Needs InitTickTimer().
void WaitTicks(int ticks);

// Jolts the visible page up and down `shakes` times, for explosions. Only
// Tiberian Dawn defines it (td/sdlstub.cc); Red Alert has its own
// Shake_The_Screen() and must not call it.
void ShakeScreen(int shakes);

#endif  // CNC_RED_ALERT_ENGINE_WINDOW_MISC_H_
