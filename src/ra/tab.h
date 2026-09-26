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

/* $Header: /CounterStrike/TAB.H 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : TAB.H *
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

#ifndef CNC_RED_ALERT_RA_TAB_H_
#define CNC_RED_ALERT_RA_TAB_H_

#include <cstddef>
#include <span>

#include "engine/gfx/pixel_buffer.h"
#include "engine/platform/ftimer.h"
#include "engine/window/keyboard.h"
#include "ra/credits.h"
#include "ra/jshell.h"
#include "ra/sidebar.h"

class TabClass : public SidebarClass {
 public:
  // Resets transient UI state after loading, preserving saved game state.
  void ResetTransientUiState() override;

  TabClass();

  void AI(engine::window::InputEvent& event, int x, int y) override;
  void Draw_It(PixelView& view, bool complete = false) override;
  static void Draw_Credits_Tab(PixelView& view);
  static void Hilite_Tab(PixelView& view, int tab);
  void Flash_Money();

  void One_Time() override;  // One-time inits
  void Redraw_Tab() {
    IsTabToRedraw = true;
    Flag_To_Redraw(false);
  }

  CreditClass Credits;

  Timer<FrameTickSource> FlasherTimer;

 protected:
  /*
  **	If the tab graphic is to be redrawn, then this flag is true.
  */
  bool IsTabToRedraw : 1 {false};

 private:
  static void Set_Active(int select);

  Timer<FrameTickSource> MoneyFlashTimer;

  static std::span<const std::byte> TabShape;
};

#endif  // CNC_RED_ALERT_RA_TAB_H_
