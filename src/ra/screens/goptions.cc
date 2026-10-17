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

/* $Header: /counterstrike/GOPTIONS.CPP 6     3/15/97 7:18p Steve_tall $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : OPTIONS.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : June 8, 1994 *
 *                                                                                             *
 *                  Last Update : July 27, 1995 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * OptionsClass::Process -- Handles all the options graphic
 *interface.                       *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "ra/screens/goptions.h"

#include <algorithm>
#include <iterator>
#include <memory>
#include <vector>

#include "absl/strings/str_format.h"
#include "base/array.h"
#include "base/flags.h"
#include "base/point.h"
#include "base/rect.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/window/hardware_cursor.h"
#include "engine/window/keyboard.h"
#include "game/ui/control.h"
#include "game/ui/gadget.h"
#include "game/ui/gadget_list.h"
#include "game/ui/input.h"
#include "game/ui/standard_colors.h"
#include "ra/ai/scenario.h"
#include "ra/assets/palette.h"
#include "ra/assets/palettes.h"
#include "ra/assets/screen.h"
#include "ra/assets/theme.h"
#include "ra/core/defines.h"
#include "ra/data/text_ids.h"
#include "ra/editor/mapedit.h"
#include "ra/map/tab.h"
#include "ra/map/world.h"
#include "ra/net/event.h"
#include "ra/net/mplayer.h"
#include "ra/net/network.h"
#include "ra/net/queue.h"
#include "ra/net/session.h"
#include "ra/objects/house.h"
#include "ra/screens/gamedlg.h"
#include "ra/screens/loaddlg.h"
#include "ra/screens/movie.h"
#include "ra/startup/conquer.h"
#include "ra/startup/game_state.h"
#include "ra/startup/version.h"
#include "ra/ui/color_scheme.h"
#include "ra/ui/dialog.h"
#include "ra/ui/msgbox.h"
#include "ra/ui/textbtn.h"
#include "ra/wol/wolstrng.h"

namespace ra {

using enum engine::window::KeyNumber;

/***********************************************************************************************
 * OptionsClass::Process -- Handles all the options graphic interface. *
 *                                                                                             *
 *    This routine is the main control for the visual representation of the
 *options            * screen. It handles the visual overlay and the player
 *input.                              *
 *                                                                                             *
 * INPUT:      none *
 *                                                                                             *
 * OUTPUT:     none *
 *                                                                                             *
 * WARNINGS:      none *
 *                                                                                             *
 * HISTORY:     12/31/1994 MML : Created. * 06/23/1995 JLB : Handles restating
 *the mission objective.                                 * 07/27/1995 JLB :
 *Adjusts menu for multiplay mode.                                         *
 *=============================================================================================*/
