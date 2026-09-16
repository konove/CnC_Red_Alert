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

/* $Header: /CounterStrike/GOPTIONS.H 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : OPTIONS.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : June 8, 1994 *
 *                                                                                             *
 *                  Last Update : June 8, 1994   [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_RA_GOPTIONS_H_
#define CNC_RED_ALERT_RA_GOPTIONS_H_

#include "ra/config.h"
#include "ra/options.h"

class GameOptionsClass : public OptionsClass {
  static constexpr int kButtonLoad = 1;
  static constexpr int kButtonSave = kButtonLoad + 1;
  static constexpr int kButtonDelete = kButtonSave + 1;
  static constexpr int kButtonGame = kButtonDelete + 1;
  static constexpr int kButtonQuit = kButtonGame + 1;
  static constexpr int kButtonDraw = kButtonQuit + 1;
  static constexpr int kButtonResume = kButtonDraw + 1;
  static constexpr int kButtonRestate = kButtonResume + 1;
  static constexpr int kButtonCount = kButtonRestate + 1;

  static constexpr int kOptionWidth = 216 + 8;
  static constexpr int kOptionHeight = 100;
  static constexpr int kOptionX = (320 - (216 + 8)) / 2;
  static constexpr int kOptionY = (200 - 100) / 2;
  static constexpr int kButtonWidth = config::kIsFrench ? 142 : 130;
  static constexpr int kNumberOfButtons = 6;  //	ajw Not used.
  static constexpr int kCaptionYPos = 5;
  static constexpr int kButtonY = 21;
  static constexpr int kBorder1Len = 72;
  static constexpr int kBorder2Len = 16;
  static constexpr int kButtonResumeY = 100 - 15;

 public:
  GameOptionsClass() = default;
  void Adjust_Variables_For_Resolution();
  void Process();

 private:
  int OptionWidth = 0;
  int OptionHeight = 0;
  int OptionX = 0;
  int OptionY = 0;
  int ButtonWidth = 0;
  int OButtonHeight = 0;
  int CaptionYPos = 0;
  int ButtonY = 0;
  int Border1Len = 0;
  int Border2Len = 0;
  int ButtonResumeY = 0;
};

extern bool RedrawOptionsMenu;

#endif  // CNC_RED_ALERT_RA_GOPTIONS_H_
