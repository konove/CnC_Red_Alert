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

/* $Header: /CounterStrike/DROP.CPP 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : DROP.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 01/24/96 *
 *                                                                                             *
 *                  Last Update : January 24, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "ra/drop.h"

#include "port/ex_string.h"
#include "port/safe_string.h"
#include "ra/dialog.h"
#include "ra/gadget.h"
#include "ra/keyframe.h"
#include "sdllib/font.h"

DropListClass::DropListClass(int id, char* text, int max_len,
                             TextPrintType flags, int x, int y, int w, int h,
                             const void* up, const void* down)
    : EditClass(id, text, max_len, flags, x, y, w, 18, kAlphanumeric),
      IsDropped(false),
      ListHeight(h),
      DropButton(0, down, x + w, y),
      List(0, x, y + Get_Build_Frame_Height(down),
           w + Get_Build_Frame_Width(down), h, flags, up, down) {
  Fancy_Text_Print("", 0, 0, nullptr, 0, flags);
  Height = FontHeight + 1;
  List.Make_Peer(*this);
  DropButton.Make_Peer(*this);
}

void DropListClass::Zap() {
  Collapse();
  List.Zap();
  DropButton.Zap();
  EditClass::Zap();
}

DropListClass& DropListClass::Add(LinkClass& object) {
  DropButton.Add(object);
  return dynamic_cast<DropListClass&>(EditClass::Add(object));
}

DropListClass& DropListClass::Add_Tail(LinkClass& object) {
  DropButton.Add_Tail(object);
  return dynamic_cast<DropListClass&>(EditClass::Add_Tail(object));
}

DropListClass& DropListClass::Add_Head(LinkClass& object) {
  DropButton.Add_Head(object);
  return dynamic_cast<DropListClass&>(EditClass::Add_Head(object));
}

DropListClass* DropListClass::Remove() {
  if (IsDropped) {
    Collapse();
  }
  DropButton.Remove();
  return dynamic_cast<DropListClass*>(EditClass::Remove());
}

int DropListClass::Add_Item(const char* text) {
  port::SafeCopy(String, text, MaxLength);
  Flag_To_Redraw();
  return List.Add_Item(text);
}

const char* DropListClass::Current_Item() { return List.Current_Item(); }

int DropListClass::Current_Index() { return List.Current_Index(); }

void DropListClass::Set_Selected_Index(int index) {
  if (static_cast<unsigned>(index) < static_cast<unsigned>(List.Count())) {
    List.Set_Selected_Index(index);
    port::SafeCopy(String, List.Get_Item(Current_Index()), MaxLength);
  } else {
    String[0] = '\0';
  }
}

void DropListClass::Clear_Focus() { Collapse(); }

void DropListClass::Peer_To_Peer(unsigned flags, KeyNumType& key,
                                 ControlClass& whom) {
  if (&whom == &DropButton) {
    if (flags & LEFTRELEASE) {
      if (IsDropped) {
        Collapse();
        key = ButtonKey(static_cast<int>(ID));
      } else {
        Expand();
      }
    }
  }

  if (&whom == &List) {
    port::SafeCopy(String, List.Current_Item(), MaxLength);
    Flag_To_Redraw();
    key = ButtonKey(static_cast<int>(ID));
  }
}

void DropListClass::Expand() {
  if (!IsDropped) {
    List.X = X;
    List.Y = Y + 18;
    List.Width = Width;
    List.Height = ListHeight;
    List.Add(Head_Of_List());
    List.Flag_To_Redraw();
    IsDropped = true;
  }
}

void DropListClass::Collapse() {
  if (IsDropped) {
    List.Remove();
    IsDropped = false;
  }
}

void DropListClass::Set_Position(int x, int y) {
  EditClass::Set_Position(x, y);
  List.Set_Position(x, y + Get_Build_Frame_Height(DropButton.Get_Shape_Data()));
  DropButton.Set_Position(x + Width, y);
}

void DropListClass::Set_Selected_Index(const char* text) {
  if (text) {
    for (int index = 0; index < Count(); index++) {
      if (stricmp(text, List.Get_Item(index)) == 0) {
        Set_Selected_Index(index);
        break;
      }
    }
  }
}

void DropListClass::Flag_To_Redraw() {
  if (IsDropped) {
    List.Flag_To_Redraw();
  }
  EditClass::Flag_To_Redraw();
}
