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

//	BigCheck.cpp
//	ajw 9/14/98

#include "ra/ui/bigcheck.h"

#include "engine/file/mix_archive.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/shape.h"
#include "engine/window/hardware_cursor.h"
#include "engine/window/keyboard.h"
#include "game/ui/input.h"
#include "game/ui/standard_colors.h"
#include "game/ui/toggle.h"
#include "ra/assets/shape_draw.h"
#include "ra/core/defines.h"
#include "ra/ui/color_scheme.h"
#include "ra/ui/dialog.h"

namespace ra {

using engine::gfx::ShapeFlags;

//***********************************************************************************************
bool BigCheckBoxClass::Draw(PixelView& view, bool forced) {
  if (game::ToggleClass::Draw(view, forced)) {
    game::TheMouse().Hide();

    if (!IsOn) {
      if (!IsDisabled) {
        CC_Draw_Shape(view, MixArchive::RetrieveData("bigcheck.shp"), 0, X, Y,
                      WINDOW_MAIN, ShapeFlags::kNone);
      } else {
        CC_Draw_Shape(view, MixArchive::RetrieveData("bigcheck.shp"), 2, X, Y,
                      WINDOW_MAIN, ShapeFlags::kNone);
      }
    } else {
      if (!IsDisabled) {
        CC_Draw_Shape(view, MixArchive::RetrieveData("bigcheck.shp"), 1, X, Y,
                      WINDOW_MAIN, ShapeFlags::kNone);
      } else {
        CC_Draw_Shape(view, MixArchive::RetrieveData("bigcheck.shp"), 3, X, Y,
                      WINDOW_MAIN, ShapeFlags::kNone);
      }
    }

    const TextPrintType flags = TextFlags;

    //		if( !IsDisabled )
    RemapControlType* pScheme = Get_Color_Scheme();
    //		else
    //		{
    //			pScheme = &GreyScheme;
    //			flags = flags | TPF_MEDIUM_COLOR;
    //		}

    TextPrinter(view, {pScheme, game::kTransparentBlack, TextFont, flags})
        .PrintClipped(szCaption.c_str(),
                      {X + BIGCHECK_OFFSETX, Y + BIGCHECK_OFFSETY}, Width, {});

    game::TheMouse().Show();
    return true;
  }
  return false;
}

//***********************************************************************************************
bool BigCheckBoxClass::Action(unsigned flags, engine::window::KeyNumber& key,
                              int x, int y) {
  /*	if( flags & kLeftPress )
          {
                  if (IsOn) {
                          Turn_Off();
                  } else {
                          Turn_On();
                  }
          }
  */
  return game::ToggleClass::Action(flags, key, x, y);
}

}  // namespace ra
