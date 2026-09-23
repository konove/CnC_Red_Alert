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

/* $Header:   F:\projects\c&c\vcs\code\goptions.cpv   2.17   16 Oct 1995
 * 16:50:26   JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
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
 *interface.                       * Draw_Caption -- Draws a caption on a dialog
 *box.                                          *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "td/goptions.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <iterator>

#include "absl/strings/str_format.h"
#include "base/array.h"
#include "sdllib/font.h"
#include "sdllib/keyboard.h"
#include "sdllib/misc.h"
#include "sdllib/pixel_buffer.h"
#include "sdllib/shape.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/ww_win.h"
#include "sdllib/wwstd.h"
#include "td/confdlg.h"
#include "td/conquer.h"
#include "td/control.h"
#include "td/defines.h"
#include "td/dialog.h"
#include "td/event.h"
#include "td/gadget.h"
#include "td/game_state.h"
#include "td/gamedlg.h"
#include "td/init.h"
#include "td/jshell.h"
#include "td/loaddlg.h"
#include "td/mapedit.h"
#include "td/mplayer.h"
#include "td/msgbox.h"
#include "td/network.h"
#include "td/palette.h"
#include "td/palettes.h"
#include "td/queue.h"
#include "td/scenario.h"
#include "td/screen.h"
#include "td/session.h"
#include "td/tab.h"
#include "td/text.h"
#include "td/textbtn.h"
#include "td/theme.h"
#include "td/world.h"
#include "tech/game_file.h"
#include "tech/mix_archive.h"

void GameOptionsClass::Adjust_Variables_For_Resolution() {
  const int factor = TheScreen().visible_view().width() == 320 ? 1 : 2;

  OptionWidth = (216 + 8) * factor;
  OptionHeight = 100 * factor;
  OptionX = (TheScreen().visible_view().width() - OptionWidth) / 2;
  OptionY = (TheScreen().visible_view().height() - OptionHeight) / 2;
  ButtonWidth = 130 * factor;
  OButtonHeight = 9 * factor;
  CaptionYPos = 5 * factor;
  ButtonY = 21 * factor;
  Border1Len = 72 * factor;
  Border2Len = 16 * factor;
  ButtonResumeY = OptionHeight - (15 * factor);
}
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
// Not const: runs the dialog, which can load, save or quit the game.
// NOLINTNEXTLINE(readability-make-member-function-const)
void GameOptionsClass::Process() {
  static const struct {
    int ID;          // Button ID to use.
    int Text;        // Text number to use for this button.
    bool Multiplay;  // Allowed in multiplayer version?
  } _constants[] = {
      {kButtonLoad, TXT_LOAD_MISSION, false},
      {kButtonSave, TXT_SAVE_MISSION, false},
      {kButtonDelete, TXT_DELETE_MISSION, true},
      {kButtonGame, TXT_GAME_CONTROLS, true},
      {kButtonQuit, TXT_QUIT_MISSION, true},
      {kButtonResume, TXT_RESUME_MISSION, true},
      {kButtonRestate, TXT_RESTATE_MISSION, false},
  };

  /*
  **	Variables.
  */
  TextButtonClass* buttons = nullptr;
  int selection = 0;
  bool pressed = false;
  int curbutton = 6;
  int y = 0;
  TextButtonClass*
      buttonsel[sizeof(_constants) / sizeof(base::At(_constants, 0))];

  PixelView& view = TheScreen().visible_view();

  /*
  **	Build the button list for all of the buttons for this dialog.
  */
  int maxwidth = 0;
  const int resfactor = TheScreen().visible_view().width() == 320 ? 1 : 2;

  for (int index = 0; index < std::ssize(_constants);
       index++) {
    int text = base::At(_constants, index).Text;
    base::At(buttonsel, index) = nullptr;

    if (TheSession().type() != GAME_NORMAL &&
        !base::At(_constants, index).Multiplay) {
      base::At(buttonsel, index) = nullptr;
      continue;
    }

    if (TheSession().type() != GAME_NORMAL && text == TXT_DELETE_MISSION) {
      text = TXT_RESIGN;
    }

    if (index < 5) {
      y = ((TheScreen().visible_view().height() - OptionHeight) / 2) + ButtonY +
          ((OButtonHeight + 2) * index);
    } else {
      y = OptionY + ButtonResumeY;
    }

    auto* g = new TextButtonClass(
        static_cast<unsigned>(base::At(_constants, index).ID), text,
        TPF_6PT_GRAD | TPF_NOSHADOW, 0, y);

    maxwidth = std::max(g->Width, maxwidth);
    if (!buttons) {
      buttons = g;
    } else {
      g->Add_Tail(*buttons);
    }

    base::At(buttonsel, index) = g;
  }

  base::At(buttonsel, curbutton - 1)->Turn_On();

  /*
  **	Force all button lengths to match the maximum length of the widest
  *button.
  */
  GadgetClass* g = buttons;
  while (g) {
    g->Width = std::max(maxwidth, 90 * resfactor);
    g->X = OptionX + ((OptionWidth - g->Width) / 2);
    g = g->Get_Next();
  }
