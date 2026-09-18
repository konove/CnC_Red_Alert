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
#ifndef CNC_RED_ALERT_RA_CONQUER_H_
#define CNC_RED_ALERT_RA_CONQUER_H_

// File: The game's outer loop: RunGame() picks and plays games until the
// player exits, RunFrame() runs one frame, and ServiceRealTime() does the
// real-time servicing that has to keep running inside blocking loops and
// dialogs.

// The game's entry point: one-time init, then a loop of choosing a game and
// running it until the player exits.
void RunGame();

// Runs one frame of the game. Returns true when the game should end.
bool RunFrame();

// Real-time maintenance -- sound, music, and network servicing -- followed by
// presenting the screen. Unlike the per-frame game logic this has to run as
// often as possible, so it is called from inside blocking loops and dialogs as
// well as from the main loop. Presenting waits for the display refresh (see
// PresentFrame), which is also what keeps those loops from spinning.
void ServiceRealTime();

// The maintenance half of ServiceRealTime(), without presenting. For work
// that runs straight through, such as saving and loading, where nothing on
// screen changes and each present would only wait out a display refresh.
void ServiceBackgroundTasks();

#endif  // CNC_RED_ALERT_RA_CONQUER_H_
