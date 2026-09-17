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

//	ToolTip.cpp

#include "ra/tooltip.h"

#include <cstring>
#include <string_view>

#include "base/numeric.h"
#include "base/types.h"
#include "port/safe_string.h"
#include "ra/defines.h"
#include "ra/dialog.h"
#include "ra/externs.h"
#include "ra/gadget.h"
#include "ra/iconlist.h"
#include "ra/text_ids.h"
#include "ra/winbits.h"
#include "sdllib/font.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/wwstd.h"

namespace {

// The gadget this tooltip is bound to, when bIconList says it is a list whose
// lines each carry their own help text. Null if it turns out not to be one.
IconListClass* AsIconList(GadgetClass* gadget) {
  return dynamic_cast<IconListClass*>(gadget);
}

}  // namespace

//***********************************************************************************************
ToolTipClass::ToolTipClass(GadgetClass* gadget, const char* szText, int x_show,
                           int y_show, bool right_align /* = false */,
                           bool icon_list /*= false */)
    : pGadget(gadget),
      bRightAlign(right_align),

      bIconList(icon_list),
      xShow(x_show),
      yShow(y_show)

{
  if (szText != nullptr &&
      std::string_view(szText).size() > TOOLTIPTEXT_MAX_LEN) {
    port::SafeCopy(szTip, "Tooltip too long!");
  } else {
    port::SafeCopy(szTip, szText != nullptr ? szText : "");
  }

  Set_Font(TypeFontPtr);
  Fancy_Text_Print(TXT_NONE, 0, 0, nullptr, kTBlack,
                   TPF_TYPE);  //	Required before String_Pixel_Width()
                               // call, for god's sake.
  wShow = String_Pixel_Width(szTip) + 2;
  hShow = 11;

  if (!bIconList) {
    //	Else it is reallocated on every draw.
    pSaveRect.resize(base::ToSize(base::ssize{wShow} * hShow));
    if (bRightAlign) {
      xShow -= wShow;
    }
  } else {
    pSaveRect.clear();
  }

  //	bIconList is true if tooltips appear for individual line items in an
  // iconlist. 	szText in this case is ignored. 	yShow is the y position
  // of the top row's tooltip - other rows will be offset from here.
}

//***********************************************************************************************
ToolTipClass* ToolTipClass::GetToolTipHit() {
  //	Returns 'this' if the mouse is over gadget bound to tooltip.
  //	Otherwise calls the same function in the next tooltip in the list of
  // which *this is a part.
  if (bGadgetHit()) {
    return this;
  }
  if (next) {
    return next->GetToolTipHit();
  }
  return nullptr;
}

//***********************************************************************************************
bool ToolTipClass::bGadgetHit() const {
  //	Returns true if the mouse is currently over the gadget to which *this is
  // bound.
  const int x = Get_Mouse_X();
  const int y = Get_Mouse_Y();
  return x > pGadget->X && x < pGadget->X + pGadget->Width && y > pGadget->Y &&
         y < pGadget->Y + pGadget->Height;
}

//***********************************************************************************************
void ToolTipClass::Move(int x_show, int y_show) {
  bool bRestoreShow = false;
  if (bShowing) {
    bRestoreShow = true;
    Unshow();
  }
  this->xShow = x_show;
  if ((!bIconList) && bRightAlign) {
    this->xShow -= wShow;
  }

  this->yShow = y_show;
  if (bRestoreShow) {
    Show();
  }
}

