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

/* $Header: /CounterStrike/SPECIAL.CPP 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : SPECIAL.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 05/27/95 *
 *                                                                                             *
 *                  Last Update : August 20, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * Fetch_Difficulty -- Fetches the difficulty setting desired. *
 *   Special_Dialog -- Handles the special options dialog. * SpecialClass::Init
 *-- Initialize the special class of options.                            *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "ra/ai/special.h"

#include <cstring>
#include <iterator>
#include <memory>
#include <span>
#include <string_view>

#include "base/array.h"
#include "base/strings/safe_string.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/wwstd.h"
#include "engine/window/hardware_cursor.h"
#include "engine/window/keyboard.h"
#include "game/ui/gadget.h"
#include "game/ui/input.h"
#include "ra/assets/screen.h"
#include "ra/core/defines.h"
#include "ra/core/inline.h"
#include "ra/data/rules.h"
#include "ra/data/text_ids.h"
#include "ra/debug/debug.h"
#include "ra/editor/mapedit.h"
#include "ra/map/world.h"
#include "ra/net/event.h"
#include "ra/net/network.h"
#include "ra/net/session.h"
#include "ra/startup/conquer.h"
#include "ra/ui/checkbox.h"
#include "ra/ui/color_scheme.h"
#include "ra/ui/dialog.h"
#include "ra/ui/slider.h"
#include "ra/ui/textbtn.h"

namespace ra {

using enum engine::window::KeyNumber;

#define kOptionWidth (236 * 2)
#define kOptionHeight (162 * 2)
#define kOptionX ((640 - kOptionWidth) / 2)
#define kOptionY ((400 - kOptionHeight) / 2)

/***********************************************************************************************
 * SpecialClass::Init -- Initialize the special class of options. *
 *                                                                                             *
 *    This initialization function is required (as opposed to using a
 *constructor) because     * the SpecialClass is declared as part of a union. A
 *union cannot have a member with a     * constructor. Other than this anomoly,
 *the function serves the same purpose as a          * normal constructor. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/20/1996 JLB : Created. *
 *=============================================================================================*/
void SpecialClass::Init() {
  IsShadowGrow = false;
  IsSpeedBuild = false;
  IsFromInstall = false;
  IsCaptureTheFlag = false;
  IsInert = false;
  IsThreePoint = false;
  IsTGrowth = true;
  IsTSpread = true;
}

/***********************************************************************************************
 * Special_Dialog -- Handles the special options dialog. *
 *                                                                                             *
 *    This dialog is used when setting the special game options. It does not
 *appear in the     * final version of the game. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/21/1995 JLB : Created. *
 *=============================================================================================*/
void Special_Dialog(bool simple) {
  SpecialClass oldspecial = TheSpecial();
  game::GadgetClass* buttons = nullptr;
  struct {
    int Description;
    bool Setting;
    std::unique_ptr<CheckBoxClass> Button;
  } _options[] = {
      {TXT_THREE_POINT, false, nullptr},
      {TXT_SPEED_BUILD, false, nullptr},
  };

  TextButtonClass ok(200, TXT_OK, kButtonFont, kButtonFlags, kOptionX + 30,
                     kOptionY + kOptionHeight - 30);
  TextButtonClass cancel(201, TXT_CANCEL, kButtonFont, kButtonFlags,
                         kOptionX + kOptionWidth - 120,
                         kOptionY + kOptionHeight - 30);
  buttons = &ok;
  cancel.Add(*buttons);

  for (int index = 0; index < std::ssize(_options); index++) {
    base::At(_options, index).Button = std::make_unique<CheckBoxClass>(
        static_cast<unsigned>(100 + index), kOptionX + 34,
        kOptionY + 40 + (index * 20));
    if (base::At(_options, index).Button) {
      base::At(_options, index).Button->Add(*buttons);

      bool value = false;
      switch (base::At(_options, index).Description) {
        case TXT_THREE_POINT:
          value = TheSpecial().IsThreePoint;
          break;

        case TXT_SPEED_BUILD:
          value = TheSpecial().IsSpeedBuild;
          break;
        default:
          break;
      }

      base::At(_options, index).Setting = value;
      if (value) {
        base::At(_options, index).Button->Turn_On();
      } else {
        base::At(_options, index).Button->Turn_Off();
      }
    }
  }

  TheMap().Override_Mouse_Shape(MOUSE_NORMAL);
  PixelView& view = TheScreen().visible_view();
  bool display = true;
  bool process = true;
  while (process) {
    if (TheSession().Type == GAME_NORMAL) {
      ServiceRealTime();
    } else {
      if (RunFrame()) {
        process = false;
      }
    }

    if (display) {
      display = false;

      game::TheMouse().Hide();
      Dialog_Box(kOptionX, kOptionY, kOptionWidth, kOptionHeight);
      Draw_Caption(view, TXT_SPECIAL_OPTIONS, kOptionX, kOptionY, kOptionWidth);

      for (const auto& _option : _options) {
        Fancy_Text_Print(view, _option.Description, _option.Button->X + 20,
                         _option.Button->Y, Get_Color_Scheme(), kTBlack,
                         FontType::k6PointGradient,
                         TPF_USE_GRAD_PAL | TPF_NOSHADOW);
      }
      buttons->Draw_All(view);
      game::TheMouse().Show();
    }

    const engine::window::KeyNumber input = buttons->Input(view).key;
    switch (static_cast<int>(input)) {
      case KN_ESC:
      case engine::window::ButtonKey(200):
        process = false;
        for (const auto& _option : _options) {
          const bool setting = _option.Setting;
          switch (_option.Description) {
            case TXT_THREE_POINT:
              oldspecial.IsThreePoint = setting;
              break;

            case TXT_SPEED_BUILD:
              oldspecial.IsSpeedBuild = setting;
              break;
            default:
              break;
          }
        }
        if (!simple) {
          TheNetwork().out_list().Add(EventClass(oldspecial));
        } else {
          TheSpecial() = oldspecial;
        }
        break;

      case engine::window::ButtonKey(201):
        process = false;
        break;

      case KN_NONE:
        break;

      default:
        const int index = ButtonId(input) - 100;
        if (static_cast<unsigned>(index) <
            sizeof(_options) / sizeof(_options[0])) {
          base::At(_options, index).Setting =
              base::At(_options, index).Button->IsOn;
        }
        break;
    }
  }

  if (!simple) {
    TheMap().Revert_Mouse_Shape();
    TheScreen().hidden_view().Clear();
    TheMap().Flag_To_Redraw(true);
    TheMap().Render();
  }
}

