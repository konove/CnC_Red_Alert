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

/* $Header: /CounterStrike/LIST.H 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : LIST.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 01/15/95 *
 *                                                                                             *
 *                  Last Update : January 15, 1995 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_RA_LIST_H_
#define CNC_RED_ALERT_RA_LIST_H_

#include <span>
#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "absl/base/attributes.h"
#include "base/types.h"
#include "ra/conquer.h"
#include "ra/control.h"
#include "ra/defines.h"
#include "ra/dialog.h"
#include "ra/gadget.h"
#include "ra/globals.h"
#include "ra/jshell.h"
#include "ra/link.h"
#include "ra/shapebtn.h"
#include "ra/slider.h"
#include "ra/vector_dynamic.h"
#include "sdllib/drawbuff.h"
#include "sdllib/font.h"
#include "sdllib/keyboard.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/wwstd.h"

// Scrollable list box widget similar to a Windows ListBox control.
// Displays a list of text items with support for selection, scrolling, and tab
// stops. Automatically manages an optional scroll bar when the list content
// exceeds visible area. Items can be added/removed dynamically and accessed by
// index or text string.
//
// The list owns its text: Add_Item copies the string it is given, so callers
// may pass a stack buffer and forget it. Pointers from Get_Item and
// Current_Item stay valid until the item is removed, replaced with Set_Item,
// or another item is added.
class ListClass : public ControlClass {
 public:
  ListClass(int id, int x, int y, int w, int h, TextPrintType flags,
            std::span<const std::byte> up, std::span<const std::byte> down);
  // Not copyable -- see LinkClass.
  ListClass(const ListClass&) = delete;
  ~ListClass() override;
  ListClass& operator=(const ListClass&) = delete;
  ListClass(ListClass&&) = delete;
  ListClass& operator=(ListClass&&) = delete;

  // Appends a copy of `text` and returns the new item's index. A nullptr
  // text adds nothing; the returned index is then that of the last item.
  virtual int Add_Item(const char* text);
  // Appends the text table string `text`; TXT_NONE adds nothing.
  virtual int Add_Item(int text);
  virtual bool Add_Scroll_Bar();
  virtual void Bump(bool up);
  [[nodiscard]] virtual int Count() const {
    return static_cast<int>(List.size());
  }
  [[nodiscard]] virtual int Current_Index() const;
  // The selected item's text, or nullptr when the list is empty.
  [[nodiscard]] virtual const char* Current_Item() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND;
  bool Draw_Me(bool forced) override;
  // The item's text, or nullptr when the list is empty. An out-of-range
  // index is clamped to the nearest item.
  [[nodiscard]] virtual const char* Get_Item(int index) const
      ABSL_ATTRIBUTE_LIFETIME_BOUND;
  virtual int Step_Selected_Index(int step);
  void Flag_To_Redraw() final;

  void Peer_To_Peer(unsigned flags, KeyNumType& key,
                    ControlClass& whom) override;
  // Removes the first item whose text equals `text` (nullptr: nothing).
  virtual void Remove_Item(const char* text);
  // Removes the item at `index`; out-of-range indices are ignored.
  virtual void Remove_Item(int index);
  // Removes every item, one at a time through Remove_Item(int), so
  // subclasses that keep per-item state alongside the text stay aligned.
  virtual void Clear();
  // Replaces the text of the item at `index`; out-of-range indices are
  // ignored.
  void Set_Item(int index, std::string_view text);
  virtual bool Remove_Scroll_Bar() final;
  virtual void Set_Selected_Index(int index);
  virtual void Set_Selected_Index(const char* text);
  virtual void Set_Tabs(std::span<const int> tabs);
  virtual bool Set_View_Index(int index);
  virtual void Step(bool up);
  void Set_Position(int x, int y) override;

  /*
  ** These overloaded list routines handle adding/removing the scroll bar
  ** automatically when the list box is added or removed.
  */
  LinkClass& Add(LinkClass& list) ABSL_ATTRIBUTE_LIFETIME_BOUND override;
  LinkClass& Add_Tail(LinkClass& list) ABSL_ATTRIBUTE_LIFETIME_BOUND override;
  LinkClass& Add_Head(LinkClass& list) ABSL_ATTRIBUTE_LIFETIME_BOUND override;
  GadgetClass* Remove() override;

 protected:
  bool Action(unsigned flags, KeyNumType& key) override;
  virtual void Draw_Entry(int index, int x, int y, int width, bool selected);

  /*
  **	This controls what the text looks like. It uses the basic TPF_ flags
  *that *	are used to control Fancy_Text_Print().
  */
  TextPrintType TextFlags;

  /*
  **	This is a series of tabstop pixel positions to use when processing any
  **	<TAB> characters found in a list box string. The tabs are a series of
  **	pixel offsets from the starting pixel position of the text.
  */
  std::span<const int> Tabs;

  // The items' text, in display order. Owned by the list.
  std::vector<std::string> List;

  /*
  **	This is the total pixel height of a standard line of text. This is
  *greatly *	influenced by the TextFlags value.
  */
  int LineHeight;

  /*
  **	This is the number of text lines that can fit within the list box.
  */
  int LineCount;

  /*
  **	If the slider bar has been created, these point to the respective
  *gadgets *	that it is composed of.
  */
  bool IsScrollActive : 1 {false};
  ShapeButtonClass UpGadget;
  ShapeButtonClass DownGadget;
  SliderClass ScrollGadget;

  /*
  **	This is the currently selected index. It is highlighted.
  */
  int SelectedIndex{0};

  /*
  **	This specifies the line (index) that is at the top of the list box.
  */
  int CurrentTopIndex{0};
};

