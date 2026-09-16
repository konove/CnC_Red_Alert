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

/* $Header: /CounterStrike/DOOR.H 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : DOOR.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 06/11/95 *
 *                                                                                             *
 *                  Last Update : June 11, 1995 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_RA_DOOR_H_
#define CNC_RED_ALERT_RA_DOOR_H_
#include "ra/stage.h"

class DoorClass {
 private:
  /*
  **	This is the animation control handler.
  */
  StageClass Control;

  /*
  **	This is the recorded number of stages of the current
  **	door animation process.
  */
  unsigned char Stages{0};

  /*
  **	This is the door state.
  */
  static constexpr int kIsClosed = 0;   // Door is closed.
  static constexpr int kIsOpening = 1;  // Door is in the process of opening.
  static constexpr int kIsOpen = 2;     // Door is fully open.
  static constexpr int kIsClosing = 3;  // Door is in the process of closing.
  int State = kIsClosed;

  /*
  **	If the animation for this door indicates that the object it is
  **	attached to should be redrawn, then this flag will be true. Drawing
  **	clears it from const draw routines, so it is redraw bookkeeping rather
  **	than logical state.
  */
  mutable bool IsToRedraw : 1 {false};

 public:
  DoorClass();

  // Saved-game support.
  template <class Archive>
  void Serialize(Archive& ar) {
    bool is_to_redraw = IsToRedraw;
    ar(Control, Stages, State, is_to_redraw);
    IsToRedraw = is_to_redraw;
  }

  [[nodiscard]] bool Time_To_Redraw() const { return IsToRedraw; }
  void Clear_Redraw_Flag() const { IsToRedraw = false; }
  void AI();
  [[nodiscard]] int Door_Stage() const;
  [[nodiscard]] bool Is_Door_Opening() const { return State == kIsOpening; }
  [[nodiscard]] bool Is_Door_Closing() const { return State == kIsClosing; }
  bool Open_Door(int rate, int stages);
  bool Close_Door(int rate, int stages);
  [[nodiscard]] bool Is_Door_Open() const { return State == kIsOpen; }
  [[nodiscard]] bool Is_Door_Closed() const { return State == kIsClosed; }
  [[nodiscard]] bool Is_Ready_To_Open() const;
};

#endif  // CNC_RED_ALERT_RA_DOOR_H_