//***********************************************************************************************
void ToolTipClass::Show() {
  if (!bShowing) {
    Set_Font(TypeFontPtr);
    int xShowUse = xShow;
    int yShowUse = 0;
    int wShowUse = 0;
    const char* szTipUse = nullptr;
    if (!bIconList) {
      yShowUse = yShow;
      wShowUse = wShow;
      szTipUse = szTip;
    } else {
      IconListClass* pIconList = AsIconList(pGadget);
      if (pIconList == nullptr) {
        bShowing = true;
        return;
      }
      iLastIconListIndex = pIconList->IndexUnderMouse();
      if (iLastIconListIndex < 0) {
        //	Nothing to show.
        bShowing = true;
        return;
      }
      yShowUse = pIconList->OffsetToIndex(iLastIconListIndex, yShow);
      szTipUse = pIconList->Get_Item_Help(iLastIconListIndex);
      if (!szTipUse || *szTipUse == 0) {
        //	Nothing to show.
        bShowing = true;
        bLastShowNoText = true;
        return;
      }
      Fancy_Text_Print(TXT_NONE, 0, 0, nullptr, kTBlack,
                       TPF_TYPE);  //	Required before String_Pixel_Width()
                                   // call, for god's sake.
      wShowUse = String_Pixel_Width(szTipUse) + 2;
      if (bRightAlign) {
        xShowUse -= wShowUse;
      }
      pSaveRect.resize(base::ToSize(base::ssize{wShowUse} * hShow));
      bLastShowNoText = false;
      xLastShow = xShowUse;
      yLastShow = yShowUse;
      wLastShow = wShowUse;
    }

    //	Save rect about to be corrupted.
    Hide_Mouse();
    SaveSurfaceRect(xShowUse, yShowUse, wShowUse, hShow, pSaveRect,
                    WINDOW_MAIN);
    //	Draw text.
    // Simple_Text_Print( szTipUse, xShowUse, yShowUse,
    // GadgetClass::Get_Color_Scheme(), ColorRemaps[ PCOLOR_BROWN ].Color,
    // TPF_TYPE ); //TPF_DROPSHADOW );
    Simple_Text_Print(szTipUse, xShowUse, yShowUse,
                      GadgetClass::Get_Color_Scheme(), kBlack,
                      TPF_TYPE);  // TPF_DROPSHADOW );
    //	Draw bounding rect.
    //		LogicPage->Draw_Rect( xShowUse, yShowUse, xShowUse + wShowUse -
    // 1, yShowUse + hShow - 1, ColorRemaps[ PCOLOR_GOLD ].Color );
    Draw_Box(xShowUse, yShowUse, wShowUse, hShow, BOXSTYLE_BOX, false);
    Show_Mouse();
    bShowing = true;
  }
}

//***********************************************************************************************
void ToolTipClass::Unshow() {
  if (bShowing) {
    int xShowUse = 0;
    int yShowUse = 0;
    int wShowUse = 0;
    if (!bIconList) {
      xShowUse = xShow;
      wShowUse = wShow;
      yShowUse = yShow;
    } else {
      if (iLastIconListIndex == -1 || bLastShowNoText) {
        //	Nothing to restore.
        bShowing = false;
        return;
      }
      //	(Can't rely on iconlist being the same as when Show() occurred.)
      //			IconListClass* pIconList =
      //(IconListClass*)pGadget; 			yShowUse =
      // pIconList->OffsetToIndex( iLastIconListIndex, yShow );
      // const char* szTipUsed = pIconList->Get_Item_Help( iLastIconListIndex );
      // if( !szTipUsed || *szTipUsed == 0 )
      //			{
      //				//	Nothing to restore.
      //				bShowing = false;
      //				return;
      //			}
      //			Fancy_Text_Print( TXT_NONE, 0, 0, TBLACK,
      // TBLACK, TPF_TYPE );	//	Required before String_Pixel_Width()
      // call, for god's sake. 			wShowUse = String_Pixel_Width(
      // szTipUsed ) + 2; 			if( bRightAlign )
      // xShowUse -= wShowUse;
      xShowUse = xLastShow;
      yShowUse = yLastShow;
      wShowUse = wLastShow;
    }
    Hide_Mouse();
    RestoreSurfaceRect(xShowUse, yShowUse, wShowUse, hShow, pSaveRect,
                       WINDOW_MAIN);
    Show_Mouse();
    bShowing = false;
  }
}

//***********************************************************************************************
bool ToolTipClass::bOverDifferentLine() const {
  //	bIconList must be true if this is being used.
  //	Returns true if the iconlist line that the mouse is over is different
  // than the last time Show() was called.
  IconListClass* pIconList = AsIconList(pGadget);
  return pIconList != nullptr &&
         pIconList->IndexUnderMouse() != iLastIconListIndex;
}