template <class T>
class TListClass final : public ControlClass {
 public:
  TListClass(int id, int x, int y, int w, int h, TextPrintType flags,
             std::span<const std::byte> up, std::span<const std::byte> down);
  TListClass(const TListClass<T>&) = delete;
  ~TListClass() override;
  TListClass& operator=(const TListClass&) = delete;
  TListClass(TListClass&&) = delete;
  TListClass& operator=(TListClass&&) = delete;
  T operator[](int index) const { return List[index]; }
  T& operator[](int index) { return List[index]; }

  int Add_Item(T text);
  bool Add_Scroll_Bar();
  void Insert_Item(T item);
  void Bump(bool up);
  [[nodiscard]] int Count() const { return static_cast<int>(List.Count()); }
  [[nodiscard]] int Current_Index() const;
  T Current_Item() const;
  bool Draw_Me(bool forced) override;
  int Step_Selected_Index(int step);
  void Flag_To_Redraw() override;
  [[nodiscard]] T Get_Item(int index) const { return List[index]; }

  void Peer_To_Peer(unsigned flags, KeyNumType& key,
                    ControlClass& whom) override;
  void Remove_Item(T /*text*/);
  void Remove_Index(int /*index*/);
  bool Remove_Scroll_Bar();
  void Set_Selected_Index(int index);
  void Set_Selected_Index(T text);
  void Set_Tabs(std::span<const int> tabs);
  bool Set_View_Index(int index);
  void Step(bool up);
  void Set_Position(int x, int y) override;

  /*
  ** These overloaded list routines handle adding/removing the scroll bar
  ** automatically when the list box is added or removed.
  */
  LinkClass& Add(LinkClass& list) ABSL_ATTRIBUTE_LIFETIME_BOUND override;
  LinkClass& Add_Tail(LinkClass& list) ABSL_ATTRIBUTE_LIFETIME_BOUND override;
  LinkClass& Add_Head(LinkClass& list) ABSL_ATTRIBUTE_LIFETIME_BOUND override;
  GadgetClass* Remove() override;

 protected:
  bool Action(unsigned flags, KeyNumType& key) override;

  /*
  **	This controls what the text looks like. It uses the basic TPF_ flags
  *that *	are used to control Fancy_Text_Print().
  */
  TextPrintType TextFlags;

  /*
  **	This is a series of tabstop pixel positions to use when processing any
  **	<TAB> characters found in a list box string. The tabs are a series of
  **	pixel offsets from the starting pixel position of the text.
  */
  std::span<const int> Tabs;

  /*
  **	The actual list of text pointers is maintained by this list manager.
  */
  DynamicVectorClass<T> List;

  /*
  **	This is the total pixel height of a standard line of text. This is
  *greatly *	influenced by the TextFlags value.
  */
  int LineHeight;

  /*
  **	This is the number of text lines that can fit within the list box.
  */
  int LineCount;

  /*
  **	If the slider bar has been created, these point to the respective
  *gadgets *	that it is composed of.
  */
  bool IsScrollActive : 1 {false};
  ShapeButtonClass UpGadget;
  ShapeButtonClass DownGadget;
  SliderClass ScrollGadget;

  /*
  **	This is the currently selected index. It is highlighted.
  */
  int SelectedIndex{0};

  /*
  **	This specifies the line (index) that is at the top of the list box.
  */
  int CurrentTopIndex{0};
};

