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

/* $Header: /CounterStrike/SAVEDLG.H 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : SAVEDLG.H *
 *                                                                                             *
 *                   Programmer : Maria del Mar McCready Legg, Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : Jan 8, 1995 *
 *                                                                                             *
 *                  Last Update : Jan 18, 1995   [MML] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_RA_SAVEDLG_H_
#define CNC_RED_ALERT_RA_SAVEDLG_H_

class SaveOptionsClass {
 private:
  static constexpr int kButtonCancel = 200;
  static constexpr int kButtonSave = 201;
  static constexpr int kOptionWidth = 216;
  static constexpr int kOptionHeight = 122;
  static constexpr int kOptionX = (320 - kOptionWidth) / 2 & ~7;
  static constexpr int kOptionY = (200 - kOptionHeight) / 2;
  static constexpr int kNumberOfButtons = 2;
  static constexpr int kCaptionYPos = 5;
  static constexpr int kBorder1Len = 49;
  static constexpr int kButtonCancelX = 90;
  static constexpr int kButtonCancelY = 103;
  static constexpr int kListboxX = 40;
  static constexpr int kListboxY = 24;
  static constexpr int kListboxW = 136;
  static constexpr int kListboxH = 72;

 public:
  SaveOptionsClass() {}
  void Process();
};

#endif  // CNC_RED_ALERT_RA_SAVEDLG_H_
