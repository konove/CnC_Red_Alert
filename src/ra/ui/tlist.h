// File: TListClass, Red Alert's list box of typed items, each drawn with its
// Description().

#ifndef CNC_RED_ALERT_RA_UI_TLIST_H_
#define CNC_RED_ALERT_RA_UI_TLIST_H_

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "absl/base/attributes.h"
#include "base/algorithm.h"
#include "base/array.h"
#include "base/types.h"
#include "engine/gfx/font.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/wwstd.h"
#include "engine/window/hardware_cursor.h"
#include "engine/window/keyboard.h"
#include "game/ui/control.h"
#include "game/ui/gadget.h"
#include "game/ui/input.h"
#include "game/ui/link.h"
#include "ra/core/defines.h"
#include "ra/core/jshell.h"
#include "ra/data/text_ids.h"
#include "ra/ui/dialog.h"
#include "ra/ui/shapebtn.h"
#include "ra/ui/slider.h"

namespace ra {

template <class T>
class TListClass final : public game::ControlClass {
 public:
  TListClass(int id, int x, int y, int w, int h, FontType font,
             TextPrintType flags, std::span<const std::byte> up,
             std::span<const std::byte> down);
  TListClass(const TListClass<T>&) = delete;
  ~TListClass() override;
  TListClass& operator=(const TListClass&) = delete;
  TListClass(TListClass&&) = delete;
  TListClass& operator=(TListClass&&) = delete;
  // Returns the indexed list entry through the checked backing vector.
  [[nodiscard]] T at(int index) const { return base::At(List, index); }
  T operator[](int index) const { return at(index); }
  T& at(int index) ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return base::At(List, index);
  }
  T& operator[](int index) ABSL_ATTRIBUTE_LIFETIME_BOUND { return at(index); }

  int Add_Item(T text);
  bool Add_Scroll_Bar();
  void Insert_Item(T item);
  void Bump(bool up);
  [[nodiscard]] int Count() const { return static_cast<int>(std::ssize(List)); }
  [[nodiscard]] int Current_Index() const;
  [[nodiscard]] T Current_Item() const;
  bool Draw_Me(PixelView& view, bool forced) override;
  int Step_Selected_Index(int step);
  void Flag_To_Redraw() override;
  [[nodiscard]] T Get_Item(int index) const { return base::At(List, index); }
  // Orders the entries by `less`; equal entries keep the order they were
  // added in.
  template <class Compare>
  void Sort(Compare less) {
    std::ranges::stable_sort(List, less);
    Flag_To_Redraw();
  }

  void Peer_To_Peer(unsigned flags, engine::window::KeyNumber& key,
                    game::ControlClass& whom) override;
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
  game::LinkClass& Add(game::LinkClass& list)
      ABSL_ATTRIBUTE_LIFETIME_BOUND override;
  game::LinkClass& Add_Tail(game::LinkClass& list)
      ABSL_ATTRIBUTE_LIFETIME_BOUND override;
  game::LinkClass& Add_Head(game::LinkClass& list)
      ABSL_ATTRIBUTE_LIFETIME_BOUND override;
  game::GadgetClass* Remove() override;

 protected:
  bool Action(unsigned flags, engine::window::KeyNumber& key, int x,
              int y) override;

  // The font and the print flags the lines render with, as
  // Fancy_Text_Print() takes them.
  FontType TextFont;
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
  std::vector<T> List;

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
TListClass<T>::TListClass(int id, int x, int y, int w, int h, FontType font,
                          TextPrintType flags, std::span<const std::byte> up,
                          std::span<const std::byte> down)
    : game::ControlClass(static_cast<unsigned>(id), x, y, w, h,
                         kLeftPress | kLeftRelease | kKeyboard, false),
      TextFont(font),
      TextFlags(flags),
      LineHeight(FontLineHeight(TextFontStyle(font, flags)) - 1),
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
    List.push_back(item);
  } else {
    List.insert(List.begin() + Current_Index(), item);
  }
}

template <class T>
int TListClass<T>::Add_Item(T text) {
  //	if (text) {
  List.push_back(text);
  Flag_To_Redraw();

  /*
  **	Add scroll gadget if the list gets too large to display all of the items
  **	at the same time.
  */
  if (std::ssize(List) > LineCount) {
    Add_Scroll_Bar();
  }

  /*
  **	Tell the slider that there is one more entry in the list.
  */
  if (IsScrollActive) {
    ScrollGadget.Set_Maximum(static_cast<int>(std::ssize(List)));
  }
  //	}
  return (static_cast<int>(std::ssize(List)) - 1);
}