template <class T>
TListClass<T>::TListClass(int id, int x, int y, int w, int h,
                          TextPrintType flags, std::span<const std::byte> up, std::span<const std::byte> down)
    : ControlClass(static_cast<unsigned>(id), x, y, w, h,
                   kLeftPress | kLeftRelease | kKeyboard, false),
      TextFlags(flags),
      LineHeight(FontHeight + FontYSpacing - 1),
      LineCount((h - 1) / LineHeight),
      UpGadget(0, up, x + w, y),
      DownGadget(0, down, x + w, y + h),
      ScrollGadget(0, x + w, y, 0, h, true) {
  /*
  **	Set preliminary values for the slider related gadgets. They don't
  *automatically *	appear at this time, but there are some values that can
  *be pre-filled in.
  */
  UpGadget.X -= UpGadget.Width;
  DownGadget.X -= DownGadget.Width;
  DownGadget.Y -= DownGadget.Height;
  ScrollGadget.X -= std::max(UpGadget.Width, DownGadget.Width);
  ScrollGadget.Y = Y + UpGadget.Height;
  ScrollGadget.Height -= UpGadget.Height + DownGadget.Height;
  ScrollGadget.Width = std::max(UpGadget.Width, DownGadget.Width);

  /*
  **	Set the list box to a default state.
  */
  Fancy_Text_Print(TXT_NONE, 0, 0, nullptr, kTBlack, TextFlags);
}

template <class T>
void TListClass<T>::Set_Position(int x, int y) {
  UpGadget.X = x + Width - UpGadget.Width;
  UpGadget.Y = y;
  DownGadget.X = x + Width - DownGadget.Width;
  DownGadget.Y = y + Height - DownGadget.Height;
  ScrollGadget.X = x + Width - std::max(UpGadget.Width, DownGadget.Width);
  ScrollGadget.Y = y + UpGadget.Height;
  ScrollGadget.Height = Height - (UpGadget.Height + DownGadget.Height);
  ScrollGadget.Width = std::max(UpGadget.Width, DownGadget.Width);
}

template <class T>
TListClass<T>::~TListClass() {
  Remove_Scroll_Bar();
}

template <class T>
void TListClass<T>::Insert_Item(T item) {
  if (Current_Index() >= Count()) {
    List.Add(item);
  } else {
    List.Add(item);

    /*
    **	Move all trailing items upward.
    */
    for (base::ssize index = List.Count() - 1; index >= Current_Index(); index--) {
      List[index + 1] = List[index];
    }

    /*
    **	Insert the new item into the location at the current index.
    */
    List[Current_Index()] = item;
  }
}

template <class T>
int TListClass<T>::Add_Item(T text) {
  //	if (text) {
  List.Add(text);
  Flag_To_Redraw();

  /*
  **	Add scroll gadget if the list gets too large to display all of the items
  **	at the same time.
  */
  if (List.Count() > LineCount) {
    Add_Scroll_Bar();
  }

  /*
  **	Tell the slider that there is one more entry in the list.
  */
  if (IsScrollActive) {
    ScrollGadget.Set_Maximum(static_cast<int>(List.Count()));
  }
  //	}
  return (static_cast<int>(List.Count()) - 1);
}

template <class T>
void TListClass<T>::Remove_Index(int index) {
  if (index >= 0 && index < List.Count()) {
    List.Delete(index);

    /*
    **	If the list is now small enough to display completely within the list
    *box region, *	then delete the slider gadget (if they are present).
    */
    if (List.Count() <= LineCount) {
      Remove_Scroll_Bar();
    }

    /*
    **	Tell the slider that there is one less entry in the list.
    */
    if (IsScrollActive) {
      ScrollGadget.Set_Maximum(static_cast<int>(List.Count()));
    }

    /*
    ** If we just removed the selected entry, select the previous one
    */
    if (SelectedIndex >= List.Count()) {
      SelectedIndex--;
      SelectedIndex = std::max(SelectedIndex, 0);
    }

    /*
    ** If we just removed the top-displayed entry, step up one item
    */
    if (CurrentTopIndex >= List.Count()) {
      CurrentTopIndex--;
      CurrentTopIndex = std::max(CurrentTopIndex, 0);
      if (IsScrollActive) {
        ScrollGadget.Step(true);
      }
    }
  }
}

template <class T>
void TListClass<T>::Remove_Item(T text) {
  Remove_Index(List.ID(text));
}

