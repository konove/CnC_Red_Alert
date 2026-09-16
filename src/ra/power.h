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

#include <cstdint>

#include "ra/defines.h"
#include "ra/display.h"
#include "ra/gadget.h"
#include "ra/jshell.h"
#include "ra/radar.h"
#include "sdllib/keyboard.h"
#include "tech/ftimer.h"

class PowerClass : public RadarClass {
 public:
  // Resets transient UI state after loading, preserving saved game state.
  void ResetTransientUiState() override;

  PowerClass();

  /*
  ** Initialization
  */
  void One_Time() override;  // One-time inits

  void Init_Clear() override;  // Clears all to known state
  void Draw_It(bool complete = false) override;
  void AI(KeyNumType& input, int x, int y) override;
  void Refresh_Cells(CELL cell, const int16_t* list) override;
  void Flash_Power();

  bool IsPowerToRedraw : 1 {false};

 protected:
  /*
  **	This gadget is used to capture mouse input on the power bar.
  */
  class PowerButtonClass : public GadgetClass {
   public:
    PowerButtonClass() noexcept
        : GadgetClass(
              0, 0, 0, 0,
              kLeftPress | kLeftRelease | kLeftHeld | kLeftUp | kRightPress,
              true) {}

   protected:
    bool Action(unsigned flags, KeyNumType& key) override;
    friend class PowerClass;
  };

  /*
  **	This is the "button" that tracks all input to the tactical map.
  ** It must be available to derived classes, for Save/Load purposes.
  */
  static PowerButtonClass PowerButton;

  static constexpr int kPowerX = 10 * ICON_PIXEL_W;
  static constexpr int kPowerY = 7 + 70 + 13;
  static constexpr int kPowerHeight = 200 - (7 + 70 + 13);
  static constexpr int kPowerWidth = 8;
  static constexpr int kPowerLineSpace = 5;
  static constexpr int kPowerLineWidth = 3;
  static constexpr int kPowerStepLevel = 100;
  static constexpr int kPowerStepFactor = 5;

 private:
  static int Power_Height(int value);

  /*
  **	If the power bar should be rendered with some flash effect then
  **	this specifies the duration that the flash will occur.
  */
  Timer<FrameTickSource> FlashTimer;

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
  static const void* PowerBarShape;
};

#endif  // CNC_RED_ALERT_RA_POWER_H_
