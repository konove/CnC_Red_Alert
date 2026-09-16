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

/* $Header:   F:\projects\c&c\vcs\code\goptions.h_v   2.19   16 Oct 1995
 * 16:46:26   JOE_BOSTIC  $ */
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

#ifndef CNC_RED_ALERT_TD_GOPTIONS_H_
#define CNC_RED_ALERT_TD_GOPTIONS_H_

#include "td/options.h"

class GameOptionsClass : public OptionsClass {
  static constexpr int kButtonLoad = 1;
  static constexpr int kButtonSave = 2;
  static constexpr int kButtonDelete = 3;
  static constexpr int kButtonGame = 4;
  static constexpr int kButtonQuit = 5;
  static constexpr int kButtonResume = 6;
  static constexpr int kButtonRestate = 7;

  static constexpr int kButtonCount = 8;

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

void Draw_Caption(int text, int x, int y, int w);

#endif  // CNC_RED_ALERT_TD_GOPTIONS_H_
