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

/* $Header:   F:\projects\c&c\vcs\code\colrlist.h_v   1.10   16 Oct 1995
 * 16:47:16   JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : COLRLIST.H *
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

#ifndef CNC_RED_ALERT_TD_UI_COLRLIST_H_
#define CNC_RED_ALERT_TD_UI_COLRLIST_H_

#include <cstddef>
#include <optional>
#include <span>
#include <vector>

#include "engine/gfx/palette_index.h"
#include "engine/gfx/pixel_buffer.h"
#include "game/ui/standard_colors.h"
#include "td/core/defines.h"
#include "td/ui/list.h"

namespace td {

/***************************************************************************
** This class adds the ability for every list item to have a different color.
*/
class ColorListClass : public ListClass {
 public:
  /*********************************************************************
  ** These enums are the ways a selected item can be drawn
  */
  enum class SelectStyleType {
    SELECT_NONE,       // selected items aren't drawn differently
    SELECT_HIGHLIGHT,  // item is highlighted
    SELECT_BOX,        // draw a box around the item
    SELECT_BAR,        // draw a bar behind the item
    SELECT_INVERT,     // draw the string inverted
  };
  using enum SelectStyleType;

  ColorListClass(int id, int x, int y, int w, int h, FontType font,
                 TextPrintType flags, std::span<const std::byte> up,
                 std::span<const std::byte> down);
  ~ColorListClass() override;
  ColorListClass(const ColorListClass&) = delete;
  ColorListClass& operator=(const ColorListClass&) = delete;
  ColorListClass(ColorListClass&&) = delete;
  ColorListClass& operator=(ColorListClass&&) = delete;

  int Add_Item(const char* text) override {
    return Add_Item(text, game::kWhite);
  }
  int Add_Item(int text) override { return Add_Item(text, game::kWhite); }
  virtual int Add_Item(const char* text, engine::gfx::PaletteIndex color);
  virtual int Add_Item(int text, engine::gfx::PaletteIndex color);
  void Remove_Item(const char* text) override { ListClass::Remove_Item(text); }
  void Remove_Item(int index) override;

  // Sets how the selected item is drawn. `color` is what the style draws
  // with; without one a style draws in the item's own color.
  virtual void Set_Selected_Style(
      SelectStyleType style,
      std::optional<engine::gfx::PaletteIndex> color = std::nullopt);

  /*
  **	This is the list of colors for each item.
  */
  std::vector<engine::gfx::PaletteIndex> Colors;

 protected:
  void Draw_Entry(PixelView& view, int index, int x, int y, int width,
                  bool selected) override;

  /*
  **	This tells how to draw the selected item.
  */
  SelectStyleType Style{SELECT_HIGHLIGHT};
  std::optional<engine::gfx::PaletteIndex>
      SelectColor;  // nullopt: the item's own.
};

}  // namespace td

#endif  // CNC_RED_ALERT_TD_UI_COLRLIST_H_
