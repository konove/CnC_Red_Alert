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

/* $Header: /CounterStrike/DROP.H 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : DROP.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 07/05/96 *
 *                                                                                             *
 *                  Last Update : July 5, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_RA_DROP_H_
#define CNC_RED_ALERT_RA_DROP_H_

#include <cstring>

#include "ra/control.h"
#include "ra/defines.h"
#include "ra/edit.h"
#include "ra/keyframe.h"
#include "ra/link.h"
#include "ra/list.h"
#include "ra/shapebtn.h"
#include "sdllib/keyboard.h"

class DropListClass : public EditClass {
 public:
  DropListClass(int id, char* text, int max_len, TextPrintType flags, int x,
                int y, int w, int h, const void* up, const void* down);
  ~DropListClass() override {}

  DropListClass& Add(LinkClass& object) override;
  DropListClass& Add_Tail(LinkClass& object) override;
  DropListClass& Add_Head(LinkClass& object) override;
  DropListClass* Remove() override;
  void Zap() override;

  virtual int Add_Item(const char* text);
  virtual const char* Current_Item();
  virtual int Current_Index();
  virtual void Set_Selected_Index(int index);
  virtual void Set_Selected_Index(const char* text);
  void Peer_To_Peer(unsigned flags, KeyNumType&, ControlClass& whom) override;
  void Clear_Focus() override;
  virtual int Count() const { return List.Count(); }
  virtual const char* Get_Item(int index) const { return List.Get_Item(index); }

  void Flag_To_Redraw() override;

  void Expand();
  void Collapse();

  void Set_Position(int x, int y) override;

  // Not copyable -- see LinkClass.
  DropListClass(const DropListClass&) = delete;
  DropListClass& operator=(const DropListClass&) = delete;

  /*
  **	Indicates whether the list box has dropped down or not.
  */
  unsigned IsDropped : 1;

  /*
  **	Height of list box when it is expanded.
  */
  int ListHeight;

  /*
  **	Drop down button.
  */
  ShapeButtonClass DropButton;

  /*
  **	List object when it is expanded.
  */
  ListClass List;
};

template <class T>
class TDropListClass : public EditClass {
 public:
  TDropListClass(int id, char* text, int max_len, TextPrintType flags, int x,
                 int y, int w, int h, const void* up, const void* down);
  TDropListClass(const TDropListClass<T>&) = delete;
  ~TDropListClass() override {}

  T operator[](int index) const { return List[index]; }
  T& operator[](int index) { return List[index]; }

  TDropListClass& Add(LinkClass& object) override;
  TDropListClass& Add_Tail(LinkClass& object) override;
  TDropListClass& Add_Head(LinkClass& object) override;
  TDropListClass* Remove() override;
  void Zap() override;

  virtual int Add_Item(T text);
  virtual T Current_Item();
  virtual int Current_Index();
  virtual void Set_Selected_Index(int index);
  virtual void Set_Selected_Index(T item);
  void Peer_To_Peer(unsigned flags, KeyNumType&, ControlClass& whom) override;
  void Clear_Focus() override;
  virtual int Count() const { return List.Count(); }
  virtual T Get_Item(int index) const { return List.Get_Item(index); }

  void Expand();
  void Collapse();

  void Set_Position(int x, int y) override;

  TDropListClass& operator=(const TDropListClass<T>&) = delete;

  /*
  **	Indicates whether the list box has dropped down or not.
  */
  unsigned IsDropped : 1;

  /*
  **	Height of list box when it is expanded.
  */
  int ListHeight;

  /*
  **	Drop down button.
  */
  ShapeButtonClass DropButton;

  /*
  **	List object when it is expanded.
  */
  TListClass<T> List;
};

template <class T>
TDropListClass<T>::TDropListClass(
    int id,
    char* text,  // NOLINT(readability-non-const-parameter)
    int max_len, TextPrintType flags, int x, int y, int w, int h,
    const void* up, const void* down)
    : EditClass(id, text, max_len, flags, x, y, w, 9, kAlphanumeric),
      IsDropped(false),
      ListHeight(h),
      DropButton(0, down, x + w, y),
      List(0, x, y + Get_Build_Frame_Height(down),
           w + Get_Build_Frame_Width(down), h, flags, up, down) {
  List.Make_Peer(*this);
  DropButton.Make_Peer(*this);
}

template <class T>
void TDropListClass<T>::Zap() {
  Collapse();
  List.Zap();
  DropButton.Zap();
  EditClass::Zap();
}

template <class T>
TDropListClass<T>& TDropListClass<T>::Add(LinkClass& object) {
  DropButton.Add(object);
  return ((TDropListClass&)EditClass::Add(object));
}

template <class T>
TDropListClass<T>& TDropListClass<T>::Add_Tail(LinkClass& object) {
  DropButton.Add_Tail(object);
  return ((TDropListClass&)EditClass::Add_Tail(object));
}

template <class T>
TDropListClass<T>& TDropListClass<T>::Add_Head(LinkClass& object) {
  DropButton.Add_Head(object);
  return ((TDropListClass&)EditClass::Add_Head(object));
}

template <class T>
TDropListClass<T>* TDropListClass<T>::Remove() {
  if (IsDropped) {
    Collapse();
  }
  DropButton.Remove();
  return ((TDropListClass*)EditClass::Remove());
}

template <class T>
int TDropListClass<T>::Add_Item(T item) {
  strncpy(String, item->Description(), MaxLength);
  Flag_To_Redraw();
  return List.Add_Item(item);
}

template <class T>
T TDropListClass<T>::Current_Item() {
  return List.Current_Item();
}

template <class T>
int TDropListClass<T>::Current_Index() {
  return List.Current_Index();
}

template <class T>
void TDropListClass<T>::Set_Selected_Index(int index) {
  if (static_cast<unsigned>(index) < static_cast<unsigned>(List.Count())) {
    List.Set_Selected_Index(index);
    strncpy(String, List.Get_Item(Current_Index())->Description(), MaxLength);
  } else {
    String[0] = '\0';
  }
}

template <class T>
void TDropListClass<T>::Clear_Focus() {
  Collapse();
}

template <class T>
void TDropListClass<T>::Peer_To_Peer(unsigned flags, KeyNumType& key,
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
    strncpy(String, List.Current_Item()->Description(), MaxLength);
    Flag_To_Redraw();
    key = ButtonKey(static_cast<int>(ID));
  }
}

template <class T>
void TDropListClass<T>::Expand() {
  if (!IsDropped) {
    List.X = X;
    List.Y = Y + 9;
    List.Width = Width;
    List.Height = ListHeight;
    List.Add(Head_Of_List());
    List.Flag_To_Redraw();
    IsDropped = true;
  }
}

template <class T>
void TDropListClass<T>::Collapse() {
  if (IsDropped) {
    List.Remove();
    IsDropped = false;
  }
}

template <class T>
void TDropListClass<T>::Set_Position(int x, int y) {
  EditClass::Set_Position(x, y);
  List.Set_Position(x, y + Get_Build_Frame_Height(DropButton.Get_Shape_Data()));
  DropButton.Set_Position(x + Width, y);
}

template <class T>
void TDropListClass<T>::Set_Selected_Index(T text) {
  for (int index = 0; index < Count(); index++) {
    if (text == List.Get_Item(index)) {
      Set_Selected_Index(index);
      break;
    }
  }
}
#endif  // CNC_RED_ALERT_RA_DROP_H_