template <class T>
bool TListClass<T>::Action(unsigned flags, KeyNumType& key) {
  if (flags & kLeftRelease) {
    key = KN_NONE;
    flags &= (~kLeftRelease);
    ControlClass::Action(flags, key);
    return true;
  }
  /*
   ** Handle keyboard events here.
   */
  if (flags & kKeyboard) {
    /*
    **	Process the keyboard character. If indicated, consume this
    *keyboard event *	so that the edit gadget ID number is not returned.
    */
    if (key == KN_UP) {
      Step_Selected_Index(-1);
      key = KN_NONE;
    } else if (key == KN_DOWN) {
      Step_Selected_Index(1);
      key = KN_NONE;
    } else {
      flags &= ~kKeyboard;
    }

  } else {
    int index = Get_Mouse_Y() - (Y + 1);
    index = index / LineHeight;
    SelectedIndex = CurrentTopIndex + index;
    SelectedIndex = std::min<int>(SelectedIndex, static_cast<int>(List.Count()) - 1);
  }
  return ControlClass::Action(flags, key);
}

template <class T>
bool TListClass<T>::Draw_Me(bool forced) {
  // As in ListClass::Draw_Me: skipping ControlClass avoids asking the peer
  // drop list to redraw from inside the list's own draw.
  // NOLINTNEXTLINE(bugprone-parent-virtual-call)
  if (GadgetClass::Draw_Me(forced)) {
    /*
    **	Turn off the mouse.
    */
    if (LogicPage == &SeenBuff) {
      Conditional_Hide_Mouse(X, Y, X + Width, Y + Height);
    }

    Draw_Box(X, Y, Width, Height, BOXSTYLE_BOX, true);

    /*
    **	Draw List.
    */
    if (List.Count()) {
      for (int index = 0; index < LineCount; index++) {
        const int line = CurrentTopIndex + index;

        if (List.Count() > line) {
          /*
          **	Prints the text and handles right edge clipping and tabs.
          */
          List[line]->Draw_It(line, X + 1, Y + (LineHeight * index) + 1,
                              Width - 2, LineHeight, (line == SelectedIndex),
                              TextFlags);
          //					List[index].Draw_It(line, X+1,
          // Y+(LineHeight*index)+1, Width-2, LineHeight, (line ==
          // SelectedIndex), TextFlags);
          // Draw_Entry(line, X+1, Y+(LineHeight*index)+1, Width-2, (line ==
          // SelectedIndex));
        }
      }
    }

    /*
    **	Turn on the mouse.
    */
    if (LogicPage == &SeenBuff) {
      Conditional_Show_Mouse();
    }
    return true;
  }
  return false;
}

template <class T>
void TListClass<T>::Bump(bool up) {
  if (IsScrollActive && ScrollGadget.Step(up)) {
    CurrentTopIndex = ScrollGadget.Get_Value();
    Flag_To_Redraw();
  }
}

template <class T>
void TListClass<T>::Step(bool up) {
  if (IsScrollActive && ScrollGadget.Step(up)) {
    CurrentTopIndex = ScrollGadget.Get_Value();
    Flag_To_Redraw();
  }
}

template <class T>
T TListClass<T>::Current_Item() const {
  static T _temp;
  if (List.Count() <= SelectedIndex) {
    return _temp;
  }
  return List[SelectedIndex];
}

template <class T>
int TListClass<T>::Current_Index() const {
  return SelectedIndex;
}

template <class T>
void TListClass<T>::Peer_To_Peer(unsigned flags, KeyNumType& /*unused*/,
                                 ControlClass& whom) {
  if (flags & kLeftRelease) {
    if (&whom == &UpGadget) {
      Step(true);
    }
    if (&whom == &DownGadget) {
      Step(false);
    }
  }

  /*
  **	The slider has changed, so reflect the current list position
  **	according to the slider setting.
  */
  if (&whom == &ScrollGadget) {
    Set_View_Index(ScrollGadget.Get_Value());
  }
}

template <class T>
bool TListClass<T>::Set_View_Index(int index) {
  index = std::clamp<int>(index, 0, static_cast<int>(List.Count()) - LineCount);
  if (index != CurrentTopIndex) {
    CurrentTopIndex = index;
    Flag_To_Redraw();
    if (IsScrollActive) {
      ScrollGadget.Set_Value(CurrentTopIndex);
    }
    return true;
  }
  return false;
}

