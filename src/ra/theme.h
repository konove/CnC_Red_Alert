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

/* $Header: /CounterStrike/THEME.H 1     3/03/97 10:26a Joe_bostic $ */
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

#ifndef CNC_RED_ALERT_RA_THEME_H_
#define CNC_RED_ALERT_RA_THEME_H_

#include "ra/defines.h"

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
    bool Repeat;       // Always repeat this score?
    bool Available;    // Is the score available?
    int Owner;  // What houses are allowed to play this theme (bit field)?
  } ThemeControl;

  static ThemeControl _themes[magic_enum::enum_count<ThemeType>()];

  enum { THEME_DELAY = kTimerSecond };

 public:
  ThemeClass();

  ThemeType From_Name(const char* name) const;
  ThemeType Next_Song(ThemeType index) const;
  ThemeType What_Is_Playing() const { return Score; }
  bool Is_Allowed(ThemeType index) const;
  bool Is_Regular(ThemeType theme) const {
    return theme != THEME_NONE && _themes[theme].Normal;
  }
  const char* Base_Name(ThemeType index) const;
  const char* Full_Name(ThemeType index) const;
  int Play_Song(ThemeType index);
  int Still_Playing() const;
  int Track_Length(ThemeType index) const;
  static void Scan();
  void AI();
  void Fade_Out() { Queue_Song(THEME_QUIET); }
  void Queue_Song(ThemeType index);
  void Set_Theme_Data(ThemeType theme, int scenario, int owners);
  void Stop();
  void Suspend();
};

#endif  // CNC_RED_ALERT_RA_THEME_H_