void GameOptionsClass::Process() {
  static const struct {
    int ID;          // Button ID to use.
    int Text;        // Text number to use for this button.
    bool Multiplay;  // Allowed in multiplayer version?
  } _constants[] = {
      {kButtonLoad, TXT_LOAD_MISSION, false},
      {kButtonSave, TXT_SAVE_MISSION, true},
      {kButtonDelete, TXT_DELETE_MISSION, true},
      {kButtonGame, TXT_GAME_CONTROLS, true},
      {kButtonQuit, TXT_QUIT_MISSION, true},
      {kButtonDraw, TXT_OK, true},
      {kButtonResume, TXT_RESUME_MISSION, true},
      {kButtonRestate, TXT_RESTATE_MISSION, false},
  };

  /*
  **	Variables.
  */
  game::GadgetList buttons;
  // Owns every gadget linked into `buttons`.
  std::vector<std::unique_ptr<TextButtonClass>> owned_buttons;
  int selection = 0;
  int curbutton = 7;
  int y = 0;
  TextButtonClass* buttonsel[std::size(_constants)];
  static const int num_buttons = sizeof(_constants) / sizeof(_constants[0]);

  int num_players = 0;

  //
  // Compute the number of real players in the game; only allow saves
  // if there are more than 1.
  //
  for (int i = 0; i < TheSession().Players.Count(); i++) {
    if (!HouseClass::As_Pointer(TheSession().Players.at(i)->ID)->IsDefeated) {
      num_players++;
    }
  }

  PixelView& view = TheScreen().visible_view();

  /*
  **	Build the button list for all of the buttons for this dialog.
  */
  int maxwidth = 0;

  for (int index = 0; index < num_buttons; index++) {
    int text = base::At(_constants, index).Text;
    base::At(buttonsel, index) = nullptr;

    if (TheSession().Type != GAME_NORMAL &&
        !base::At(_constants, index).Multiplay) {
      continue;
    }

    if ((TheSession().Type == GAME_SKIRMISH ||
         TheSession().Type == GAME_INTERNET) &&
        text == TXT_SAVE_MISSION) {
      continue;
    }

    if (TheSession().Type != GAME_NORMAL && num_players < 2 &&
        text == TXT_SAVE_MISSION) {
      continue;
    }

    if (TheSession().Type == GAME_SKIRMISH && text == TXT_DELETE_MISSION) {
      continue;
    }

    if (TheSession().Type != GAME_NORMAL && text == TXT_DELETE_MISSION) {
      text = TXT_RESIGN;
    }

    if (index < 6) {
      y = ((TheScreen().visible_view().height() - OptionHeight) / 2) + ButtonY +
          ((OButtonHeight + 2) * index);
    } else {
      y = OptionY + ButtonResumeY;
    }

    TextButtonClass* g = nullptr;
    if (base::At(_constants, index).ID == kButtonDraw) {
      if (TheSession().Type != GAME_NORMAL &&
          TheSession().Type != GAME_SKIRMISH &&
          TheSession().Players.Count() == 2) {
        if (TheScenario().bLocalProposesDraw) {
          if (!TheScenario().bOtherProposesDraw) {
            g = owned_buttons
                    .emplace_back(std::make_unique<TextButtonClass>(
                        kButtonDraw, TXT_WOL_RETRACT_DRAW, kButtonFont,
                        kButtonFlags, 0, y))
                    .get();
          } else {
            continue;  //	Game will end now anyway.
          }
        } else {
          if (!TheScenario().bOtherProposesDraw) {
            g = owned_buttons
                    .emplace_back(std::make_unique<TextButtonClass>(
                        kButtonDraw, TXT_WOL_PROPOSE_DRAW, kButtonFont,
                        kButtonFlags, 0, y))
                    .get();
          } else {
            g = owned_buttons
                    .emplace_back(std::make_unique<TextButtonClass>(
                        kButtonDraw, TXT_WOL_ACCEPT_DRAW, kButtonFont,
                        kButtonFlags, 0, y))
                    .get();
          }
        }
      } else {
        continue;
      }
    } else {
      g = owned_buttons
              .emplace_back(std::make_unique<TextButtonClass>(
                  static_cast<unsigned>(base::At(_constants, index).ID), text,
                  kButtonFont, kButtonFlags, 0, y))
              .get();
    }

    maxwidth = std::max(g->width(), maxwidth);
    buttons.Add(*g);

    base::At(buttonsel, index) = g;
  }

  /*
  ** BG: In skirmish mode, there is no 'restate' button, so we have to
  **     backtrack through the list to find the last valid button.
  */
  while (!base::At(buttonsel, curbutton - 1)) {
    curbutton--;
  }

  base::At(buttonsel, curbutton - 1)->Turn_On();

  /*
  **	Force all button lengths to match the maximum length of the widest
  *button.
  */
  for (game::Gadget* const g : buttons.gadgets()) {
    g->set_width(std::max(maxwidth, 180));
    g->SetPosition(
        base::Point{OptionX + ((OptionWidth - g->width()) / 2), g->y()});
  }
  TextButtonClass* const resume = buttonsel[kButtonResume - 1];
  resume->set_width(180);
  resume->SetPosition(base::Point{OptionX + 34, resume->y()});

  if (TheSession().Type == GAME_NORMAL) {
    TextButtonClass* const restate = buttonsel[kButtonRestate - 1];
    restate->set_width(180);
    restate->SetPosition(base::Point{
        OptionX + OptionWidth - (restate->width() + 34), restate->y()});
  }

  /*
  **	This causes left mouse button clicking within the confines of the dialog
  *to *	be ignored if it wasn't recognized by any other button or slider.
  */
  const auto dialog_area = std::make_unique<game::Gadget>(
      base::Rect{OptionX, OptionY, OptionWidth, OptionHeight},
      game::Gadget::InputFlags::kLeftPress);
  buttons.Add(*dialog_area);

  /*
  **	This cause a right click anywhere or a left click outside the dialog
  *region *	to be equivalent to clicking on the return to game button.
  */
  const auto outside_click = std::make_unique<game::ControlClass>(
      kButtonResume, 0, 0, TheScreen().visible_view().width(),
      TheScreen().visible_view().height(),
      game::Gadget::InputFlags::kLeftPress |
          game::Gadget::InputFlags::kRightPress);
  buttons.Add(*outside_click);

  game::TheKeyboard().Clear();

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
    if (TheSession().Type == GAME_NORMAL ||
        TheSession().Type == GAME_SKIRMISH) {
      ServiceRealTime();
    } else {
      if (RunFrame()) {
        process = false;
      }
    }

    /*
    **	Refresh display if needed.
    */
    if (display || TheGameState().redraw_options_menu()) {
      /*
      **	Redraw the map.
      */
      TheScreen().hidden_view().Clear();
      TheMap().Flag_To_Redraw(true);
      TheMap().Render();

      game::TheMouse().Hide();

      /*
      **	Draw the background.
      */
      Dialog_Box(OptionX, OptionY, OptionWidth, OptionHeight);

      /*
      **	Draw the arrows border if requested.
      */
      Draw_Caption(view, TXT_OPTIONS, OptionX, OptionY, OptionWidth);

      /*
      **	Display the version number at the bottom of the dialog box.
      */
      TextPrinter(view, {Get_Color_Scheme(), game::kTransparentBlack,
                         FontType::kEditor, TPF_NOSHADOW | TPF_RIGHT})
          .Print(absl::StrFormat("%s\rV%s", TheScenario().ScenarioName,
                                 Version_Name()),
                 {OptionX + OptionWidth - 50,
                  OptionY + OptionHeight -
                      (TheSession().Type == GAME_NORMAL ? 64 : 48)});

      buttons.DrawAll(view);
      TabClass::Hilite_Tab(view, 0);
      game::TheMouse().Show();
      display = false;
      TheGameState().redraw_options_menu() = false;
    }

    /*
    **	Get user input.
    */
    const engine::window::KeyNumber input = buttons.Input(view).key;

    /*
    **	Process Input.
    */
    switch (static_cast<int>(input)) {
      case engine::window::ButtonKey(kButtonRestate):
        selection = kButtonRestate;
        pressed = true;
        break;

      case engine::window::ButtonKey(kButtonLoad):
        selection = kButtonLoad;
        pressed = true;
        break;

      case engine::window::ButtonKey(kButtonSave):
        selection = kButtonSave;
        pressed = true;
        break;

      case engine::window::ButtonKey(kButtonDelete):
        selection = kButtonDelete;
        pressed = true;
        break;

      case engine::window::ButtonKey(kButtonQuit):
        selection = kButtonQuit;
        pressed = true;
        break;

      case engine::window::ButtonKey(kButtonGame):
        selection = kButtonGame;
        pressed = true;
        break;

      case engine::window::ButtonKey(kButtonDraw):
        selection = kButtonDraw;
        pressed = true;
        break;

      case KN_ESC:
      case engine::window::ButtonKey(kButtonResume):
        selection = kButtonResume;
        pressed = true;
        break;

      case KN_UP:
        base::At(buttonsel, curbutton - 1)->Turn_Off();
        base::At(buttonsel, curbutton - 1)->FlagToRedraw();
        do {
          curbutton--;
          if (curbutton < 1) {
            curbutton = num_buttons;
          }
        } while (!base::At(buttonsel, curbutton - 1));

        base::At(buttonsel, curbutton - 1)->Turn_On();
        base::At(buttonsel, curbutton - 1)->FlagToRedraw();
        break;

      case KN_DOWN:
        base::At(buttonsel, curbutton - 1)->Turn_Off();
        base::At(buttonsel, curbutton - 1)->FlagToRedraw();
        do {
          curbutton++;
          if (curbutton > num_buttons) {
            curbutton = 1;
          }
        } while (!base::At(buttonsel, curbutton - 1));

        base::At(buttonsel, curbutton - 1)->Turn_On();
        base::At(buttonsel, curbutton - 1)->FlagToRedraw();
        break;

      case KN_RETURN:
        base::At(buttonsel, curbutton - 1)->IsPressed = true;
        base::At(buttonsel, curbutton - 1)->Draw(view, true);
        selection = curbutton;
        pressed = true;
        game::TheKeyboard().Clear();
        break;

      default:
        break;
    }

    if (pressed) {
      base::At(buttonsel, curbutton - 1)->Turn_Off();
      base::At(buttonsel, curbutton - 1)->FlagToRedraw();
      curbutton = selection;
      base::At(buttonsel, curbutton - 1)->Turn_On();
      base::At(buttonsel, curbutton - 1)->FlagToRedraw();

      switch (selection) {
        case kButtonRestate:
          display = true;
          if (Restate_Mission() == BriefingAction::kPlayVideo) {
            TheGameState().breakout_allowed() = true;
            Play_Movie(TheScenario().BriefMovie);
            TheTheme().Queue_Song(THEME_PICK_ANOTHER);
          }
          ThePalettes().black_palette().Adjust(0x08,
                                               ThePalettes().white_palette());
          ThePalettes().black_palette().Set();
          ThePalettes().black_palette().Adjust(0xFF);
          ThePalettes().black_palette().Set();
          ThePalettes().game_palette().Set();
          TheMap().Flag_To_Redraw(true);
          process = false;
          break;

        case kButtonLoad:
          display = true;
          if (LoadOptionsClass(LoadOptionsClass::LOAD).Process()) {
            process = false;
          }
          break;

        case kButtonSave:
          display = true;
          if (TheSession().Type == GAME_NORMAL) {
            LoadOptionsClass(LoadOptionsClass::SAVE).Process();

          } else {
            TheNetwork().out_list().Add(EventClass(EventClass::SAVEGAME));
            process = false;
          }
          break;

        case kButtonDelete:
          display = true;
          if (TheSession().Type != GAME_NORMAL) {
            if (Surrender_Dialog(TXT_SURRENDER)) {
              TheNetwork().out_list().Add(EventClass(EventClass::DESTRUCT));
            }
            process = false;
          } else {
            LoadOptionsClass(LoadOptionsClass::WWDELETE).Process();
          }
          break;

        case kButtonQuit:
          if (TheSession().Type == GAME_NORMAL) {
            switch (WWMessageBox().Process(TXT_CONFIRM_EXIT, TXT_ABORT,
                                           TXT_CANCEL, TXT_RESTART)) {
              case 1:
                display = true;
                break;

              case 0:
                process = false;
                Queue_Exit();
                break;

              case 2:
                TheGameState().player_restarts() = true;
                process = false;
                break;
              default:
                break;
            }
          } else {
            if (Surrender_Dialog(TXT_CONFIRM_EXIT)) {
              process = false;
              Queue_Exit();
            } else {
              display = true;
            }
            // if (WWMessageBox().Process(TXT_CONFIRM_EXIT, TXT_YES, TXT_NO) ==
            // 0) { process = false; Queue_Exit();
            //} else {
            // display = true;
            //}
          }
          break;

        case kButtonDraw:
          if (TheScenario().bLocalProposesDraw) {
            //	Retract draw offer.
            TheNetwork().out_list().Add(EventClass(EventClass::RETRACT_DRAW));
            process = false;
          } else {
            if (!TheScenario().bOtherProposesDraw) {
              //	Propose a draw?
              if (Surrender_Dialog(TXT_WOL_PROPOSE_DRAW_CONFIRM)) {
                TheNetwork().out_list().Add(
                    EventClass(EventClass::PROPOSE_DRAW));
                process = false;
              } else {
                display = true;
              }
            } else {
              //	Accept a draw?
              if (Surrender_Dialog(TXT_WOL_ACCEPT_DRAW_CONFIRM)) {
                TheNetwork().out_list().Add(
                    EventClass(EventClass::PROPOSE_DRAW));
                process = false;
              } else {
                display = true;
              }
            }
          }
          break;

        case kButtonGame:
          display = true;
          GameControlsClass::Process();
          break;

        case kButtonResume:
          Save_Settings();
          process = false;
          display = true;
          break;
        default:
          break;
      }

      pressed = false;
      base::At(buttonsel, curbutton - 1)->IsPressed = false;
      base::At(buttonsel, curbutton - 1)->Turn_Off();
      base::At(buttonsel, curbutton - 1)->FlagToRedraw();
    }
  }

  /*
  **	Clean up and re-enter the game.
  */
  for (const auto& button : owned_buttons) {
    button->ClearFocus();
  }
  owned_buttons.clear();

  /*
  **	Redraw the map.
  */
  game::TheKeyboard().Clear();
  TheScreen().hidden_view().Clear();
  TheMap().Flag_To_Redraw(true);
  TheMap().Render();
}

void GameOptionsClass::Adjust_Variables_For_Resolution() {
  OptionWidth = (216 + 8) * 2;
  OptionHeight = 222;
  OptionX = (TheScreen().visible_view().width() - OptionWidth) / 2;
  OptionY = (TheScreen().visible_view().height() - OptionHeight) / 2;
  ButtonWidth = 260;
  OButtonHeight = 18;
  CaptionYPos = 10;
  ButtonY = 42;
  Border1Len = 144;
  Border2Len = 32;
  ButtonResumeY = OptionHeight - 38;
}

}  // namespace ra
