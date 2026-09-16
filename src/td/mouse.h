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

/* $Header:   F:\projects\c&c\vcs\code\mouse.h_v   2.16   16 Oct 1995 16:45:06
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
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

#ifndef CNC_RED_ALERT_TD_MOUSE_H_
#define CNC_RED_ALERT_TD_MOUSE_H_

class ArchiveReader;
class ArchiveWriter;

#include <cstddef>
#include <span>

#include "base/enum_array.h"
#include "sdllib/keyboard.h"
#include "sdllib/timer.h"
#include "td/defines.h"
#include "td/scroll.h"
#include "tech/file.h"

class MouseClass : public ScrollClass {
 public:
  void ResetTransientUiState() override;
  // Saved gameplay state; runtime UI resources remain local.
  template <class Archive>
  void Serialize(Archive& ar);

  MouseClass();

  /*
  ** Initialization
  */
  void One_Time() override;    // One-time inits
  void Init_Clear() override;  // Clears all to known state

  void AI(KeyNumType& input, int x, int y) override;
  bool Override_Mouse_Shape(MouseType mouse, bool wwsmall = false) override;
  void Revert_Mouse_Shape() override;
  [[nodiscard]] MouseType Get_Mouse_Shape() const override {
    return NormalMouseShape;
  }
  void Mouse_Small(bool wwsmall) override;

  /*
  **	File I/O.
  */
  virtual bool Load(ArchiveReader& file);
  virtual bool Save(ArchiveWriter& file);

  void Set_Default_Mouse(MouseType mouse, bool size = false) override;

  /*
  **	This allows the tactical map input gadget access to change the
  **	mouse shapes.
  */
  friend class TacticalClass;

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
  static base::EnumArray<MouseType, MouseStruct, kMouseCount> MouseControl;

  /*
  **	If the small representation of the mouse is active, then this flag is
  *true.
  */
  bool IsSmall : 1 = false;

  /*
  **	This points to the loaded mouse shapes.
  */
  static std::span<const std::byte> MouseShapes;

  /*
  **	The mouse shape is controlled by these variables. These
  **	hold the current mouse shape (so resetting won't be needlessly
  *performed) and *	the normal default mouse shape (when arrow shapes are
  *needed).
  */
  MouseType CurrentMouseShape{MOUSE_NORMAL};
  MouseType NormalMouseShape{MOUSE_NORMAL};

  /*
  **	For animating mouse shapes, this controls the frame and animation rate.
  */
  static CountDownTimerClass Timer;
  int Frame = 0;
  //		StageClass Control;

};

extern template void MouseClass::Serialize<ArchiveWriter>(ArchiveWriter&);
extern template void MouseClass::Serialize<ArchiveReader>(ArchiveReader&);

#endif  // CNC_RED_ALERT_TD_MOUSE_H_