#ifdef FRENCH
  buttonsel[kButtonResume - 1]->Width = 104 * resfactor;
#else
  base::At(buttonsel, kButtonResume - 1)->Width = 90 * resfactor;
#endif
  base::At(buttonsel, kButtonResume - 1)->X = OptionX + (5 * resfactor);

  if (TheSession().type() == GAME_NORMAL) {
    base::At(buttonsel, kButtonRestate - 1)->Width = 90 * resfactor;
    base::At(buttonsel, kButtonRestate - 1)->X =
        OptionX + OptionWidth -
        (base::At(buttonsel, kButtonRestate - 1)->Width + (5 * resfactor));
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

  Keyboard::Clear();

  Fancy_Text_Print(view, TXT_NONE, 0, 0, kCcGreen, kTBlack,
                   TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

  /*
  **	Main Processing Loop.
  */
  bool display = true;
  bool process = true;
  pressed = false;
  while (process) {
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
    **	Invoke game callback.
    */
    if (TheSession().type() == GAME_NORMAL) {
      Call_Back();
    } else {
      if (Main_Loop()) {
        process = false;
      }
    }

    /*
    **	Refresh display if needed.
    */
    if (display) {
      /*
      **	Redraw the map.
      */
      TheScreen().hidden_page().view().Clear();
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
      Window_Box(view, WINDOW_EDITOR,
                 BOXSTYLE_GREEN_BORDER);  // has border, raised up

      /*
      **	Draw the arrows border if requested.
      */
      Draw_Caption(view, TXT_OPTIONS, OptionX, OptionY, OptionWidth);

      /*
      **	Display the version number at the bottom of the dialog box.
      */
#ifdef DEMO
      Version_Number();
      Fancy_Text_Print(
          view, "DEMO%s",
          ((WindowList[static_cast<int>(WINDOW_EDITOR)][kWindowX] +
            WindowList[static_cast<int>(WINDOW_EDITOR)][kWindowWidth])
           << 3) -
              3 * resfactor,
          WindowList[static_cast<int>(WINDOW_EDITOR)][kWindowY] +
              WindowList[static_cast<int>(WINDOW_EDITOR)][kWindowHeight] -
              ((TheSession().type() == GAME_NORMAL) ? (32 * resfactor)
                                                    : (24 * resfactor)),
          kGrey, kTBlack, TPF_6POINT | TPF_NOSHADOW | TPF_RIGHT, ScenarioName,
          TheGameState().version_text());
#else
      Fancy_Text_Print(
          view, "%s\rV.%d%s",
          ((base::At(base::At(WindowList, static_cast<int>(WINDOW_EDITOR)),
                     kWindowX) +
            base::At(base::At(WindowList, static_cast<int>(WINDOW_EDITOR)),
                     kWindowWidth)) *
           8) -
              (3 * resfactor),
          base::At(base::At(WindowList, static_cast<int>(WINDOW_EDITOR)),
                   kWindowY) +
              base::At(base::At(WindowList, static_cast<int>(WINDOW_EDITOR)),
                       kWindowHeight) -
              (TheSession().type() == GAME_NORMAL ? 32 * resfactor
                                                  : 24 * resfactor),
          kGrey, kTBlack, TPF_6POINT | TPF_NOSHADOW | TPF_RIGHT,
          TheWorld().scenario_name(), Version_Number(),
          TheGameState().version_text());
#endif

      buttons->Draw_All(view);
      TabClass::Hilite_Tab(view, 0);
      Show_Mouse();
      display = false;
    }

    /*
    **	Get user input.
    */
    const KeyNumType input = buttons->Input(view);

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

      case KN_ESC:
      case ButtonKey(kButtonResume):
        selection = kButtonResume;
        pressed = true;
        break;

      case KN_UP:
        base::At(buttonsel, curbutton - 1)->Turn_Off();
        base::At(buttonsel, curbutton - 1)->Flag_To_Redraw();
        curbutton--;
        if (TheSession().type() == GAME_NORMAL) {
          if (curbutton < kButtonLoad) {
            curbutton = kButtonCount - 1;
          }
        } else {
          if (curbutton < kButtonDelete) {
            curbutton = kButtonResume;
            //						curbutton =
            //(kButtonCount-1);
          }
        }
        base::At(buttonsel, curbutton - 1)->Turn_On();
        base::At(buttonsel, curbutton - 1)->Flag_To_Redraw();
        break;

      case KN_DOWN:
        base::At(buttonsel, curbutton - 1)->Turn_Off();
        base::At(buttonsel, curbutton - 1)->Flag_To_Redraw();
        curbutton++;
        if (TheSession().type() == GAME_NORMAL) {
          if (curbutton >= kButtonCount) {
            curbutton = kButtonLoad;
          }
        } else {
          if (curbutton > kButtonResume) {
            curbutton = kButtonDelete;
          }
        }
        base::At(buttonsel, curbutton - 1)->Turn_On();
        base::At(buttonsel, curbutton - 1)->Flag_To_Redraw();
        break;

      case KN_RETURN:
        base::At(buttonsel, curbutton - 1)->IsPressed = true;
        base::At(buttonsel, curbutton - 1)->Draw_Me(view, true);
        selection = curbutton;
        pressed = true;
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
#ifdef JAPANESE
          if (!Restate_Mission(ScenarioName, TXT_VIDEO,
                               TXT_TAB_BUTTON_CONTROLS)) {
#else
          if (!Restate_Mission(TheWorld().scenario_name(), TXT_VIDEO,
                               TXT_OPTIONS)) {
#endif
            TheGameState().breakout_allowed() = true;
            char buffer[25];
            absl::SNPrintF(buffer, sizeof(buffer), "%s.VQA",
                           TheWorld().brief_movie());
            if (GameFileExists(buffer)) {
              Play_Movie(TheWorld().brief_movie());
            } else {
              Play_Movie(TheWorld().action_movie());
            }
            // BreakoutAllowed = false;
            std::ranges::fill(ThePalettes().black_palette(), 0x01);
            Set_Palette(ThePalettes().black_palette());
            std::ranges::fill(ThePalettes().black_palette(), 0x00);
            Set_Palette(ThePalettes().black_palette());
            TheMap().Flag_To_Redraw(true);
            TheTheme().Queue_Song(THEME_PICK_ANOTHER);
            process = false;
          }
          break;

        case kButtonLoad:
          display = true;
          if (LoadOptionsClass(LoadOptionsClass::LOAD).Process()) {
            process = false;
          }
          break;

        case kButtonSave:
          display = true;
          LoadOptionsClass(LoadOptionsClass::SAVE).Process();
          break;

        case kButtonDelete:
          display = true;
          if (TheSession().type() != GAME_NORMAL) {
            if (Surrender_Dialog()) {
              TheNetwork().out_list().Add(EventClass(EventClass::DESTRUCT));
            }
            process = false;
          } else {
            LoadOptionsClass(LoadOptionsClass::WWDELETE).Process();
          }
          break;

        case kButtonQuit:
          if (TheSession().type() == GAME_NORMAL) {
#ifdef JAPANESE
            switch (CCMessageBox().Process(TXT_CONFIRM_EXIT, TXT_YES, TXT_NO,
                                           TXT_RESTART)) {
#else
            switch (CCMessageBox().Process(TXT_CONFIRM_EXIT, TXT_ABORT,
                                           TXT_CANCEL, TXT_RESTART)) {
#endif
              case 2:
                display = true;
                break;

              case 0:
                process = false;
                Queue_Exit();
                break;

              case 1:
                TheGameState().player_restarts() = true;
                process = false;
                break;
              default:
                break;
            }
          } else {
            if (ConfirmationClass::Process(TXT_CONFIRM_EXIT)) {
              process = false;
              Queue_Exit();
            } else {
              display = true;
            }
          }
          break;

        case kButtonGame:
          display = true;
          GameControlsClass::Process();
          break;

        case kButtonResume:
          // Save_Settings();
          process = false;
          display = true;
          break;

        default:
          break;
      }

      pressed = false;
      base::At(buttonsel, curbutton - 1)->IsPressed = false;
      // buttonsel[curbutton-1]->Turn_Off();
      base::At(buttonsel, curbutton - 1)->Turn_On();
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
  Keyboard::Clear();
  Call_Back();
  TheScreen().hidden_page().view().Clear();
  Call_Back();
  TheMap().Flag_To_Redraw(true);
  TheMap().Render();
}

/***********************************************************************************************
 * Draw_Caption -- Draws a caption on a dialog box. *
 *                                                                                             *
 *    This routine draws the caption text and any fancy filigree that the dialog
 *may require.  *
 *                                                                                             *
 * INPUT:   text  -- The text of the caption. This is the text number. *
 *                                                                                             *
 *          x,y   -- The dialog box X and Y pixel coordinate of the upper left
 *corner.         *
 *                                                                                             *
 *          w     -- The width of the dialog box (in pixels). *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/23/1995 JLB : Created. *
 *=============================================================================================*/
void Draw_Caption(PixelView& view, int text, int x, int y, int w) {
  OptionControlType option = OPTION_NONE;
  const int factor = TheScreen().visible_view().width() == 320 ? 1 : 2;

  /*
  **	Determine the filigree to use depending on the text of the caption.
  */
  switch (text) {
    case TXT_GAME_CONTROLS:
    case TXT_OPTIONS:
      option = OPTION_CONTROLS;
      break;

    case TXT_LOAD_MISSION:
    case TXT_SAVE_MISSION:
    case TXT_DELETE_MISSION:
      option = OPTION_DELETE;
      break;

    case TXT_NONE:
    case TXT_MODEM_SERIAL:
    case TXT_SELECT_MPLAYER_GAME:
    case TXT_SELECT_SERIAL_GAME:
      option = OPTION_DIALOG;
      break;

    case TXT_HOST_SERIAL_GAME:
    case TXT_JOIN_SERIAL_GAME:
      option = OPTION_SERIAL;
      break;

    case TXT_SETTINGS:
    case TXT_PHONE_LIST:
    case TXT_PHONE_LISTING:
      option = OPTION_PHONE;
      break;

    case TXT_JOIN_NETWORK_GAME:
      option = OPTION_JOIN_NETWORK;
      break;

    case TXT_NETGAME_SETUP:
      option = OPTION_NETWORK;
      break;

    case TXT_VISUAL_CONTROLS:
      option = OPTION_VISUAL;
      break;

    case TXT_SOUND_CONTROLS:
      option = OPTION_SOUND;
      break;

    default:
      option = OPTION_DIALOG;
      break;
  }

  /*
  **	Draw the filigree at the corners of the dialog.
  */
  if (option != OPTION_NONE) {
    CC_Draw_Shape(view, MixArchive::RetrieveData("OPTIONS.SHP"),
                  static_cast<int>(option), x + 12, y + 11, WINDOW_MAIN,
                  SHAPE_CENTER);
    CC_Draw_Shape(view, MixArchive::RetrieveData("OPTIONS.SHP"),
                  static_cast<int>(option) + 1, x + w - 14, y + 11, WINDOW_MAIN,
                  SHAPE_CENTER);
  }

  /*
  **	Draw the caption.
  */
  if (text != TXT_NONE) {
    Fancy_Text_Print(
        view, text, (w / 2) + x, (5 * factor) + y, kCcGreen, kTBlack,
        TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

    const int length = StringPixelWidth(Text_String(text));
    view.DrawLine(x + (w / 2) - (length / 2),
                  y + g_font_max_height + g_font_y_spacing + (5 * factor),
                  x + (w / 2) + (length / 2),
                  y + g_font_max_height + g_font_y_spacing + (5 * factor),
                  kCcGreen);
  }
}
