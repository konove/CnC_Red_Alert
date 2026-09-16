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

/* $Header:   F:\projects\c&c\vcs\code\list.h_v   2.17   16 Oct 1995 16:46:24
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
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

#ifndef CNC_RED_ALERT_TD_LIST_H_
#define CNC_RED_ALERT_TD_LIST_H_

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "absl/base/attributes.h"
#include "sdllib/keyboard.h"
#include "td/control.h"
#include "td/defines.h"
#include "td/gadget.h"
#include "td/link.h"
#include "td/shapebtn.h"
#include "td/slider.h"

/***************************************************************************
 * ListClass -- Like a Windows ListBox structure
 **
 *                                                                         *
 * INPUT:      int x -- x position of gadget
 ** int y -- y position of gadget
 ** int w -- width of gadget
 ** int h -- height of gadget
 ** UWORD flags -- see enumeration choices
 **
 *                                                                         *
 * OUTPUT:     none.
 ** WARNINGS:
 ** HISTORY:    01/03/1995 MML : Created.                                   *
 *=========================================================================*/
class ListClass : public ControlClass {
 public:
  ListClass(int id, int x, int y, int w, int h, TextPrintType flags,
            std::span<const std::byte> up, std::span<const std::byte> down);
  ~ListClass() override;
  ListClass(const ListClass&) = delete;
  ListClass& operator=(const ListClass&) = delete;
  ListClass(ListClass&&) = delete;
  ListClass& operator=(ListClass&&) = delete;

  //		static ListClass * Create_One_Of(int id, int x, int y, int w,
  // int h, TextPrintType flags, void const * up, void const * down);
  // The list owns its text: Add_Item copies the string it is given, so
  // callers may pass a stack buffer and forget it. Pointers from Get_Item and
  // Current_Item stay valid until the item is removed, replaced with
  // Set_Item, or another item is added.

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
  virtual void Set_Tabs(std::span<const int> tabs);
  virtual bool Set_View_Index(int index);
  virtual void Step(bool up);

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
  **	This is the total pixel height of a standar line of text. This is
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

#endif  // CNC_RED_ALERT_TD_LIST_H_
