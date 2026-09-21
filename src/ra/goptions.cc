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

#include "ra/goptions.h"

#include <algorithm>
#include <iterator>

#include "base/array.h"
#include "ra/conquer.h"
#include "ra/control.h"
#include "ra/defines.h"
#include "ra/dialog.h"
#include "ra/event.h"
#include "ra/gadget.h"
#include "ra/game_state.h"
#include "ra/gamedlg.h"
#include "ra/house.h"
#include "ra/input.h"
#include "ra/jshell.h"
#include "ra/loaddlg.h"
#include "ra/mapedit.h"
#include "ra/movie.h"
#include "ra/mplayer.h"
#include "ra/msgbox.h"
#include "ra/network.h"
#include "ra/palette.h"
#include "ra/palettes.h"
#include "ra/queue.h"
#include "ra/scenario.h"
#include "ra/screen.h"
#include "ra/session.h"
#include "ra/tab.h"
#include "ra/text_ids.h"
#include "ra/textbtn.h"
#include "ra/theme.h"
#include "ra/vector_dynamic.h"
#include "ra/version.h"
#include "ra/wolstrng.h"
#include "ra/world.h"
#include "sdllib/keyboard.h"
#include "sdllib/misc.h"
#include "sdllib/pixel_buffer.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/wwstd.h"

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
  TextButtonClass* buttons = nullptr;
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
    if (!HouseClass::As_Pointer(TheSession().Players.at(i)->Player.ID)
             ->IsDefeated) {
      num_players++;
    }
  }

  SetLogicPage(TheScreen().visible_view());

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
            g = new TextButtonClass(kButtonDraw, TXT_WOL_RETRACT_DRAW,
                                    kTpfButton, 0, y);
          } else {
            continue;  //	Game will end now anyway.
          }
        } else {
          if (!TheScenario().bOtherProposesDraw) {
            g = new TextButtonClass(kButtonDraw, TXT_WOL_PROPOSE_DRAW,
                                    kTpfButton, 0, y);
          } else {
            g = new TextButtonClass(kButtonDraw, TXT_WOL_ACCEPT_DRAW,
                                    kTpfButton, 0, y);
          }
        }
      } else {
        continue;
      }
    } else {
      g = new TextButtonClass(
          static_cast<unsigned>(base::At(_constants, index).ID), text,
          kTpfButton, 0, y);
    }

    maxwidth = std::max(g->Width, maxwidth);
    if (buttons == nullptr) {
      buttons = g;
    } else {
      g->Add_Tail(*buttons);
    }

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
  GadgetClass* g = buttons;
  while (g != nullptr) {
    g->Width = std::max(maxwidth, 180);
    g->X = OptionX + ((OptionWidth - g->Width) / 2);
    g = g->Get_Next();
  }
  buttonsel[kButtonResume - 1]->Width = 180;
  buttonsel[kButtonResume - 1]->X = OptionX + 34;

  if (TheSession().Type == GAME_NORMAL) {
    buttonsel[kButtonRestate - 1]->Width = 180;
    buttonsel[kButtonRestate - 1]->X =
        OptionX + OptionWidth - (buttonsel[kButtonRestate - 1]->Width + 34);
  }

  /*
  **	This causes left mouse button clicking within the confines of the dialog
  *to *	be ignored if it wasn't recognized by any other button or slider.
  */
  (new GadgetClass(OptionX, OptionY, OptionWidth, OptionHeight,
                   GadgetClass::kLeftPress))
      ->Add_Tail(*buttons);

  /*
  **	This cause a right click anywhere or a left click outside the dialog
  *region *	to be equivalent to clicking on the return to game button.
  */
  (new ControlClass(kButtonResume, 0, 0, TheScreen().visible_view().width(),
                    TheScreen().visible_view().height(),
                    GadgetClass::kLeftPress | GadgetClass::kRightPress))
      ->Add_Tail(*buttons);

  TheKeyboard().Clear();

  Fancy_Text_Print(TXT_NONE, 0, 0, GadgetClass::Get_Color_Scheme(), kTBlack,
                   TPF_CENTER | kTpfText);

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
    if (display || TheGameState().redraw_options_menu()) {
      /*
      **	Redraw the map.
      */
      TheScreen().hidden_view().Clear();
      TheMap().Flag_To_Redraw(true);
      TheMap().Render();

      /*
      **	Reset up the window.  Window x-coords are in bytes not pixels.
      */
      Set_Window(static_cast<int>(WINDOW_EDITOR), OptionX, OptionY, OptionWidth,
                 OptionHeight);
      Hide_Mouse();

      /*
      **	Draw the background.
      */
      Dialog_Box(OptionX, OptionY, OptionWidth, OptionHeight);

      /*
      **	Draw the arrows border if requested.
      */
      Draw_Caption(TXT_OPTIONS, OptionX, OptionY, OptionWidth);

      /*
      **	Display the version number at the bottom of the dialog box.
      */
      Fancy_Text_Print(
          "%s\rV%s", OptionX + OptionWidth - 50,
          OptionY + OptionHeight - (TheSession().Type == GAME_NORMAL ? 64 : 48),
          GadgetClass::Get_Color_Scheme(), kTBlack,
          TPF_EFNT | TPF_NOSHADOW | TPF_RIGHT, TheScenario().ScenarioName,
          Version_Name());

      buttons->Draw_All();
      TabClass::Hilite_Tab(0);
      Show_Mouse();
      display = false;
      TheGameState().redraw_options_menu() = false;
    }

    /*
    **	Get user input.
    */
    const KeyNumType input = buttons->Input();

    /*
    **	Process Input.
    */
    switch (static_cast<int>(input)) {
      case ButtonKey(kButtonRestate):
        selection = kButtonRestate;
        pressed = true;
        break;

      case ButtonKey(kButtonLoad):
        selection = kButtonLoad;
        pressed = true;
        break;

      case ButtonKey(kButtonSave):
        selection = kButtonSave;
        pressed = true;
        break;

      case ButtonKey(kButtonDelete):
        selection = kButtonDelete;
        pressed = true;
        break;

      case ButtonKey(kButtonQuit):
        selection = kButtonQuit;
        pressed = true;
        break;

      case ButtonKey(kButtonGame):
        selection = kButtonGame;
        pressed = true;
        break;

      case ButtonKey(kButtonDraw):
        selection = kButtonDraw;
        pressed = true;
        break;

      case KN_ESC:
      case ButtonKey(kButtonResume):
        selection = kButtonResume;
        pressed = true;
        break;

      case KN_UP:
        base::At(buttonsel, curbutton - 1)->Turn_Off();
        base::At(buttonsel, curbutton - 1)->Flag_To_Redraw();
        do {
          curbutton--;
          if (curbutton < 1) {
            curbutton = num_buttons;
          }
        } while (!base::At(buttonsel, curbutton - 1));

        base::At(buttonsel, curbutton - 1)->Turn_On();
        base::At(buttonsel, curbutton - 1)->Flag_To_Redraw();
        break;

      case KN_DOWN:
        base::At(buttonsel, curbutton - 1)->Turn_Off();
        base::At(buttonsel, curbutton - 1)->Flag_To_Redraw();
        do {
          curbutton++;
          if (curbutton > num_buttons) {
            curbutton = 1;
          }
        } while (!base::At(buttonsel, curbutton - 1));

        base::At(buttonsel, curbutton - 1)->Turn_On();
        base::At(buttonsel, curbutton - 1)->Flag_To_Redraw();
        break;

      case KN_RETURN:
        base::At(buttonsel, curbutton - 1)->IsPressed = true;
        base::At(buttonsel, curbutton - 1)->Draw_Me(true);
        selection = curbutton;
        pressed = true;
        TheKeyboard().Clear();
        break;

      default:
        break;
    }

    if (pressed) {
      base::At(buttonsel, curbutton - 1)->Turn_Off();
      base::At(buttonsel, curbutton - 1)->Flag_To_Redraw();
      curbutton = selection;
      base::At(buttonsel, curbutton - 1)->Turn_On();
      base::At(buttonsel, curbutton - 1)->Flag_To_Redraw();

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
      base::At(buttonsel, curbutton - 1)->Flag_To_Redraw();
    }
  }

  /*
  **	Clean up and re-enter the game.
  */
  buttons->Delete_List();

  /*
  **	Redraw the map.
  */
  TheKeyboard().Clear();
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
