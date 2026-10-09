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

#ifndef CNC_RED_ALERT_GAME_UI_LIST_H_
#define CNC_RED_ALERT_GAME_UI_LIST_H_

#include <algorithm>
#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "absl/base/attributes.h"
#include "absl/strings/match.h"
#include "base/numeric.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/window/keyboard.h"
#include "game/ui/control.h"
#include "game/ui/gadget.h"
#include "game/ui/gadget_list.h"
#include "game/ui/input.h"
#include "game/ui/link.h"
#include "game/ui/shapebtn.h"
#include "game/ui/slider.h"
#include "game/ui/widget_skin.h"

namespace game {

// A box of text lines, one of them selected, with up and down buttons and a
// slider that appear when there are more lines than fit.
// NOLINTBEGIN(portability-template-virtual-member-function): see WidgetSkin.
template <WidgetSkin Skin>
class ListClass : public ControlClass {
 public:
  ListClass(int id, int x, int y, int w, int h, Skin::Font font,
            Skin::PrintFlags flags, std::span<const std::byte> up,
            std::span<const std::byte> down)
      : ControlClass(static_cast<unsigned>(id), x, y, w, h,
                     kLeftPress | kLeftRelease | kKeyboard, false),
        TextFont(font),
        TextFlags(flags),
        LineHeight(Skin::Text_Height(font, flags) - 1),
        LineCount((h - 1) / LineHeight),
        UpGadget(0, up, x + w, y),
        DownGadget(0, down, x + w, y + h),
        ScrollGadget(0, x + w, y, 0, h, true) {
    // Set preliminary values for the slider related gadgets. They don't
    // automatically appear at this time, but there are some values that can
    // be pre-filled in.
    UpGadget.X -= UpGadget.Width;
    DownGadget.X -= DownGadget.Width;
    DownGadget.Y -= DownGadget.Height;
    ScrollGadget.X -= std::max(UpGadget.Width, DownGadget.Width);
    ScrollGadget.Y = Y + UpGadget.Height;
    ScrollGadget.Height -= UpGadget.Height + DownGadget.Height;
    ScrollGadget.Width = std::max(UpGadget.Width, DownGadget.Width);
  }
  // Not copyable -- see Link.
  ListClass(const ListClass&) = delete;
  ~ListClass() override { ListClass::Remove_Scroll_Bar(); }
  ListClass& operator=(const ListClass&) = delete;
  ListClass(ListClass&&) = delete;
  ListClass& operator=(ListClass&&) = delete;

  // Appends a copy of `text` and returns the new item's index. A nullptr
  // text adds nothing; the returned index is then that of the last item.
  virtual int Add_Item(const char* text) {
    if (text) {
      List.emplace_back(text);
      Flag_To_Redraw();

      // Add scroll gadget if the list gets too large to display all of the
      // items at the same time.
      if (Count() > LineCount) {
        Add_Scroll_Bar();
      }

      // Tell the slider that there is one more entry in the list.
      if (IsScrollActive) {
        ScrollGadget.Set_Maximum(Count());
      }
    }
    return Count() - 1;
  }
  // Appends the text table string `text`; the "no text" number adds nothing.
  virtual int Add_Item(int text) {
    if (text != Skin::kTextNone) {
      Add_Item(Skin::Text_String(text));
    }
    return Count() - 1;
  }

  virtual bool Add_Scroll_Bar() {
    if (IsScrollActive) {
      return false;
    }
    IsScrollActive = true;

    // Flag the list box to be redrawn because it now must be made narrower
    // to accomodate the new slider gadgets.
    Flag_To_Redraw();
    Width -= ScrollGadget.Width;

    // Tell the newly created gadgets that they should inform this list box
    // whenever they get touched. In this way, the list box will
    // automatically be updated under control of the slider buttons.
    UpGadget.Make_Peer(*this);
    DownGadget.Make_Peer(*this);
    ScrollGadget.Make_Peer(*this);

    // Add these gadgets to the same gadget list that the list box is part of.
    if (this->list() != nullptr) {
      AddScrollGadgets(*this->list());
    } else {
      UpGadget.InsertAfter(*this);
      DownGadget.InsertAfter(*this);
      ScrollGadget.InsertAfter(*this);
    }

    // Make sure these added gadgets get redrawn at the next opportunity.
    UpGadget.Flag_To_Redraw();
    DownGadget.Flag_To_Redraw();
    ScrollGadget.Flag_To_Redraw();

    // Inform the slider of the size of the window and the current view
    // position.
    ScrollGadget.Set_Maximum(Count());
    ScrollGadget.Set_Thumb_Size(LineCount);
    ScrollGadget.Set_Value(CurrentTopIndex);
    return true;
  }

