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

/* $Header: /CounterStrike/MOUSE.H 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : MOUSE.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 12/15/94 *
 *                                                                                             *
 *                  Last Update : December 15, 1994 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_RA_MOUSE_H_
#define CNC_RED_ALERT_RA_MOUSE_H_

#include "ra/defines.h"
#include "ra/jshell.h"
#include "ra/scroll.h"
#include "sdllib/include/keyboard.h"
#include "tech/ftimer.h"
#include "tech/noinit.h"
#include "tech/pipe.h"
#include "tech/straw.h"

class MouseClass : public ScrollClass {
 public:
  MouseClass();
  MouseClass(const NoInitClass& x) : ScrollClass(x) {}

  /*
  ** Initialization
  */
  void One_Time() override;    // One-time inits
  void Init_Clear() override;  // Clears all to known state

  void AI(KeyNumType& input, int x, int y) override;
  bool Override_Mouse_Shape(MouseType mouse, bool wsmall = false) override;
  void Revert_Mouse_Shape() override;
  MouseType Get_Mouse_Shape() const override { return NormalMouseShape; }
  void Mouse_Small(bool wsmall) override;

  /*
  **	File I/O.
  */
  virtual bool Load(Straw& file);
  virtual bool Save(Pipe& file) const;

  void Set_Default_Mouse(MouseType mouse, bool wsmall = false) override;

  /*
  **	This allows the tactical map input gadget access to change the
  **	mouse shapes.
  */
  friend class TacticalClass;

  /*
  ** This points to the loaded mouse shapes.
  */
  static const void* MouseShapes;

 private:
  /*
  **	This type is used to control the frames and rates of the mouse
  **	pointer. Some mouse pointers are actually looping animations.
  */
  typedef struct MouseStruct {
    int StartFrame;  // Starting frame number.
    int FrameCount;  // Number of animation frames.
    int FrameRate;   // Frame delay between changing frames.
    int SmallFrame;  // Start frame number for small version (if any).
    int X, Y;        // Hotspot X and Y offset.
  } MouseStruct;

  /*
  **	The control frames and rates for the various mouse pointers are stored
  **	in this static array.
  */
  static MouseStruct MouseControl[MOUSE_COUNT];

 public:
  /*
  **	If the small representation of the mouse is active, then this flag is
  *true.
  */
  unsigned IsSmall : 1;

 private:
  /*
  **	The mouse shape is controlled by these variables. These
  **	hold the current mouse shape (so resetting won't be needlessly
  *performed) and *	the normal default mouse shape (when arrow shapes are
  *needed).
  */
  MouseType CurrentMouseShape;
  MouseType NormalMouseShape;

  /*
  **	For animating mouse shapes, this controls the frame and animation rate.
  */
  static CDTimerClass<SystemTimerClass> Timer;
  int Frame;
};

#endif  // CNC_RED_ALERT_RA_MOUSE_H_
