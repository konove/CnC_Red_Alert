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

/* $Header:   F:\projects\c&c\vcs\code\power.h_v   2.16   16 Oct 1995 16:48:06
 * JOE_BOSTIC  $ */
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

#ifndef CNC_RED_ALERT_TD_POWER_H_
#define CNC_RED_ALERT_TD_POWER_H_

#include <cstdint>

#include "sdllib/keyboard.h"
#include "td/defines.h"
#include "td/gadget.h"
#include "td/jshell.h"
#include "td/radar.h"

class PowerClass : public RadarClass {
 public:
  void ResetTransientUiState() override;
  int PowX = 0;
  int PowY = 0;
  int PowWidth = 0;
  int PowHeight = 0;
  int PowLineSpace = 0;
  int PowLineWidth = 0;

  PowerClass();

  /*
  ** Initialization
  */
  void One_Time() override;  // One-time inits

  void Init_Clear() override;  // Clears all to known state
  void Draw_It(bool complete = false) override;
  void AI(KeyNumType& input, int x, int y) override;
  void Refresh_Cells(CELL cell, const int16_t* list) override;
  //		virtual void Must_Redraw_Sidebar();

  /*
  **	File I/O.
  */

  bool IsPowerToRedraw : 1 {false};

 protected:
  /*
  **	This gadget is used to capture mouse input on the power bar.
  */
  class PowerButtonClass : public GadgetClass {
   public:
    PowerButtonClass() noexcept
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
    POWER_STEP_LEVEL = 100,
    POWER_STEP_FACTOR = 6,
  };

 private:
  int Power_Height(int value);

  bool IsActive : 1 = false;

  int RecordedDrain{-1};
  int RecordedPower{-1};
  int DesiredDrainHeight{0};
  int DesiredPowerHeight{0};
  int DrainHeight{0};
  int PowerHeight{0};
  int DrainBounce{0};
  int PowerBounce{0};
  int16_t PowerDir{0};
  int16_t DrainDir{0};

  /*
  **	Points to the shape to use for the "desired" power level indicator.
  */
  static const void* PowerShape;

  /*
  ** Points to the shapes to be used for drawing the power bar
  */
  static const void* PowerBarShape;
};

#endif  // CNC_RED_ALERT_TD_POWER_H_
