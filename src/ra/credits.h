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

/* $Header: /CounterStrike/CREDITS.H 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : CREDIT.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : April 19, 1994 *
 *                                                                                             *
 *                  Last Update : April 19, 1994   [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_RA_CREDITS_H_
#define CNC_RED_ALERT_RA_CREDITS_H_

#include <cstdint>

/****************************************************************************
**	The animating credit counter display is controlled by this class.
*/
class CreditClass {
 public:
  int64_t Credits{0};  // Value of credits trying to update display to.

  /*---------------------------------------------------------------------
  **	Constructors, Destructors, and overloaded operators.
  */
  CreditClass();

  /*---------------------------------------------------------------------
  **	Member function prototypes.
  */
  void Update(bool forced = false, bool redraw = false);

  void Graphic_Logic(bool forced = false);
  void AI(bool forced = false);

  int64_t Current{0};  // Credit value currently displayed.

  unsigned IsToRedraw : 1 {false};
  unsigned IsUp : 1 {false};
  unsigned IsAudible : 1 {false};

 private:
  int Countdown{0};  // Delay between ticks.
};

#endif  // CNC_RED_ALERT_RA_CREDITS_H_
