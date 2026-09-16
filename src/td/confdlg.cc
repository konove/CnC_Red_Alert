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

/* $Header:   F:\projects\c&c\vcs\code\confdlg.cpv   2.17   16 Oct 1995 16:49:52
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : CONFDLG.CPP *
 *                                                                                             *
 *                   Programmer : Maria del Mar McCready Legg * Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : Jan 30, 1995 *
 *                                                                                             *
 *                  Last Update : Jan 30, 1995   [MML] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * ConfirmationClass::Process -- Handles all the options graphic
 *interface.                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "td/confdlg.h"

#include <algorithm>

#include "port/safe_string.h"
#include "sdllib/font.h"
#include "sdllib/gbuffer.h"
#include "sdllib/keyboard.h"
#include "sdllib/misc.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/wwstd.h"
#include "td/conquer.h"
#include "td/control.h"
#include "td/defines.h"
#include "td/dialog.h"
#include "td/externs.h"
#include "td/gadget.h"
#include "td/globals.h"
#include "td/goptions.h"
#include "td/jshell.h"
#include "td/text.h"
#include "td/textbtn.h"

bool ConfirmationClass::Process(int text) { return Process(Text_String(text)); }

/***********************************************************************************************
 * ConfirmationClass::Process -- Handles all the options graphic interface. *
 *                                                                                             *
 *    This dialog uses an edit box to confirm a deletion. *
 *                                                                                             *
 * INPUT:   	char *string - display in edit box. * OUTPUT:  	none * WARNINGS:
 *none * HISTORY:    12/31/1994 MML : Created. *
 *=============================================================================================*/
bool ConfirmationClass::Process(const char* string) {
  const int factor = SeenBuff.Get_Width() == 320 ? 1 : 2;

  constexpr int kNumOfButtons = 2;

  char buffer[80 * 3];
  bool result = true;
  int width = 0;
  int height = 0;
  int selection = 0;
  TextButtonClass* buttons[kNumOfButtons];

  /*
  **	Set up the window.  Window x-coords are in bytes not pixels.
  */
  port::SafeCopy(buffer, string);
  Fancy_Text_Print(TXT_NONE, 0, 0, kTBlack, kTBlack,
                   TPF_6PT_GRAD | TPF_NOSHADOW);
  Format_Window_String(buffer, 200 * factor, width, height);
  width += 60 * factor;
  height += 60 * factor;
  const int x = ((320 * factor) - width) / 2;
  const int y = ((200 * factor) - height) / 2;

  Set_Logic_Page(SeenBuff);

  /*
  **	Create Buttons.  Button coords are in pixels, but are window-relative.
  */

  const int bheight = FontHeight + FontYSpacing + 2;  // button width and height
  const int bwidth =
      std::max<int>(String_Pixel_Width(Text_String(TXT_YES)) + 8, 30);

  TextButtonClass yesbtn(
      kButtonYes, TXT_YES, TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
      x + (10 * factor), y + height - (bheight + (5 * factor)), bwidth);

  TextButtonClass nobtn(kButtonNo, TXT_NO,
                        TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
                        x + width - (bwidth + (10 * factor)),
                        y + height - (bheight + (5 * factor)), bwidth);

  nobtn.Add_Tail(yesbtn);

  int curbutton = 1;
  buttons[0] = &yesbtn;
  buttons[1] = &nobtn;
  buttons[curbutton]->Turn_On();

  /*
  **	This causes left mouse button clicking within the confines of the dialog
  *to *	be ignored if it wasn't recognized by any other button or slider.
  */
  GadgetClass dialog(x, y, width, height, GadgetClass::kLeftPress);
  dialog.Add_Tail(yesbtn);

  /*
  **	This causes a right click anywhere or a left click outside the dialog
  *region *	to be equivalent to clicking on the return to options dialog.
  */
  ControlClass background(kButtonNo, 0, 0, SeenBuff.Get_Width(),
                          SeenBuff.Get_Height(),
                          GadgetClass::kLeftPress | GadgetClass::kRightPress);
  background.Add_Tail(yesbtn);

  /*
  **	Main Processing Loop.
  */
  bool display = true;
  bool process = true;
  bool pressed = false;
  while (process) {
    /*
    **	Invoke game callback.
    */
    if (GameToPlay == GAME_NORMAL) {
      Call_Back();
    } else {
      if (Main_Loop()) {
        process = false;
        result = false;
      }
    }

    /*
    ** If we have just received input focus again after running in the
    *background then
    ** we need to redraw.
    */
    if (AllSurfaces.SurfacesRestored) {
      AllSurfaces.SurfacesRestored = false;
      display = true;
    }

    /*
    **	Refresh display if needed.
    */
    if (display) {
      Hide_Mouse();

      /*
      **	Draw the background.
      */
      Dialog_Box(x, y, width, height);
      Draw_Caption(TXT_CONFIRMATION, x, y, width);
      Fancy_Text_Print(buffer, x + (20 * factor), y + (30 * factor), kCcGreen,
                       kTBlack, TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

      /*
      **	Draw the titles.
      */
      yesbtn.Draw_All();
      Show_Mouse();
      display = false;
    }

    /*
    **	Get user input.
    */
    const KeyNumType input = yesbtn.Input();

    /*
    **	Process Input.
    */
    switch (static_cast<int>(input)) {
      case ButtonKey(kButtonYes):
        selection = kButtonYes;
        pressed = true;
        break;

      case KN_ESC:
      case ButtonKey(kButtonNo):
        selection = kButtonNo;
        pressed = true;
        break;

      case KN_LEFT:
        buttons[curbutton]->Turn_Off();
        buttons[curbutton]->Flag_To_Redraw();

        curbutton--;
        if (curbutton < 0) {
          curbutton = kNumOfButtons - 1;
        }

        buttons[curbutton]->Turn_On();
        buttons[curbutton]->Flag_To_Redraw();
        break;

      case KN_RIGHT:
        buttons[curbutton]->Turn_Off();
        buttons[curbutton]->Flag_To_Redraw();

        curbutton++;
        if (curbutton > kNumOfButtons - 1) {
          curbutton = 0;
        }

        buttons[curbutton]->Turn_On();
        buttons[curbutton]->Flag_To_Redraw();
        break;

      case KN_RETURN:
        selection = curbutton + kButtonYes;
        pressed = true;
        break;

      default:
        break;
    }

    if (pressed) {
      switch (selection) {
        case kButtonYes:
          result = true;
          process = false;
          break;

        case kButtonNo:
          result = false;
          process = false;
          break;
        default:
          break;
      }

      pressed = false;
    }
  }
  return result;
}