  virtual void Bump(bool up) {
    if (IsScrollActive && ScrollGadget.Step(up)) {
      CurrentTopIndex = ScrollGadget.Get_Value();
      Flag_To_Redraw();
    }
  }
  [[nodiscard]] virtual int Count() const {
    return static_cast<int>(List.size());
  }
  [[nodiscard]] virtual int Current_Index() const { return SelectedIndex; }
  // The selected item's text, or nullptr when the list is empty.
  [[nodiscard]] virtual const char* Current_Item() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    if (Count() <= SelectedIndex) {
      return nullptr;
    }
    return List.at(base::ToSize(SelectedIndex)).c_str();
  }

  bool Draw_Me(PixelView& view, bool forced) override {
    // ControlClass::Draw_Me redraws the peer gadget. A list's peer is the
    // drop list that owns it, so going through ControlClass would have the
    // list ask its owner to redraw in the middle of its own draw. Events
    // still reach the peer through ControlClass::Action.
    // NOLINTNEXTLINE(bugprone-parent-virtual-call)
    if (!GadgetClass::Draw_Me(view, forced)) {
      return false;
    }
    Skin::Draw_List_Box(view, X, Y, Width, Height);

    if (Count() > 0) {
      for (int index = 0; index < LineCount; index++) {
        const int line = CurrentTopIndex + index;
        if (Count() > line) {
          // Prints the text and handles right edge clipping and tabs.
          Draw_Entry(view, line, X + 1, Y + (LineHeight * index) + 1, Width - 2,
                     line == SelectedIndex);
        }
      }
    }
    return true;
  }

