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

// The first-launch introduction (INTRO.H, Barry W. Green, May 1995).

#ifndef CNC_RED_ALERT_RA_INTRO_H_
#define CNC_RED_ALERT_RA_INTRO_H_

// Plays the introduction movie. Select_Game() calls this when it starts the
// campaign on the first launch after installing (Special.IsFromInstall).
//
// When playing from the DVD, first asks the player to choose Allies or Soviets
// and stores the answer in CurrentCD (0 Allied, 1 Soviet), which the caller
// turns into the first scenario. With CDs the disc in the drive has already
// answered that question. Leaves the mouse as it found it. Originally
// "Choose_Side": in Tiberian Dawn this is where the player picks a house.
void PlayFirstLaunchIntro();

#endif  // CNC_RED_ALERT_RA_INTRO_H_
