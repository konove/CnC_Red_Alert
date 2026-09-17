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

// File: The game's outer loop: Main_Game() picks and plays games until the
// player exits, Main_Loop() runs one frame, and Call_Back() does the real-time
// servicing that has to keep running inside blocking loops and dialogs.

// The game's entry point: one-time init, then a loop of choosing a game and
// running it until the player exits.
void Main_Game(int argc, char* argv[]);

// Runs one frame of the game. Returns true when the game should end.
bool Main_Loop();

// Real-time maintenance -- sound, music, and network servicing. Unlike the
// per-frame game logic this has to run as often as possible, so it is called
// from inside blocking loops and dialogs as well as from the main loop.
void Call_Back();

#endif  // CNC_RED_ALERT_RA_CONQUER_H_