/***********************************************************************************************
 * Fetch_Difficulty -- Fetches the difficulty setting desired. *
 *                                                                                             *
 *    This will display a dialog box that requests the player to specify a
 *difficulty          * setting. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with a difficulty setting of 0 for easiest and 4 for
 *hardest.              *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/13/1996 JLB : Created. *
 *=============================================================================================*/
int Fetch_Difficulty(bool amath) {
  const int w = 500;
  const int h = 160;
  const int x = (640 / 2) - (w / 2);
  const int y = (400 / 2) - (h / 2);
  const int bwidth = 60;

  /*
  **	Fill the description buffer with the description text. Break
  **	the text into appropriate spacing.
  */
  char buffer[512];
  base::SafeCopy(buffer, Text_String(TXT_DIFFICULTY));
  // If it's an aftermath mission, trim the sentence to get rid of the campaign
  // stuff.
  if (amath) {
    int index = 0;
    while (base::At(buffer, index) && base::At(buffer, index) != '.') {
      index++;
    }
    if (base::At(buffer, index) == '.') {
      base::At(buffer, index + 1) = 0;
    }
  }
  PixelView& view = TheScreen().visible_view();
  int width = 0;
  int height = 0;
  Format_Window_String(TextFontStyle(FontType::k6PointGradient, TPF_NOSHADOW),
                       buffer, w - 120, width, height);

  /*
  **	Create the OK button.
  */
  TextButtonClass okbutton(1, TXT_OK, kButtonFont, kButtonFlags,
                           x + w - (bwidth + 40), y + h - 36, bwidth);
  game::GadgetClass* buttonlist = &okbutton;

  /*
  **	Create the slider button.
  */
  SliderClass slider(2, x + 40, y + h - 58, w - 80, 16, true);
  if (TheRules().IsFineDifficulty) {
    slider.Set_Maximum(5);
    slider.Set_Value(2);
  } else {
    slider.Set_Maximum(3);
    slider.Set_Value(1);
  }
  slider.Add(*buttonlist);

  /*
  **	Main Processing Loop.
  */
  bool redraw = true;
  bool process = true;
  while (process) {
    if (redraw) {
      redraw = false;

      /*
      **	Draw the background of the dialog.
      */
      game::TheMouse().Hide();
      Dialog_Box(x, y, w, h);

      /*
      **	Draw the body of the message.
      */
      Fancy_Text_Print(view, buffer, x + 40, y + 30, Get_Color_Scheme(),
                       kTBlack, FontType::k6PointGradient, TPF_NOSHADOW);

      /*
      **	Display the descripton of the slider range.
      */
      Fancy_Text_Print(view, TXT_HARD, slider.X + slider.Width, slider.Y - 18,
                       Get_Color_Scheme(), kTBlack, FontType::k6PointGradient,
                       TPF_RIGHT | TPF_DROPSHADOW);
      Fancy_Text_Print(view, TXT_EASY, slider.X, slider.Y - 18,
                       Get_Color_Scheme(), kTBlack, FontType::k6PointGradient,
                       TPF_DROPSHADOW);
      Fancy_Text_Print(view, TXT_NORMAL, slider.X + (slider.Width / 2),
                       slider.Y - 18, Get_Color_Scheme(), kTBlack,
                       FontType::k6PointGradient, TPF_CENTER | TPF_DROPSHADOW);

      /*
      **	Redraw the buttons.
      */
      if (buttonlist) {
        buttonlist->Draw_All(view);
      }
      game::TheMouse().Show();
    }

    /*
    **	Invoke game callback.
    */
    ServiceRealTime();

    /*
    **	Fetch and process input.
    */
    const engine::window::KeyNumber input = buttonlist->Input(view).key;

    switch (static_cast<int>(input)) {
      case KN_RETURN:
      case 1U | kButtonFlag:
        process = false;
        break;

      default:
        break;
    }
  }

  return slider.Get_Value() * (TheRules().IsFineDifficulty ? 1 : 2);
}

}  // namespace ra
