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

#include <cstddef>
#include <cstring>
#include <span>

#include "absl/base/attributes.h"
#include "base/array.h"
#include "base/numeric.h"
#include "port/safe_string.h"
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
  DropListClass(int id, std::span<char> text, int max_len, TextPrintType flags, int x,
                int y, int w, int h, std::span<const std::byte> up, std::span<const std::byte> down);
  ~DropListClass() override = default;
  DropListClass(DropListClass&&) = delete;
  DropListClass& operator=(DropListClass&&) = delete;

  DropListClass& Add(LinkClass& object) override;
  DropListClass& Add_Tail(LinkClass& object) override;
  DropListClass& Add_Head(LinkClass& object) override;
  DropListClass* Remove() override;
  void Zap() override;

  virtual int Add_Item(const char* text);
  virtual const char* Current_Item() ABSL_ATTRIBUTE_LIFETIME_BOUND;
  virtual int Current_Index();
  virtual void Set_Selected_Index(int index);
  virtual void Set_Selected_Index(const char* text);
  void Peer_To_Peer(unsigned flags, KeyNumType& /*key*/ /*unused*/,
                    ControlClass& whom) override;
  void Clear_Focus() override;
  [[nodiscard]] virtual int Count() const { return List.Count(); }
  [[nodiscard]] virtual const char* Get_Item(int index) const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return List.Get_Item(index);
  }

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
  bool IsDropped : 1 {false};

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
  TDropListClass(int id, std::span<char> text, int max_len, TextPrintType flags, int x,
                 int y, int w, int h, std::span<const std::byte> up, std::span<const std::byte> down);
  TDropListClass(const TDropListClass<T>&) = delete;
  ~TDropListClass() override = default;
  TDropListClass(TDropListClass&&) = delete;
  TDropListClass& operator=(TDropListClass&&) = delete;

  // Returns the indexed list entry through the checked backing vector.
  [[nodiscard]] T at(int index) const { return List.at(index); }
  T operator[](int index) const { return at(index); }
  T& at(int index) { return List.at(index); }
  T& operator[](int index) { return at(index); }

  TDropListClass& Add(LinkClass& object) override;
  TDropListClass& Add_Tail(LinkClass& object) override;
  TDropListClass& Add_Head(LinkClass& object) override;
  TDropListClass* Remove() override;
  void Zap() override;

  virtual int Add_Item(T item);
  virtual T Current_Item();
  virtual int Current_Index();
  virtual void Set_Selected_Index(int index);
  virtual void Set_Selected_Index(T text);
  void Peer_To_Peer(unsigned flags, KeyNumType& /*key*/,
                    ControlClass& whom) override;
  void Clear_Focus() override;
  [[nodiscard]] virtual int Count() const { return List.Count(); }
  [[nodiscard]] virtual T Get_Item(int index) const {
    return List.Get_Item(index);
  }

  void Expand();
  void Collapse();

  void Set_Position(int x, int y) override;

  TDropListClass& operator=(const TDropListClass<T>&) = delete;

  /*
  **	Indicates whether the list box has dropped down or not.
  */
  bool IsDropped : 1 {false};

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
    std::span<char> text,
    int max_len, TextPrintType flags, int x, int y, int w, int h,
    std::span<const std::byte> up, std::span<const std::byte> down)
    : EditClass(id, text, max_len, flags, x, y, w, 9, kAlphanumeric),

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
  return dynamic_cast<TDropListClass&>(EditClass::Add(object));
}

template <class T>
TDropListClass<T>& TDropListClass<T>::Add_Tail(LinkClass& object) {
  DropButton.Add_Tail(object);
  return dynamic_cast<TDropListClass&>(EditClass::Add_Tail(object));
}

template <class T>
TDropListClass<T>& TDropListClass<T>::Add_Head(LinkClass& object) {
  DropButton.Add_Head(object);
  return dynamic_cast<TDropListClass&>(EditClass::Add_Head(object));
}

template <class T>
TDropListClass<T>* TDropListClass<T>::Remove() {
  if (IsDropped) {
    Collapse();
  }
  DropButton.Remove();
  return dynamic_cast<TDropListClass*>(EditClass::Remove());
}

template <class T>
int TDropListClass<T>::Add_Item(T item) {
  port::SafeCopy(String.first(base::ToSize(MaxLength)), item->Description());
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
    port::SafeCopy(String.first(base::ToSize(MaxLength)), List.Get_Item(Current_Index())->Description());
  } else {
    base::At(String, 0) = '\0';
  }
}

template <class T>
void TDropListClass<T>::Clear_Focus() {
  Collapse();
}

template <class T>
void TDropListClass<T>::Peer_To_Peer(unsigned flags, KeyNumType& key,
                                     ControlClass& whom) {
  if ((&whom == &DropButton) && (flags & kLeftRelease)) {
    if (IsDropped) {
      Collapse();
      key = ButtonKey(static_cast<int>(ID));
    } else {
      Expand();
    }
  }

  if (&whom == &List) {
    port::SafeCopy(String.first(base::ToSize(MaxLength)), List.Current_Item()->Description());
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