  // The item's text, or nullptr when the list is empty. An out-of-range
  // index is clamped to the nearest item.
  [[nodiscard]] virtual const char* Get_Item(int index) const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    if (List.empty()) {
      return nullptr;
    }
    index = std::clamp(index, 0, Count() - 1);
    return List.at(base::ToSize(index)).c_str();
  }

  // Moves the selection by `step` lines; returns the old selection.
  virtual int Step_Selected_Index(int step) {
    const int old = SelectedIndex;
    Set_Selected_Index(old + step);
    return old;
  }

  void Flag_To_Redraw() final {
    if (IsScrollActive) {
      UpGadget.Flag_To_Redraw();
      DownGadget.Flag_To_Redraw();
      ScrollGadget.Flag_To_Redraw();
    }
    ControlClass::Flag_To_Redraw();
  }

  void Peer_To_Peer(unsigned flags, engine::window::KeyNumber& /*unused*/,
                    ControlClass& whom) override {
    if (flags & kLeftRelease) {
      if (&whom == &UpGadget) {
        Step(true);
      }
      if (&whom == &DownGadget) {
        Step(false);
      }
    }

    // The slider has changed, so reflect the current list position according
    // to the slider setting.
    if (&whom == &ScrollGadget) {
      Set_View_Index(ScrollGadget.Get_Value());
    }
  }

  // Removes the first item whose text equals `text` (nullptr: nothing).
  virtual void Remove_Item(const char* text) {
    if (text == nullptr) {
      return;
    }
    // Items are copies, so a pointer handed out by Get_Item can only be
    // matched by content; the first equal item is the one such a pointer
    // came from.
    const auto found = std::ranges::find(List, text);
    if (found != List.end()) {
      Remove_Item(static_cast<int>(found - List.begin()));
    }
  }
  // Removes the item at `index`; out-of-range indices are ignored.
  virtual void Remove_Item(int index) {
    if (index < 0 || index >= Count()) {
      return;
    }
    List.erase(List.begin() + index);

    // If the list is now small enough to display completely within the list
    // box region, then delete the slider gadget (if they are present).
    if (Count() <= LineCount) {
      Remove_Scroll_Bar();
    }

    // Tell the slider that there is one less entry in the list.
    if (IsScrollActive) {
      ScrollGadget.Set_Maximum(Count());
    }

    // If we just removed the selected entry, select the previous one.
    if (SelectedIndex >= Count()) {
      SelectedIndex--;
      SelectedIndex = std::max(SelectedIndex, 0);
    }

    // If we just removed the top-displayed entry, step up one item.
    if (CurrentTopIndex >= Count()) {
      CurrentTopIndex--;
      CurrentTopIndex = std::max(CurrentTopIndex, 0);
      if (IsScrollActive) {
        ScrollGadget.Step(true);
      }
    }
  }
  // Removes every item, one at a time through Remove_Item(int), so
  // subclasses that keep per-item state alongside the text stay aligned.
  virtual void Clear() {
    while (Count() > 0) {
      Remove_Item(0);
    }
    Flag_To_Redraw();
  }
  // Replaces the text of the item at `index`; out-of-range indices are
  // ignored.
  void Set_Item(int index, std::string_view text) {
    if (index >= 0 && index < Count()) {
      List.at(base::ToSize(index)).assign(text);
      Flag_To_Redraw();
    }
  }

  bool Remove_Scroll_Bar() {
    if (!IsScrollActive) {
      return false;
    }
    IsScrollActive = false;
    Width += ScrollGadget.Width;
    ScrollGadget.Remove();
    UpGadget.Remove();
    DownGadget.Remove();
    Flag_To_Redraw();
    return true;
  }

  // Selects line `index` and scrolls it into view. An out-of-range index
  // selects the first line in a game whose skin says so, and is ignored in
  // the other.
  virtual void Set_Selected_Index(int index) {
    if (index >= 0 && index < Count()) {
      SelectedIndex = index;
      Flag_To_Redraw();
      if (SelectedIndex < CurrentTopIndex) {
        Set_View_Index(SelectedIndex);
      }
      if (SelectedIndex >= CurrentTopIndex + LineCount) {
        Set_View_Index(SelectedIndex - (LineCount - 1));
      }
    } else if (Skin::kListOutOfRangeSelectsFirst) {
      SelectedIndex = 0;
    }
  }
  // Selects the first line equal to `text`, ignoring case.
  virtual void Set_Selected_Index(const char* text) {
    if (text && Count() > 0) {
      for (int index = 0; index < Count(); index++) {
        if (absl::EqualsIgnoreCase(List.at(base::ToSize(index)), text)) {
          Set_Selected_Index(index);
          break;
        }
      }
    }
  }
  virtual void Set_Tabs(std::span<const int> tabs) { Tabs = tabs; }

  // Scrolls line `index` to the top. Returns whether the view moved.
  virtual bool Set_View_Index(int index) {
    index = std::clamp(index, 0, std::max(0, Count() - LineCount));
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

  virtual void Step(bool up) {
    if (IsScrollActive && ScrollGadget.Step(up)) {
      CurrentTopIndex = ScrollGadget.Get_Value();
      Flag_To_Redraw();
    }
  }

  void Set_Position(int x, int y) override {
    UpGadget.X = x + Width - UpGadget.Width;
    UpGadget.Y = y;
    DownGadget.X = x + Width - DownGadget.Width;
    DownGadget.Y = y + Height - DownGadget.Height;
    ScrollGadget.X = x + Width - std::max(UpGadget.Width, DownGadget.Width);
    ScrollGadget.Y = y + UpGadget.Height;
    ScrollGadget.Height = Height - (UpGadget.Height + DownGadget.Height);
    ScrollGadget.Width = std::max(UpGadget.Width, DownGadget.Width);
  }

  /*
  ** These overloaded list routines handle adding/removing the scroll bar
  ** automatically when the list box is added or removed.
  */
  Link& InsertAfter(Link& list) ABSL_ATTRIBUTE_LIFETIME_BOUND override {
    UnlinkScrollGadgets();
    if (IsScrollActive) {
      ScrollGadget.InsertAfter(list);
      DownGadget.InsertAfter(list);
      UpGadget.InsertAfter(list);
    }
    return ControlClass::InsertAfter(list);
  }
  Link& AppendTo(Link& list) ABSL_ATTRIBUTE_LIFETIME_BOUND override {
    UnlinkScrollGadgets();
    ControlClass::AppendTo(list);
    if (IsScrollActive) {
      UpGadget.AppendTo(list);
      DownGadget.AppendTo(list);
      ScrollGadget.AppendTo(list);
    }
    return HeadOfList();
  }
  Link& PrependTo(Link& list) ABSL_ATTRIBUTE_LIFETIME_BOUND override {
    UnlinkScrollGadgets();
    if (IsScrollActive) {
      ScrollGadget.PrependTo(list);
      DownGadget.PrependTo(list);
      UpGadget.PrependTo(list);
    }
    return ControlClass::PrependTo(list);
  }
  GadgetClass* Remove() override {
    if (IsScrollActive) {
      ScrollGadget.Remove();
      DownGadget.Remove();
      UpGadget.Remove();
    }
    return ControlClass::Remove();
  }

 protected:
  bool Action(unsigned flags, engine::window::KeyNumber& key, int x,
              int y) override {
    using engine::window::KN_DOWN;
    using engine::window::KN_NONE;
    using engine::window::KN_UP;
    if (flags & kLeftRelease) {
      key = KN_NONE;
      flags &= ~kLeftRelease;
      ControlClass::Action(flags, key, x, y);
      return true;
    }

    // Handle keyboard events here.
    if (flags & kKeyboard) {
      // Process the keyboard character. If indicated, consume this keyboard
      // event so that the edit gadget ID number is not returned.
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
      int index = TheMouse().y() - (Y + 1);
      index = index / LineHeight;
      SelectedIndex = CurrentTopIndex + index;
      SelectedIndex = std::min(SelectedIndex, Count() - 1);
      if (SelectedIndex == -1) {
        SelectedIndex = 0;
      }
    }
    return ControlClass::Action(flags, key, x, y);
  }

  virtual void Draw_Entry(PixelView& view, int index, int x, int y, int width,
                          bool selected) {
    Skin::Draw_List_Entry(view, List.at(base::ToSize(index)).c_str(), x, y,
                          width, LineHeight, TextFont, TextFlags, selected,
                          Tabs);
  }

  // The scroll bar follows the list box into a list and out of it.
  void OnAdded(GadgetList& list) override {
    if (IsScrollActive) {
      AddScrollGadgets(list);
    }
  }
  void OnRemoved(GadgetList& list) override {
    if (IsScrollActive) {
      list.Remove(ScrollGadget);
      list.Remove(DownGadget);
      list.Remove(UpGadget);
    }
  }

  // Puts the scroll gadgets behind the list box in `list`.
  void AddScrollGadgets(GadgetList& list) {
    UnlinkScrollGadgets();
    list.AddAfter(UpGadget, *this);
    list.AddAfter(DownGadget, UpGadget);
    list.AddAfter(ScrollGadget, DownGadget);
  }

  // Takes the scroll gadgets out of whatever chain they are in, without
  // touching the focus. Add_Scroll_Bar() chains them behind the list box, also
  // when it is in no list yet, and the functions that add the list box to one
  // then add them again: Link's add functions want nodes that are in no list.
  void UnlinkScrollGadgets() {
    if (IsScrollActive) {
      ScrollGadget.Link::Remove();
      DownGadget.Link::Remove();
      UpGadget.Link::Remove();
    }
  }

  // The font and the print flags the lines render with, as
  // Fancy_Text_Print() takes them.
  Skin::Font TextFont;
  Skin::PrintFlags TextFlags;

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
  **	set by the font.
  */
  int LineHeight;

  /*
  **	This is the number of text lines that can fit within the list box.
  */
  int LineCount;

  /*
  **	If the slider bar has been created, these point to the respective
  **	gadgets that it is composed of.
  */
  bool IsScrollActive : 1 {false};
  ShapeButtonClass<Skin> UpGadget;
  ShapeButtonClass<Skin> DownGadget;
  SliderClass<Skin> ScrollGadget;

  /*
  **	This is the currently selected index. It is highlighted.
  */
  int SelectedIndex{0};

  /*
  **	This specifies the line (index) that is at the top of the list box.
  */
  int CurrentTopIndex{0};
};
// NOLINTEND(portability-template-virtual-member-function)

}  // namespace game

#endif  // CNC_RED_ALERT_GAME_UI_LIST_H_