template <class T>
bool TListClass<T>::Add_Scroll_Bar() {
  if (!IsScrollActive) {
    IsScrollActive = true;

    /*
    **	Everything has been created successfully. Flag the list box to be
    **	redrawn because it now must be made narrower to accomodate the new
    **	slider gadgets.
    */
    Flag_To_Redraw();
    Width -= ScrollGadget.Width;

    /*
    **	Tell the newly created gadgets that they should inform this list box
    **	whenever they get touched. In this way, the list box will automatically
    **	be updated under control of the slider buttons.
    */
    UpGadget.Make_Peer(*this);
    DownGadget.Make_Peer(*this);
    ScrollGadget.Make_Peer(*this);

    /*
    **	Add these newly created gadgets to the same gadget list that the
    **	list box is part of.
    */
    UpGadget.Add(*this);
    DownGadget.Add(*this);
    ScrollGadget.Add(*this);

    /*
    **	Make sure these added gadgets get redrawn at the next opportunity.
    */
    UpGadget.Flag_To_Redraw();
    DownGadget.Flag_To_Redraw();
    ScrollGadget.Flag_To_Redraw();

    /*
    **	Inform the slider of the size of the window and the current view
    *position.
    */
    ScrollGadget.Set_Maximum(static_cast<int>(List.Count()));
    ScrollGadget.Set_Thumb_Size(LineCount);
    ScrollGadget.Set_Value(CurrentTopIndex);

    /*
    **	Return with success flag.
    */
    return true;
  }
  return false;
}

template <class T>
bool TListClass<T>::Remove_Scroll_Bar() {
  if (IsScrollActive) {
    IsScrollActive = false;
    Width += ScrollGadget.Width;
    ScrollGadget.Remove();
    UpGadget.Remove();
    DownGadget.Remove();
    Flag_To_Redraw();
    return true;
  }
  return false;
}

template <class T>
void TListClass<T>::Set_Tabs(std::span<const int> tabs) {
  Tabs = tabs;
}

template <class T>
LinkClass& TListClass<T>::Add(LinkClass& list) {
  /*
  **	Add the scroll bar gadgets if they're active.
  */
  if (IsScrollActive) {
    ScrollGadget.Add(list);
    DownGadget.Add(list);
    UpGadget.Add(list);
  }

  /*
  **	Add myself to the list, then return.
  */
  return ControlClass::Add(list);
}

template <class T>
LinkClass& TListClass<T>::Add_Head(LinkClass& list) {
  /*
  **	Add the scroll bar gadgets if they're active.
  */
  if (IsScrollActive) {
    ScrollGadget.Add_Head(list);
    DownGadget.Add_Head(list);
    UpGadget.Add_Head(list);
  }

  /*
  **	Add myself to the list, then return.
  */
  return ControlClass::Add_Head(list);
}

template <class T>
LinkClass& TListClass<T>::Add_Tail(LinkClass& list) {
  /*
  **	Add myself to the list.
  */
  ControlClass::Add_Tail(list);

  /*
  **	Add the scroll bar gadgets if they're active.
  */
  if (IsScrollActive) {
    UpGadget.Add_Tail(list);
    DownGadget.Add_Tail(list);
    ScrollGadget.Add_Tail(list);
  }

  return Head_Of_List();
}

template <class T>
GadgetClass* TListClass<T>::Remove() {
  /*
  **	Remove the scroll bar if it's active
  */
  if (IsScrollActive) {
    ScrollGadget.Remove();
    DownGadget.Remove();
    UpGadget.Remove();
  }

  /*
  **	Remove myself & return
  */
  return ControlClass::Remove();
}

template <class T>
void TListClass<T>::Set_Selected_Index(int index) {
  if (index >= 0 && index < List.Count()) {
    SelectedIndex = index;
    Flag_To_Redraw();
    if (SelectedIndex < CurrentTopIndex) {
      Set_View_Index(SelectedIndex);
    }
    if (SelectedIndex >= CurrentTopIndex + LineCount) {
      Set_View_Index(SelectedIndex - (LineCount - 1));
    }
  }
}

template <class T>
int TListClass<T>::Step_Selected_Index(int step) {
  const int old = SelectedIndex;

  Set_Selected_Index(old + step);
  return old;
}

template <class T>
void TListClass<T>::Flag_To_Redraw() {
  if (IsScrollActive) {
    UpGadget.Flag_To_Redraw();
    DownGadget.Flag_To_Redraw();
    ScrollGadget.Flag_To_Redraw();
  }
  ControlClass::Flag_To_Redraw();
}

template <class T>
void TListClass<T>::Set_Selected_Index(T text) {
  for (int index = 0; index < Count(); index++) {
    if (text == Get_Item(index)) {
      Set_Selected_Index(index);
      break;
    }
  }
}
#endif  // CNC_RED_ALERT_RA_LIST_H_
