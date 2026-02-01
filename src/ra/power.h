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

/* $Header: /CounterStrike/POWER.H 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : POWER.H *
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

#ifndef CNC_RED_ALERT_RA_POWER_H_
#define CNC_RED_ALERT_RA_POWER_H_

#include "ra/defines.h"
#include "ra/display.h"
#include "ra/gadget.h"
#include "ra/jshell.h"
#include "ra/radar.h"
#include "sdllib/keyboard.h"
#include "tech/ftimer.h"
#include "tech/noinit.h"

class PowerClass : public RadarClass {
 public:
  PowerClass();
  PowerClass(const NoInitClass& x) : RadarClass(x), FlashTimer(x) {}

  /*
  ** Initialization
  */
  void One_Time() override;  // One-time inits

  void Init_Clear() override;  // Clears all to known state
  void Draw_It(bool complete = false) override;
  void AI(KeyNumType& input, int x, int y) override;
  void Refresh_Cells(CELL cell, const short* list) override;
  void Flash_Power();

  unsigned IsToRedraw : 1;

 protected:
  /*
  **	This gadget is used to capture mouse input on the power bar.
  */
  class PowerButtonClass : public GadgetClass {
   public:
    PowerButtonClass()
        : GadgetClass(0, 0, 0, 0,
                      LEFTPRESS | LEFTRELEASE | LEFTHELD | LEFTUP | RIGHTPRESS,
                      true) {}

   protected:
    int Action(unsigned flags, KeyNumType& key) override;
    friend class PowerClass;
  };

  /*
  **	This is the "button" that tracks all input to the tactical map.
  ** It must be available to derived classes, for Save/Load purposes.
  */
  static PowerButtonClass PowerButton;

  enum PowerEnums {
    POWER_X = 10 * ICON_PIXEL_W,
#if RESFACTOR == 2
    POWER_Y = 7 + 70 + 13,
    POWER_HEIGHT = 200 - (7 + 70 + 13),
#else
    POWER_Y = (88 + 9),
    POWER_HEIGHT = 80,
#endif
    POWER_WIDTH = 8,
    POWER_LINE_SPACE = 5,
    POWER_LINE_WIDTH = 3,
    POWER_STEP_LEVEL = 100,
    POWER_STEP_FACTOR = 5
  };

 private:
  int Power_Height(int value);

  /*
  **	If the power bar should be rendered with some flash effect then
  **	this specifies the duration that the flash will occur.
  */
  CDTimerClass<FrameTimerClass> FlashTimer;

  int RecordedDrain;
  int RecordedPower;
  int DesiredDrainHeight;
  int DesiredPowerHeight;
  int DrainHeight;
  int PowerHeight;
  int DrainBounce;
  int PowerBounce;
  short PowerDir;
  short DrainDir;

  /*
  **	Points to the shape to use for the "desired" power level indicator.
  */
  static const void* PowerShape;
  static const void* PowerBarShape;
};

#endif  // CNC_RED_ALERT_RA_POWER_H_
