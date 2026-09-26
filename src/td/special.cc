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

/* $Header:   F:\projects\c&c\vcs\code\special.cpv   1.4   16 Oct 1995 16:50:06
 * JOE_BOSTIC  $ */
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
 *                  Last Update : May 27, 1995 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "td/special.h"

#include <iterator>

#include "base/array.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/wwstd.h"
#include "engine/window/keyboard.h"
#include "engine/window/ww_mouse.h"
#include "td/checkbox.h"
#include "td/conquer.h"
#include "td/defines.h"
#include "td/dialog.h"
#include "td/event.h"
#include "td/gadget.h"
#include "td/goptions.h"
#include "td/jshell.h"
#include "td/mapedit.h"
#include "td/network.h"
#include "td/queue.h"
#include "td/screen.h"
#include "td/session.h"
#include "td/textbtn.h"
#include "td/world.h"

using enum engine::window::KeyNumber;

#define kOptionWidth 236
#define kOptionHeight 162
#define kOptionX ((320 - kOptionWidth) / 2)
#define kOptionY ((200 - kOptionHeight) / 2)

void Special_Dialog() {
  SpecialClass oldspecial = TheSpecial();
  GadgetClass* buttons = nullptr;
  static struct {
    int Description;
    bool Setting;
    CheckBoxClass* Button;
  } _options[] = {
      //		{TXT_DEFENDER_ADVANTAGE, 0, 0},
      {TXT_SEPARATE_HELIPAD, false, nullptr},
      {TXT_VISIBLE_TARGET, false, nullptr},
      {TXT_TREE_TARGET, false, nullptr},
      {TXT_MCV_DEPLOY, false, nullptr},
      {TXT_SMART_DEFENCE, false, nullptr},
      {TXT_THREE_POINT, false, nullptr},
      //		{TXT_TIBERIUM_GROWTH, 0, 0},
      //		{TXT_TIBERIUM_SPREAD, 0, 0},
      {TXT_TIBERIUM_FAST, false, nullptr},
      {TXT_ROAD_PIECES, false, nullptr},
      {TXT_SCATTER, false, nullptr},
      {TXT_SHOW_NAMES, false, nullptr},
  };

  TextButtonClass ok(200, TXT_OK, TPF_6PT_GRAD | TPF_NOSHADOW, kOptionX + 5,
                     kOptionY + kOptionHeight - 15);
  TextButtonClass cancel(201, TXT_CANCEL, TPF_6PT_GRAD | TPF_NOSHADOW,
                         kOptionX + kOptionWidth - 50,
                         kOptionY + kOptionHeight - 15);
  buttons = &ok;
  cancel.Add(*buttons);

  for (int index = 0; index < std::ssize(_options); index++) {
    base::At(_options, index).Button =
        new CheckBoxClass(static_cast<unsigned>(100 + index), kOptionX + 7,
                          kOptionY + 20 + (index * 10));
    if (base::At(_options, index).Button) {
      base::At(_options, index).Button->Add(*buttons);

      bool value = false;
      switch (base::At(_options, index).Description) {
        case TXT_SEPARATE_HELIPAD:
          value = TheSpecial().IsSeparate;
          break;

        case TXT_SHOW_NAMES:
          value = TheSpecial().IsNamed;
          break;

        case TXT_DEFENDER_ADVANTAGE:
          value = TheSpecial().IsDefenderAdvantage;
          break;

        case TXT_VISIBLE_TARGET:
          value = TheSpecial().IsVisibleTarget;
          break;

        case TXT_TREE_TARGET:
          value = TheSpecial().IsTreeTarget;
          break;

        case TXT_MCV_DEPLOY:
          value = TheSpecial().IsMCVDeploy;
          break;

        case TXT_SMART_DEFENCE:
          value = TheSpecial().IsSmartDefense;
          break;

        case TXT_THREE_POINT:
          value = TheSpecial().IsThreePoint;
          break;

        case TXT_TIBERIUM_GROWTH:
          value = TheSpecial().IsTGrowth;
          break;

        case TXT_TIBERIUM_SPREAD:
          value = TheSpecial().IsTSpread;
          break;

        case TXT_TIBERIUM_FAST:
          value = TheSpecial().IsTFast;
          break;

        case TXT_ROAD_PIECES:
          value = TheSpecial().IsRoad;
          break;

        case TXT_SCATTER:
          value = TheSpecial().IsScatter;
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

    if (TheSession().type() == GAME_NORMAL) {
      Call_Back();
    } else {
      if (Main_Loop()) {
        process = false;
      }
    }

    if (display) {
      display = false;

      Hide_Mouse();
      Dialog_Box(view, kOptionX, kOptionY, kOptionWidth, kOptionHeight);
      Draw_Caption(view, TXT_SPECIAL_OPTIONS, kOptionX, kOptionY, kOptionWidth);

      for (const auto& _option : _options) {
        Fancy_Text_Print(view, _option.Description, _option.Button->X + 10,
                         _option.Button->Y, kCcGreen, kTBlack,
                         TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);
      }
      buttons->Draw_All(view);
      Show_Mouse();
    }

    const engine::window::KeyNumber input = buttons->Input(view).key;
    switch (static_cast<int>(input)) {
      case KN_ESC:
      case engine::window::ButtonKey(200):
        process = false;
        for (const auto& _option : _options) {
          switch (_option.Description) {
            case TXT_SEPARATE_HELIPAD:
              oldspecial.IsSeparate = _option.Setting;
              break;

            case TXT_SHOW_NAMES:
              oldspecial.IsNamed = _option.Setting;
              break;

            case TXT_DEFENDER_ADVANTAGE:
              oldspecial.IsDefenderAdvantage = _option.Setting;
              break;

            case TXT_VISIBLE_TARGET:
              oldspecial.IsVisibleTarget = _option.Setting;
              break;

            case TXT_TREE_TARGET:
              oldspecial.IsTreeTarget = _option.Setting;
              break;

            case TXT_MCV_DEPLOY:
              oldspecial.IsMCVDeploy = _option.Setting;
              break;

            case TXT_SMART_DEFENCE:
              oldspecial.IsSmartDefense = _option.Setting;
              break;

            case TXT_THREE_POINT:
              oldspecial.IsThreePoint = _option.Setting;
              break;

            case TXT_TIBERIUM_GROWTH:
              oldspecial.IsTGrowth = _option.Setting;
              break;

            case TXT_TIBERIUM_SPREAD:
              oldspecial.IsTSpread = _option.Setting;
              break;

            case TXT_TIBERIUM_FAST:
              oldspecial.IsTFast = _option.Setting;
              break;

            case TXT_ROAD_PIECES:
              oldspecial.IsRoad = _option.Setting;
              break;

            case TXT_SCATTER:
              oldspecial.IsScatter = _option.Setting;
              break;
            default:
              break;
          }
        }
        TheNetwork().out_list().Add(EventClass(oldspecial));
        break;

      case engine::window::ButtonKey(201):
        process = false;
        break;

      case KN_NONE:
        break;

      default:
        const int index = ButtonId(input) - 100;
        if (static_cast<unsigned>(index) <
            sizeof(_options) / sizeof(base::At(_options, 0))) {
          base::At(_options, index).Setting =
              !base::At(_options, index).Setting;
          if (base::At(_options, index).Setting) {
            base::At(_options, index).Button->Turn_On();
          } else {
            base::At(_options, index).Button->Turn_Off();
          }
        }
        break;
    }
  }

  TheMap().Revert_Mouse_Shape();
  TheScreen().hidden_page().view().Clear();
  TheMap().Flag_To_Redraw(true);
  TheMap().Render();
}
