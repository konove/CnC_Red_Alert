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

/* $Header: /CounterStrike/CHEKLIST.CPP 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : CHEKLIST.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 07/05/96 *
 *                                                                                             *
 *                  Last Update : July 6, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * CheckListClass::Action -- action function for this class *
 *   CheckListClass::Add_Item -- Adds specifies text to check list box. *
 *   CheckListClass::CheckListClass -- constructor * CheckListClass::Check_Item
 *-- [un]checks an items                                         *
 *   CheckListClass::Draw_Entry -- draws a list box entry *
 *   CheckListClass::Get_Item -- Fetches a pointer to the text associated with
 *the index.      * CheckListClass::Remove_Item -- Remove the item that matches
 *the text pointer specified.   * CheckListClass::Set_Selected_Index -- Set the
 *selected index to match the text pointer spe* CheckListClass::~CheckListClass
 *-- Destructor for check list object.                      *
 *   CheckListClass::~CheckListClass -- destructor *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "ra/cheklist.h"

#include <cstdio>
#include <vector>

#include "absl/strings/str_format.h"
#include "base/numeric.h"
#include "ra/defines.h"
#include "ra/dialog.h"
#include "ra/gadget.h"
#include "ra/jshell.h"
#include "ra/list.h"
#include "sdllib/drawbuff.h"
#include "sdllib/gbuffer.h"
#include "sdllib/keyboard.h"
#include "sdllib/wwstd.h"

CheckListClass::CheckListClass(int id, int x, int y, int w, int h,
                               TextPrintType flags, const void* up,
                               const void* down)
    : ListClass(id, x, y, w, h, flags, up, down) {}

int CheckListClass::Add_Item(const char* text) {
  const int index = ListClass::Add_Item(text);
  // ListClass adds nothing for a null text; size to what it actually holds.
  Checked.resize(List.size(), false);
  return index;
}

void CheckListClass::Remove_Item(int index) {
  if (index >= 0 && index < Count()) {
    Checked.erase(Checked.begin() + index);
    ListClass::Remove_Item(index);
  }
}

void CheckListClass::Check_Item(int index, bool checked) {
  if (index >= 0 && index < Count() &&
      Checked[base::ToSize(index)] != checked) {
    Checked[base::ToSize(index)] = checked;
    Flag_To_Redraw();
  }
}

bool CheckListClass::Is_Checked(int index) const {
  return index >= 0 && index < Count() && Checked[base::ToSize(index)];
}

bool CheckListClass::Action(unsigned flags, KeyNumType& key) {
  /*
  ** If this is a read-only list, it's a display-only device
  */
  if (IsReadOnly) {
    return false;
  }

  /*
  **	Invoke parents Action first, so it can set the SelectedIndex if needed.
  */
  const bool rc = ListClass::Action(flags, key);

  /*
  **	Now, if this event was a left-press, toggle the checked state of the
  **	current item.
  */
  if (flags & LEFTPRESS) {
    Check_Item(SelectedIndex, !Is_Checked(SelectedIndex));
  }

  return rc;
}

void CheckListClass::Draw_Entry(int index, int x, int y, int width,
                                bool selected) {
  if (index >= Count()) {
    return;
  }

  char buffer[100] = "";
  buffer[0] = Is_Checked(index) ? CHECK_CHAR : UNCHECK_CHAR;
  buffer[1] = ' ';
  absl::SNPrintF(&buffer[2], sizeof(buffer) - 2, "%s", Get_Item(index));

  TextPrintType flags = TextFlags;
  RemapControlType* scheme = Get_Color_Scheme();

  if (selected) {
    flags = flags | TPF_BRIGHT_COLOR;
    LogicPage->Fill_Rect(x, y, x + width - 1, y + LineHeight - 1,
                         scheme->Shadow);
  } else {
    if (!(flags & TPF_USE_GRAD_PAL)) {
      flags = flags | TPF_MEDIUM_COLOR;
    }
  }

  Conquer_Clip_Text_Print(buffer, x, y, scheme, TBLACK, flags, width, Tabs);
}
