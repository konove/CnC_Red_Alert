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

/* $Header:   F:\projects\c&c\vcs\code\visudlg.cpv   2.17   16 Oct 1995 16:51:40
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : VISUDLG.CPP *
 *                                                                                             *
 *                   Programmer : Maria del Mar McCready Legg * Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : Jan 8, 1995 *
 *                                                                                             *
 *                  Last Update : June 18, 1995 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * VisualControlsClass::Process -- Process the visual control
 *dialog box.                    *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "td/visudlg.h"

#include "sdllib/include/gbuffer.h"
#include "sdllib/include/keyboard.h"
#include "sdllib/include/misc.h"
#include "sdllib/include/ww_mouse.h"
#include "sdllib/include/wwstd.h"
#include "td/conquer.h"
#include "td/control.h"
#include "td/defines.h"
#include "td/dialog.h"
#include "td/externs.h"
#include "td/gadget.h"
#include "td/goptions.h"
#include "td/jshell.h"
#include "td/slider.h"
#include "td/textbtn.h"

/***********************************************************************************************
 * VisualControlsClass::Process -- Process the visual control dialog box. *
 *                                                                                             *
 *    This routine displays and processes the visual controls dialog box. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/18/1995 JLB : Created. *
 *=============================================================================================*/
void VisualControlsClass::Process(void) {
  static int _titles[4] = {TXT_BRIGHTNESS, TXT_COLOR, TXT_CONTRAST, TXT_TINT};

  enum {
    NUM_OF_BUTTONS = 6,
  };

  // Make them resolution independent
  int option_width = kOptionWidth * RESFACTOR;    // Width of dialog box.
  int option_height = kOptionHeight * RESFACTOR;  // Height of dialog box.
  int option_x = kOptionX * RESFACTOR;
  int option_y = kOptionY * RESFACTOR;
  int text_y = kTextY * RESFACTOR;
  int slider_x = kSliderX * RESFACTOR;
  int slider_y = kSliderY * RESFACTOR;
  int slider_width = kSliderWidth * RESFACTOR;  // Width of each control slider.
  int slider_height =
      kSliderHeight * RESFACTOR;  // Height of each control slider.
  int slider_y_spacing =
      kSliderYSpacing * RESFACTOR;      // Vertical spacing between sliders.
  int button_y = kButtonY * RESFACTOR;  // Options button y pos

  /*
  **	Variables.
  */
  int selection;
  bool pressed;
  int curbutton;
  TextButtonClass *buttons[NUM_OF_BUTTONS];
  SliderClass *buttonsliders[NUM_OF_BUTTONS];

  Set_Logic_Page(SeenBuff);

  /*
  **	Create Buttons.  Button coords are in pixels, but are window-relative.
  */
  TextButtonClass optionsbtn(BUTTON_OPTIONS, TXT_GAME_CONTROLS,
                             TPF_6PT_GRAD | TPF_NOSHADOW, 0, button_y);

  TextButtonClass resetbtn(BUTTON_RESET, TXT_RESET_MENU,
                           TPF_6PT_GRAD | TPF_NOSHADOW, 0, button_y);

  /*
  **	Centers options button.
  */
  optionsbtn.X = option_x + (option_width - optionsbtn.Width - (15 * RESFACTOR));
  resetbtn.X = option_x + (15 * RESFACTOR);

  resetbtn.Add_Tail(optionsbtn);

  /*
  **	Brightness (value) control.
  */
  SliderClass brightness(BUTTON_BRIGHTNESS, slider_x,
                         slider_y + (slider_y_spacing * 0), slider_width,
                         slider_height);
  brightness.Set_Thumb_Size(20 * RESFACTOR);
  brightness.Set_Value(Options.Get_Brightness());
  brightness.Add_Tail(optionsbtn);

  /*
  **	Color (saturation) control.
  */
  SliderClass color(BUTTON_COLOR, slider_x, slider_y + (slider_y_spacing * 1),
                    slider_width, slider_height);
  color.Set_Thumb_Size(20 * RESFACTOR);
  color.Set_Value(Options.Get_Color());
  color.Add_Tail(optionsbtn);

  /*
  **	Contrast control.
  */
  SliderClass contrast(BUTTON_CONTRAST, slider_x,
                       slider_y + (slider_y_spacing * 2), slider_width,
                       slider_height);
  contrast.Set_Thumb_Size(20 * RESFACTOR);
  contrast.Set_Value(Options.Get_Contrast());
  contrast.Add_Tail(optionsbtn);

  /*
  **	Tint (hue) control.
  */
  SliderClass tint(BUTTON_TINT, slider_x, slider_y + (slider_y_spacing * 3),
                   slider_width, slider_height);
  tint.Set_Thumb_Size(20 * RESFACTOR);
  tint.Set_Value(Options.Get_Tint());
  tint.Add_Tail(optionsbtn);

  /*
  **	This causes left mouse button clicking within the confines of the dialog
  *to *	be ignored if it wasn't recognized by any other button or slider.
  */
  GadgetClass dialog(option_x, option_y, option_width, option_height,
                     GadgetClass::LEFTPRESS);
  dialog.Add_Tail(optionsbtn);

  /*
  **	This causes a right click anywhere or a left click outside the dialog
  *region *	to be equivalent to clicking on the return to options dialog.
  */
  ControlClass background(BUTTON_OPTIONS, 0, 0, SeenBuff.Get_Width(),
                          SeenBuff.Get_Height(),
                          GadgetClass::LEFTPRESS | GadgetClass::RIGHTPRESS);
  background.Add_Tail(optionsbtn);

  curbutton = 0;
  buttons[0] = nullptr;
  buttons[1] = nullptr;
  buttons[2] = nullptr;
  buttons[3] = nullptr;
  buttons[4] = &resetbtn;
  buttons[5] = &optionsbtn;

  buttonsliders[0] = &brightness;
  buttonsliders[1] = &color;
  buttonsliders[2] = &contrast;
  buttonsliders[3] = &tint;
  buttonsliders[4] = nullptr;
  buttonsliders[5] = nullptr;

  /*
  **	Main Processing Loop.
  */
  bool display = true;
  bool process = true;
  bool partial = true;
  pressed = false;
  while (process) {
    /*
    **	Invoke game callback.
    */
    if (GameToPlay == GAME_NORMAL) {
      Call_Back();
    } else {
      if (Main_Loop()) {
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
    if (display) {
      Hide_Mouse();
      Dialog_Box(option_x, option_y, option_width, option_height);
      Draw_Caption(TXT_VISUAL_CONTROLS, option_x, option_y, option_width);
      Show_Mouse();
      display = false;
      partial = true;
    }

    /*
    **	If just the buttons and captions need to be redrawn, then do so now.
    */
    if (partial) {
      Hide_Mouse();

      /*
      **	Draw the titles.
      */
      for (int i = 0; i < (sizeof(_titles) / sizeof(_titles[0])); i++) {
        Fancy_Text_Print(
            _titles[i], slider_x - (8 * RESFACTOR),
            text_y + (i * slider_y_spacing), CC_GREEN, TBLACK,
            TPF_6PT_GRAD | TPF_RIGHT | TPF_NOSHADOW |
                ((curbutton == i) ? TPF_BRIGHT_COLOR : TPF_USE_GRAD_PAL));
      }
      optionsbtn.Draw_All();
      Show_Mouse();
      partial = false;
    }

    /*
    **	Get and process player input.
    */
    KeyNumType input = optionsbtn.Input();
    switch (input) {
      case (BUTTON_BRIGHTNESS | KN_BUTTON):
        Options.Set_Brightness(brightness.Get_Value());
        break;

      case (BUTTON_COLOR | KN_BUTTON):
        Options.Set_Color(color.Get_Value());
        break;

      case (BUTTON_CONTRAST | KN_BUTTON):
        Options.Set_Contrast(contrast.Get_Value());
        break;

      case (BUTTON_TINT | KN_BUTTON):
        Options.Set_Tint(tint.Get_Value());
        break;

      case (BUTTON_RESET | KN_BUTTON):
        selection = BUTTON_RESET;
        pressed = true;
        break;

      case KN_ESC:
      case BUTTON_OPTIONS | KN_BUTTON:
        selection = BUTTON_OPTIONS;
        pressed = true;
        break;

      case (KN_LEFT):
        if (curbutton <= (BUTTON_TINT - BUTTON_BASE)) {
          buttonsliders[curbutton]->Bump(1);
          switch (curbutton) {
            case (BUTTON_BRIGHTNESS - BUTTON_BASE):
              Options.Set_Brightness(brightness.Get_Value());
              break;

            case (BUTTON_COLOR - BUTTON_BASE):
              Options.Set_Color(color.Get_Value());
              break;

            case (BUTTON_CONTRAST - BUTTON_BASE):
              Options.Set_Contrast(contrast.Get_Value());
              break;

            case (BUTTON_TINT - BUTTON_BASE):
              Options.Set_Tint(tint.Get_Value());
              break;
          }
        } else {
          buttons[curbutton]->Turn_Off();
          buttons[curbutton]->Flag_To_Redraw();

          curbutton--;
          if (curbutton < (BUTTON_RESET - BUTTON_BASE)) {
            curbutton = (BUTTON_OPTIONS - BUTTON_BASE);
          }

          buttons[curbutton]->Turn_On();
          buttons[curbutton]->Flag_To_Redraw();
        }
        break;

      case (KN_RIGHT):
        if (curbutton <= (BUTTON_TINT - BUTTON_BASE)) {
          buttonsliders[curbutton]->Bump(0);
          switch (curbutton) {
            case (BUTTON_BRIGHTNESS - BUTTON_BASE):
              Options.Set_Brightness(brightness.Get_Value());
              break;

            case (BUTTON_COLOR - BUTTON_BASE):
              Options.Set_Color(color.Get_Value());
              break;

            case (BUTTON_CONTRAST - BUTTON_BASE):
              Options.Set_Contrast(contrast.Get_Value());
              break;

            case (BUTTON_TINT - BUTTON_BASE):
              Options.Set_Tint(tint.Get_Value());
              break;
          }
        } else {
          buttons[curbutton]->Turn_Off();
          buttons[curbutton]->Flag_To_Redraw();

          curbutton++;
          if (curbutton > (BUTTON_OPTIONS - BUTTON_BASE)) {
            curbutton = (BUTTON_RESET - BUTTON_BASE);
          }

          buttons[curbutton]->Turn_On();
          buttons[curbutton]->Flag_To_Redraw();
        }
        break;

      case (KN_UP):
        if (curbutton <= (BUTTON_TINT - BUTTON_BASE)) {
          partial = true;
        } else {
          buttons[curbutton]->Turn_Off();
          buttons[curbutton]->Flag_To_Redraw();
        }

        curbutton--;
        if (curbutton == (BUTTON_RESET - BUTTON_BASE)) {
          curbutton--;
        }

        if (curbutton < 0) {
          curbutton = (BUTTON_RESET - BUTTON_BASE);
        }

        if (curbutton <= (BUTTON_TINT - BUTTON_BASE)) {
          partial = true;
        } else {
          buttons[curbutton]->Turn_On();
          buttons[curbutton]->Flag_To_Redraw();
        }
        break;

      case (KN_DOWN):
        if (curbutton <= (BUTTON_TINT - BUTTON_BASE)) {
          partial = true;
        } else {
          buttons[curbutton]->Turn_Off();
          buttons[curbutton]->Flag_To_Redraw();
        }

        curbutton++;
        if (curbutton > (BUTTON_RESET - BUTTON_BASE)) {
          curbutton = 0;
        }

        if (curbutton <= (BUTTON_TINT - BUTTON_BASE)) {
          partial = true;
        } else {
          buttons[curbutton]->Turn_On();
          buttons[curbutton]->Flag_To_Redraw();
        }
        break;

      case (KN_RETURN):
        selection = curbutton + BUTTON_BRIGHTNESS;
        pressed = true;
        break;

      default:
        break;
    }

    if (pressed) {
      switch (selection) {
        case (BUTTON_RESET):
          brightness.Set_Value(128);
          contrast.Set_Value(128);
          color.Set_Value(128);
          tint.Set_Value(128);

          Options.Set_Brightness(128);
          Options.Set_Contrast(128);
          Options.Set_Color(128);
          Options.Set_Tint(128);
          break;

        case (BUTTON_OPTIONS):
          process = false;
          break;
      }

      pressed = false;
    }
  }
}
