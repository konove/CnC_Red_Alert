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

/* $Header: /CounterStrike/CHEKLIST.H 1     3/03/97 10:24a Joe_bostic $ */
/***************************************************************************
 **   C O N F I D E N T I A L --- W E S T W O O D    S T U D I O S        **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Command & Conquer                        *
 *                                                                         *
 *                    File Name : CHEKLIST.H                               *
 *                                                                         *
 *                   Programmer : Bill Randolph                            *
 *                                                                         *
 *                   Start Date : February 16, 1995                        *
 *                                                                         *
 *                  Last Update : February 16, 1995   [BR]                 *
 *                                                                         *
 *-------------------------------------------------------------------------*
 *	This class behaves just like the standard list box, except that if the
 ** first character of a list entry is a space, clicking on it toggles the
 ** space with a check-mark ('\3').  This makes each entry in the list box
 ** "toggle-able".
 **
 *-------------------------------------------------------------------------*
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef CNC_RED_ALERT_RA_CHEKLIST_H_
#define CNC_RED_ALERT_RA_CHEKLIST_H_

#include <cstddef>
#include <span>
#include <vector>

#include "ra/defines.h"
#include "ra/list.h"
#include "sdllib/keyboard.h"

// A list box whose items each carry a check mark the user can toggle by
// clicking. The checked state lives next to the text, not in it: Get_Item
// returns the item's text alone, and Draw_Entry prepends the glyph.
class CheckListClass : public ListClass {
 public:
  /*
  **	Constructor/Destructor
  */
  CheckListClass(int id, int x, int y, int w, int h, TextPrintType flags,
                 std::span<const std::byte> up,
                 std::span<const std::byte> down);
  ~CheckListClass() override = default;
  CheckListClass(const CheckListClass&) = delete;
  CheckListClass& operator=(const CheckListClass&) = delete;
  CheckListClass(CheckListClass&&) = delete;
  CheckListClass& operator=(CheckListClass&&) = delete;

  int Add_Item(int text) override { return ListClass::Add_Item(text); }
  // Adds an unchecked item.
  int Add_Item(const char* text) override;
  void Remove_Item(const char* text) override { ListClass::Remove_Item(text); }
  void Remove_Item(int index) override;

  /*
  **	Checkmark utility functions
  */
  void Check_Item(int index, bool checked);  // sets checked state of item
  [[nodiscard]] bool Is_Checked(int index) const;  // gets checked state of item

  void Set_Read_Only(bool rdonly) { IsReadOnly = rdonly; }

  /*
  **	This defines the ASCII value of the checkmark character & non-checkmark
  **	character.
  */
  static constexpr int kCheckChar = '\3';
  static constexpr int kUncheckChar = ' ';

 protected:
  bool Action(unsigned flags, KeyNumType& key) override;
  void Draw_Entry(int index, int x, int y, int width, bool selected) override;

 private:
  // Checked state of each item, parallel to List.
  std::vector<bool> Checked;
  bool IsReadOnly{false};
};

#endif  // CNC_RED_ALERT_RA_CHEKLIST_H_
