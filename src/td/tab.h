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

/* $Header:   F:\projects\c&c\vcs\code\tab.h_v   2.18   16 Oct 1995 16:45:26
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
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

#ifndef CNC_RED_ALERT_TD_TAB_H_
#define CNC_RED_ALERT_TD_TAB_H_

#include "sdllib/keyboard.h"
#include "td/credits.h"
#include "td/sidebar.h"
#include "tech/noinit.h"

class TabClass : public SidebarClass {
 public:
  TabClass();
  TabClass(const NoInitClass& x) : SidebarClass(x), Credits(x) {}

  void AI(KeyNumType& input, int x, int y) override;
  void Draw_It(bool complete = false) override;

  void One_Time() override;  // One-time inits
  static void Draw_Credits_Tab();
  static void Hilite_Tab(int tab);
  void Redraw_Tab() {
    IsToRedraw = true;
    Flag_To_Redraw(false);
  }

  /*
  **	File I/O.
  */
  void Code_Pointers() override;
  void Decode_Pointers() override;
  int Get_Tab_Height() { return Tab_Height; }

  CreditClass Credits;

 protected:
  /*
  **	If the tab graphic is to be redrawn, then this flag is true.
  */
  unsigned IsToRedraw : 1;
  int Eva_Width;
  int Tab_Height;

 private:
  void Set_Active(int select);

  static const void* TabShape;
};

#endif  // CNC_RED_ALERT_TD_TAB_H_