template <class T>
void TListClass<T>::Remove_Index(int index) {
  if (index >= 0 && index < std::ssize(List)) {
    List.erase(List.begin() + index);

    /*
    **	If the list is now small enough to display completely within the list
    *box region, *	then delete the slider gadget (if they are present).
    */
    if (std::ssize(List) <= LineCount) {
      Remove_Scroll_Bar();
    }

    /*
    **	Tell the slider that there is one less entry in the list.
    */
    if (IsScrollActive) {
      ScrollGadget.Set_Maximum(static_cast<int>(std::ssize(List)));
    }

    /*
    ** If we just removed the selected entry, select the previous one
    */
    if (SelectedIndex >= std::ssize(List)) {
      SelectedIndex--;
      SelectedIndex = std::max(SelectedIndex, 0);
    }

    /*
    ** If we just removed the top-displayed entry, step up one item
    */
    if (CurrentTopIndex >= std::ssize(List)) {
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
  Remove_Index(base::IndexOf(List, text));
}

template <class T>
bool TListClass<T>::Action(unsigned flags, engine::window::KeyNumber& key,
                           int x, int y) {
  if (flags & kLeftRelease) {
    key = engine::window::KN_NONE;
    flags &= (~kLeftRelease);
    game::ControlClass::Action(flags, key, x, y);
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
    if (key == engine::window::KN_UP) {
      Step_Selected_Index(-1);
      key = engine::window::KN_NONE;
    } else if (key == engine::window::KN_DOWN) {
      Step_Selected_Index(1);
      key = engine::window::KN_NONE;
    } else {
      flags &= ~kKeyboard;
    }

  } else {
    int index = game::TheMouse().y() - (Y + 1);
    index = index / LineHeight;
    SelectedIndex = CurrentTopIndex + index;
    SelectedIndex =
        std::min<int>(SelectedIndex, static_cast<int>(std::ssize(List)) - 1);
  }
  return game::ControlClass::Action(flags, key, x, y);
}

template <class T>
bool TListClass<T>::Draw_Me(PixelView& view, bool forced) {
  // As in ListClass::Draw_Me: skipping ControlClass avoids asking the peer
  // drop list to redraw from inside the list's own draw.
  // NOLINTNEXTLINE(bugprone-parent-virtual-call)
  if (game::GadgetClass::Draw_Me(view, forced)) {
    Draw_Box(view, X, Y, Width, Height, BOXSTYLE_BOX, true);

    /*
    **	Draw List.
    */
    if (std::ssize(List)) {
      for (int index = 0; index < LineCount; index++) {
        const int line = CurrentTopIndex + index;

        if (std::ssize(List) > line) {
          /*
          **	Prints the text and handles right edge clipping and tabs.
          */
          base::At(List, line)
              ->Draw_It(view, line, X + 1, Y + (LineHeight * index) + 1,
                        Width - 2, LineHeight, (line == SelectedIndex),
                        TextFont, TextFlags);
          //					List[index].Draw_It(line, X+1,
          // Y+(LineHeight*index)+1, Width-2, LineHeight, (line ==
          // SelectedIndex), TextFlags);
          // Draw_Entry(line, X+1, Y+(LineHeight*index)+1, Width-2, (line ==
          // SelectedIndex));
        }
      }
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
  if (std::ssize(List) <= SelectedIndex) {
    return _temp;
  }
  return base::At(List, SelectedIndex);
}

template <class T>
int TListClass<T>::Current_Index() const {
  return SelectedIndex;
}

template <class T>
void TListClass<T>::Peer_To_Peer(unsigned flags,
                                 engine::window::KeyNumber& /*unused*/,
                                 game::ControlClass& whom) {
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
  index =
      std::clamp<int>(index, 0, static_cast<int>(std::ssize(List)) - LineCount);
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
    ScrollGadget.Set_Maximum(static_cast<int>(std::ssize(List)));
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
game::LinkClass& TListClass<T>::Add(game::LinkClass& list) {
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
  return game::ControlClass::Add(list);
}

template <class T>
game::LinkClass& TListClass<T>::Add_Head(game::LinkClass& list) {
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
  return game::ControlClass::Add_Head(list);
}

template <class T>
game::LinkClass& TListClass<T>::Add_Tail(game::LinkClass& list) {
  /*
  **	Add myself to the list.
  */
  game::ControlClass::Add_Tail(list);

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
game::GadgetClass* TListClass<T>::Remove() {
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
  return game::ControlClass::Remove();
}

template <class T>
void TListClass<T>::Set_Selected_Index(int index) {
  if (index >= 0 && index < std::ssize(List)) {
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
  game::ControlClass::Flag_To_Redraw();
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

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_UI_TLIST_H_
