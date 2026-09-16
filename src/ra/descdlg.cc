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

/* $Header: /CounterStrike/DESCDLG.CPP 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : DESCDLG.CPP *
 *                                                                                             *
 *                   Programmer : Maria del Mar McCready Legg * Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : Jan 26, 1995 *
 *                                                                                             *
 *                  Last Update : Jan 26, 1995   [MML] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * DescriptionClass::Process -- Handles all the options graphic
 *interface.                   *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "ra/descdlg.h"

#include "ra/function.h"

/***********************************************************************************************
 * DescriptionClass::Process -- Handles all the options graphic interface. *
 *                                                                                             *
 *    This dialog uses an edit box to "fill-out" a description. *
 *                                                                                             *
 * INPUT:      char *string - return answer here. * OUTPUT:     none * WARNINGS:
 *none * HISTORY:    12/31/1994 MML : Created. *
 *=============================================================================================*/
void DescriptionClass::Process(char* string) {
  /*
  **	Set up the window.  Window x-coords are in bytes not pixels.
  */
  Set_Window(WINDOW_EDITOR, kOptionX, kOptionY, kOptionWidth, kOptionHeight);
  Set_Logic_Page(SeenBuff);

  /*
  **	Create Buttons.  Button coords are in pixels, but are window-relative.
  */
  TextButtonClass optionsbtn(kButtonOptions, TXT_OK, kTpfButton, 0, kButtonY);
  TextButtonClass cancelbtn(kButtonCancel, TXT_CANCEL, kTpfButton, 0, kButtonY);

  cancelbtn.X = kOptionX + (kOptionWidth - optionsbtn.Width) / 3 * 2;
  optionsbtn.X = kOptionX + (kOptionWidth - optionsbtn.Width) / 3;
  optionsbtn.Add_Tail(cancelbtn);

  EditClass edit(kButtonEdit, string, 31, TPF_6PT_GRAD, 0, kEditY, kEditW);

  edit.Set_Focus();
  edit.X = kOptionX + (kOptionWidth - edit.Width) / 2,
  optionsbtn.Add_Tail(edit);

  /*
  **	This causes left mouse button clicking within the confines of the dialog
  *to *	be ignored if it wasn't recognized by any other button or slider.
  */
  GadgetClass dialog(kOptionX, kOptionY, kOptionWidth, kOptionHeight,
                     GadgetClass::kLeftPress);
  optionsbtn.Add_Tail(dialog);

  /*
  **	This causes a right click anywhere or a left click outside the dialog
  *region *	to be equivalent to clicking on the return to options dialog.
  */
  ControlClass background(kButtonOptions, 0, 0, 320, 200,
                          GadgetClass::kLeftPress | GadgetClass::kRightPress);
  optionsbtn.Add_Tail(background);

  /*
  **	Main Processing Loop
  */
  bool display = true;
  bool process = true;
  while (process) {
    /*
    **	Invoke game callback
    */
    Call_Back();

    /*
    **	Refresh display if needed
    */
    if (display) {
      Window_Hide_Mouse(WINDOW_EDITOR);

      /*
      **	Draw the background
      */
      Window_Box(WINDOW_EDITOR, BOXSTYLE_BORDER);  // has border, raised up
      Draw_Caption(TXT_MISSION_DESCRIPTION, kOptionX, kOptionY, kOptionWidth);

      /*
      **	Draw the titles
      */
      optionsbtn.Draw_All();
      Window_Show_Mouse();
      display = false;
    }

    /*
    **	Get user input
    */
    KeyNumType input = optionsbtn.Input();

    /*
    **	Process Input
    */
    switch (input) {
      case KN_RETURN:
      case ButtonKey(kButtonOptions):
        strtrim(string);
        process = false;
        break;

      case KN_ESC:
      case ButtonKey(kButtonCancel):
        string[0] = NULL;
        strtrim(string);
        process = false;
        break;

      case ButtonKey(kButtonEdit):
        break;

      default:
        break;
    }
  }
}
