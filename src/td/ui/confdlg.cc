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

#include "td/ui/confdlg.h"

#include <algorithm>

#include "base/array.h"
#include "base/flags.h"
#include "base/rect.h"
#include "base/strings/safe_string.h"
#include "engine/gfx/font.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/window/hardware_cursor.h"
#include "engine/window/keyboard.h"
#include "game/ui/control.h"
#include "game/ui/gadget.h"
#include "game/ui/gadget_list.h"
#include "game/ui/input.h"
#include "game/ui/standard_colors.h"
#include "td/assets/screen.h"
#include "td/core/defines.h"
#include "td/data/text.h"
#include "td/net/session.h"
#include "td/screens/goptions.h"
#include "td/startup/conquer.h"
#include "td/ui/dialog.h"
#include "td/ui/textbtn.h"

namespace td {

using enum engine::window::KeyNumber;

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
  const int factor = TheScreen().visible_view().width() == 320 ? 1 : 2;

  constexpr int kNumOfButtons = 2;

  char buffer[80 * 3];
  bool result = true;
  int width = 0;
  int height = 0;
  int selection = 0;
  TextButtonClass* buttons[kNumOfButtons];

  PixelView& view = TheScreen().visible_view();

  /*
  **	Set up the window.  Window x-coords are in bytes not pixels.
  */
  base::SafeCopy(buffer, string);
  const FontStyle font = TextFontStyle(FontType::k6PointGradient, TPF_NOSHADOW);
  Format_Window_String(font, buffer, 200 * factor, width, height);
  width += 60 * factor;
  height += 60 * factor;
  const int x = ((320 * factor) - width) / 2;
  const int y = ((200 * factor) - height) / 2;

  /*
  **	Create Buttons.  Button coords are in pixels, but are window-relative.
  */

  const int bheight = FontLineHeight(font) + 2;  // button width and height
  const int bwidth =
      std::max<int>(StringPixelWidth(font, Text_String(TXT_YES)) + 8, 30);

  TextButtonClass yesbtn(kButtonYes, TXT_YES, FontType::k6PointGradient,
                         TPF_USE_GRAD_PAL | TPF_NOSHADOW, x + (10 * factor),
                         y + height - (bheight + (5 * factor)), bwidth);

  TextButtonClass nobtn(kButtonNo, TXT_NO, FontType::k6PointGradient,
                        TPF_USE_GRAD_PAL | TPF_NOSHADOW,
                        x + width - (bwidth + (10 * factor)),
                        y + height - (bheight + (5 * factor)), bwidth);

  game::GadgetList gadgets;
  gadgets.Add(yesbtn);
  gadgets.Add(nobtn);

  int curbutton = 1;
  base::At(buttons, 0) = &yesbtn;
  base::At(buttons, 1) = &nobtn;
  base::At(buttons, curbutton)->Turn_On();

  /*
  **	This causes left mouse button clicking within the confines of the dialog
  *to *	be ignored if it wasn't recognized by any other button or slider.
  */
  game::Gadget dialog(base::Rect{x, y, width, height},
                      game::Gadget::InputFlags::kLeftPress);
  gadgets.Add(dialog);

  /*
  **	This causes a right click anywhere or a left click outside the dialog
  *region *	to be equivalent to clicking on the return to options dialog.
  */
  game::ControlClass background(kButtonNo, 0, 0,
                                TheScreen().visible_view().width(),
                                TheScreen().visible_view().height(),
                                game::Gadget::InputFlags::kLeftPress |
                                    game::Gadget::InputFlags::kRightPress);
  gadgets.Add(background);

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
    if (TheSession().type() == GAME_NORMAL) {
      Call_Back();
    } else {
      if (Main_Loop()) {
        process = false;
        result = false;
      }
    }

    /*
    **	Refresh display if needed.
    */
    if (display) {
      game::TheMouse().Hide();

      /*
      **	Draw the background.
      */
      Dialog_Box(view, x, y, width, height);
      Draw_Caption(view, TXT_CONFIRMATION, x, y, width);
      TextPrinter(view,
                  {kCcGreen, game::kTransparentBlack, FontType::k6PointGradient,
                   TPF_USE_GRAD_PAL | TPF_NOSHADOW})
          .Print(buffer, {x + (20 * factor), y + (30 * factor)});

      /*
      **	Draw the titles.
      */
      gadgets.DrawAll(view);
      game::TheMouse().Show();
      display = false;
    }

    /*
    **	Get user input.
    */
    const engine::window::KeyNumber input = gadgets.Input(view).key;

    /*
    **	Process Input.
    */
    switch (static_cast<int>(input)) {
      case engine::window::ButtonKey(kButtonYes):
        selection = kButtonYes;
        pressed = true;
        break;

      case KN_ESC:
      case engine::window::ButtonKey(kButtonNo):
        selection = kButtonNo;
        pressed = true;
        break;

      case KN_LEFT:
        base::At(buttons, curbutton)->Turn_Off();
        base::At(buttons, curbutton)->FlagToRedraw();

        curbutton--;
        if (curbutton < 0) {
          curbutton = kNumOfButtons - 1;
        }

        base::At(buttons, curbutton)->Turn_On();
        base::At(buttons, curbutton)->FlagToRedraw();
        break;

      case KN_RIGHT:
        base::At(buttons, curbutton)->Turn_Off();
        base::At(buttons, curbutton)->FlagToRedraw();

        curbutton++;
        if (curbutton > kNumOfButtons - 1) {
          curbutton = 0;
        }

        base::At(buttons, curbutton)->Turn_On();
        base::At(buttons, curbutton)->FlagToRedraw();
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

}  // namespace td
