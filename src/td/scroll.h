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

/* $Header:   F:\projects\c&c\vcs\code\scroll.h_v   2.16   16 Oct 1995 16:46:12
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : SCROLL.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 11/18/94 *
 *                                                                                             *
 *                  Last Update : November 18, 1994 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_TD_SCROLL_H_
#define CNC_RED_ALERT_TD_SCROLL_H_

#include "sdllib/keyboard.h"
#include "sdllib/timer.h"
#include "td/help.h"
#include "tech/noinit.h"

class ScrollClass : public HelpClass {
  /*
  **	If map scrolling is automatic, then this flag is true. Automatic
  *scrolling will *	cause the map to scroll if the mouse is in the scroll
  *region, regardless of *	whether or not the mouse button is held down.
  */
  unsigned IsAutoScroll : 1;

  /*
  **	Scroll speed is regulated by this count down timer. When this value
  *reaches zero, *	scroll the map in the direction required and reset this
  *timer.
  */
  static CountDownTimerClass Counter;

  /*
  **	These are the delays used (as countdown timers) to regulate the scroll
  *rate *	of the map. These times are based on game ticks.
  */
  //		enum ScrollClassEnums {
  //			INITIAL_DELAY=8,				// Delay
  // before scrolling can start at all. 			SEQUENCE_DELAY=4
  //// Delay between scroll steps.
  //		};

  int Inertia;

 public:
  ScrollClass();
  ScrollClass(const NoInitClass& x) : HelpClass(x) {}

  bool Set_Autoscroll(int control);

  void AI(KeyNumType& input, int x, int y) override;
  void Init_IO() override {
    Counter.Set(0);
    HelpClass::Init_IO();
  }

  /*
  **	File I/O.
  */
  void Code_Pointers() override;
  void Decode_Pointers() override;
};

#endif  // CNC_RED_ALERT_TD_SCROLL_H_
