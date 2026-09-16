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

/* $Header: /CounterStrike/DESCDLG.H 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : DESCDLG.H *
 *                                                                                             *
 *                   Programmer : Maria del Mar McCready Legg
 ** Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : Jan 26, 1995 *
 *                                                                                             *
 *                  Last Update : Jan 26, 1995   [MML] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*/

#ifndef CNC_RED_ALERT_RA_DESCDLG_H_
#define CNC_RED_ALERT_RA_DESCDLG_H_

#include "ra/gadget.h"

class DescriptionClass {
 private:
  static constexpr int kOptionWidth = 216;   // Width of dialog box.
  static constexpr int kOptionHeight = 122;  // Height of dialog box.
  static constexpr int kOptionX = (320 - kOptionWidth) / 2 & ~7;
  static constexpr int kOptionY = (200 - kOptionHeight) / 2;
  static constexpr int kTextX = kOptionX + 32;  // Title's x pos
  static constexpr int kTextY =
      kOptionY + 32;                        // Add 11 for each following line
  static constexpr int kButtonOptions = 1;  // Button number for "Ok"
  static constexpr int kButtonCancel = kButtonOptions + 1;
  static constexpr int kButtonEdit = kButtonCancel + 1;
  static constexpr int kButtonX = kOptionX + 63;   // Options button x pos
  static constexpr int kButtonY = kOptionY + 102;  // Options button y pos
  static constexpr int kEditY = kOptionY + 50;
  static constexpr int kEditW = 180;  // 204,

 public:
  DescriptionClass() {};
  void Process(char* string);
};

#endif  // CNC_RED_ALERT_RA_DESCDLG_H_
