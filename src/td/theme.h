/*
**	Command & Conquer(tm)
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

/* $Header:   F:\projects\c&c\vcs\code\theme.h_v   2.16   16 Oct 1995 16:45:48
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : THEME.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : August 14, 1994 *
 *                                                                                             *
 *                  Last Update : August 14, 1994   [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef THEME_H
#define THEME_H

#include "td/defines.h"

class ThemeClass {
 private:
  static const char* Theme_File_Name(ThemeType theme);

  int Current;        // Handle to current score.
  ThemeType Score;    // Score number currently being played.
  ThemeType Pending;  // Score to play next.

  typedef struct {
    const char* Name;  // Filename of score.
    int Fullname;      // Text number for full score name.
    int Scenario;      // Scenario when it first becomes available.
    int Duration;      // Duration of theme in seconds.
    bool Normal;       // Allowed in normal game play?
    bool Variation;    // Is there a variation to the score?
    bool Repeat;       // Always repeat this score?
    bool Available;    // Is the score available?
  } ThemeControl;

  static ThemeControl _themes[THEME_COUNT];

  enum { THEME_DELAY = TIMER_SECOND };

 public:
  ThemeClass();

  ThemeType From_Name(const char* name);
  int Track_Length(ThemeType index);
  int Max_Themes() { return THEME_COUNT; }
  const char* Full_Name(ThemeType index) const;
  const char* Base_Name(ThemeType index) const;
  void AI();
  void Queue_Song(ThemeType index);
  int Play_Song(ThemeType index);
  ThemeType What_Is_Playing() { return Score; }
  void Stop();
  void Fade_Out() { Queue_Song(THEME_NONE); }
  int Still_Playing();
  ThemeType Next_Song(ThemeType index);
  bool Is_Allowed(ThemeType index) const;
  static void Scan();
};

#endif
