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
// August 1994) that the games still call: setting the video mode, waiting a
// number of ticks, ending a frame, and the two exit and screen-shake hooks the
// games define themselves.

#ifndef CNC_RED_ALERT_ENGINE_WINDOW_MISC_H_
#define CNC_RED_ALERT_ENGINE_WINDOW_MISC_H_

#include "engine/window/display.h"

// Gives the window a w x h paletted surface through
// TheDisplay().SetVideoMode(); the surface is always 8-bit, whatever
// `bits_per_pixel` says. Returns false if SDL could not create it.
bool Set_Video_Mode(int w, int h, int bits_per_pixel);

// Cleans up the library systems (audio, mouse, tick timer, ...) before the
// process exits. Each game defines it in its startup.cc.
void Prog_End();

// Waits `duration` ticks of the 60 Hz tick timer, presenting a frame on each
// pass so that the window stays responsive meanwhile. Needs InitTickTimer().
void Delay(int duration);

// Jolts the visible page up and down `shakes` times, for explosions. Only
// Tiberian Dawn defines it (td/sdlstub.cc); Red Alert has its own
// Shake_The_Screen() and must not call it.
void Shake_Screen(int shakes);

// Ends the frame, presenting it. The name is DOS's: there the games waited for
// the vertical blank before touching the palette or the visible page, and
// presenting is what now stands in for that wait.
inline void Wait_Vert_Blank() { TheDisplay().EndFrame(); }

#endif  // CNC_RED_ALERT_ENGINE_WINDOW_MISC_H_
