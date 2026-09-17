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

#include "ra/bigcheck.h"

#include "ra/defines.h"
#include "ra/dialog.h"
#include "ra/gadget.h"
#include "ra/shape_draw.h"
#include "ra/toggle.h"
#include "sdllib/keyboard.h"
#include "sdllib/shape.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/wwstd.h"
#include "tech/mix_archive.h"

//***********************************************************************************************
bool BigCheckBoxClass::Draw_Me(bool forced) {
  if (ToggleClass::Draw_Me(forced)) {
    Hide_Mouse();

    if (!IsOn) {
      if (!IsDisabled) {
        CC_Draw_Shape(MixArchive::RetrieveData("bigcheck.shp"), 0, X, Y,
                      WINDOW_MAIN, SHAPE_NORMAL);
      } else {
        CC_Draw_Shape(MixArchive::RetrieveData("bigcheck.shp"), 2, X, Y,
                      WINDOW_MAIN, SHAPE_NORMAL);
      }
    } else {
      if (!IsDisabled) {
        CC_Draw_Shape(MixArchive::RetrieveData("bigcheck.shp"), 1, X, Y,
                      WINDOW_MAIN, SHAPE_NORMAL);
      } else {
        CC_Draw_Shape(MixArchive::RetrieveData("bigcheck.shp"), 3, X, Y,
                      WINDOW_MAIN, SHAPE_NORMAL);
      }
    }

    const TextPrintType flags = TextFlags;


    //		if( !IsDisabled )
    RemapControlType* pScheme = GadgetClass::Get_Color_Scheme();
    //		else
    //		{
    //			pScheme = &GreyScheme;
    //			flags = flags | TPF_MEDIUM_COLOR;
    //		}

    Conquer_Clip_Text_Print(szCaption.c_str(), X + BIGCHECK_OFFSETX,
                            Y + BIGCHECK_OFFSETY, pScheme, kTBlack, flags,
                            Width, {});

    Show_Mouse();
    return true;
  }
  return false;
}

//***********************************************************************************************
bool BigCheckBoxClass::Action(unsigned flags, KeyNumType& key) {
  /*	if( flags & kLeftPress )
          {
                  if (IsOn) {
                          Turn_Off();
                  } else {
                          Turn_On();
                  }
          }
  */
  return ToggleClass::Action(flags, key);
}
