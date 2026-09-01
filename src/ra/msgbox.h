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

/* $Header: /CounterStrike/MSGBOX.H 1     3/03/97 10:25a Joe_bostic $ */
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

#ifndef CNC_RED_ALERT_RA_MSGBOX_H_
#define CNC_RED_ALERT_RA_MSGBOX_H_

#include "ra/conquer.h"

// Set from outside the message box loop to make the box close as though the
// user had picked its cancel button. WOL uses it to take a dialog down when
// the server answers before the player does. The loop clears it on the way
// out.
extern bool cancel_current_msgbox;

class WWMessageBox {
  int Caption;

 public:
  WWMessageBox(int caption = TXT_NONE) { Caption = caption; }
  int Process(const char* msg, const char* b1txt, const char* b2txt = nullptr,
              const char* b3txt = nullptr, bool preserve = false);
  int Process(int msg, int b1txt = TXT_OK, int b2txt = TXT_NONE,
              int b3txt = TXT_NONE, bool preserve = false);
  int Process(const char* msg, int b1txt = TXT_OK, int b2txt = TXT_NONE,
              int b3txt = TXT_NONE, bool preserve = false);
};

#endif  // CNC_RED_ALERT_RA_MSGBOX_H_
