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

/* $Header: /CounterStrike/MPLAYER.CPP 3     3/13/97 2:06p Steve_tall $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : MPLAYER.CPP *
 *                                                                                             *
 *                   Programmer : Bill Randolph *
 *                                                                                             *
 *                   Start Date : April 14, 1995 *
 *                                                                                             *
 *                  Last Update : November 30, 1995 [BRR] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * Select_MPlayer_Game -- prompts user for NULL-Modem, Modem, or
 *Network game                * Clear_Listbox -- clears the given list box *
 *   Clear_Vector -- clears the given NodeNameType vector * Computer_Message --
 *"sends" a message from the computer                                   *
 *   Garble_Message -- "garbles" a message * Surrender_Dialog -- Prompts user
 *for surrendering                                         * Abort_Dialog --
 *Prompts user for confirmation on aborting the mission
 **
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "ra/mplayer.h"

#include "absl/log/check.h"
#include "base/array.h"
#include "ra/config.h"
#include "ra/conquer.h"
#include "ra/control.h"
#include "ra/defines.h"
#include "ra/dialog.h"
#include "ra/externs.h"
#include "ra/gadget.h"
#include "ra/globals.h"
#include "ra/init.h"
#include "ra/inline.h"
#include "ra/ipxmgr.h"
#include "ra/jshell.h"
#include "ra/list.h"
#include "ra/mapedit.h"
#include "ra/nulldlg.h"
#include "ra/palette.h"
#include "ra/session.h"
#include "ra/textbtn.h"
#include "ra/vector_dynamic.h"
#include "ra/wolstrng.h"
#include "sdllib/gbuffer.h"
#include "sdllib/keyboard.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/wwstd.h"

/***********************************************************************************************
 * Select_MPlayer_Game -- prompts user for NULL-Modem, Modem, or Network game *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * GAME_NORMAL, GAME_MODEM, etc. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 02/14/1995 BR : Created. *
 *=============================================================================================*/
GameType Select_MPlayer_Game() {
  //------------------------------------------------------------------------
  //	Dialog & button dimensions
  //------------------------------------------------------------------------
  const int d_dialog_w = 380;
  //	The Westwood Online button makes the dialog taller, and it recentres
  //	rather than keeping the fixed y the smaller one used.
  int d_dialog_h = config::kWolapiEnabled ? 178 : 156;  //	ajw
  const int d_dialog_y = config::kWolapiEnabled ? (510 - d_dialog_h) / 2 : 180;
  const int d_dialog_x = (640 - d_dialog_w) / 2;
  const int d_dialog_cx = d_dialog_x + (d_dialog_w / 2);

  const int d_txt6_h = 14;
  const int d_margin = 14;

  const int d_modemserial_w = 160;
  const int d_modemserial_h = 18;
  const int d_modemserial_x = d_dialog_cx - (d_modemserial_w / 2);
  const int d_modemserial_y = d_dialog_y + d_margin + d_txt6_h + d_margin;

  const int d_skirmish_w = 160;
  const int d_skirmish_h = 18;
  const int d_skirmish_x = d_dialog_cx - (d_skirmish_w / 2);
  const int d_skirmish_y = d_modemserial_y + d_modemserial_h + 4;

  const int d_ipx_w = 160;
  const int d_ipx_h = 18;
  const int d_ipx_x = d_dialog_cx - (d_ipx_w / 2);
  const int d_ipx_y = d_skirmish_y + d_skirmish_h + 4;

  //	ajw 7/2/98 - added button
  const int d_wol_w = 160;
  const int d_wol_h = 18;
  const int d_wol_x = d_dialog_cx - (d_wol_w / 2);
  const int d_wol_y = d_ipx_y + d_ipx_h + 4;

  const int d_cancel_w = 120;
  const int d_cancel_h = 18;
  const int d_cancel_x = d_dialog_cx - (d_cancel_w / 2);
  int d_cancel_y = config::kWolapiEnabled ? d_wol_y + d_wol_h + d_margin
                                          : d_ipx_y + d_ipx_h + d_margin;

  //------------------------------------------------------------------------
  //	Button enumerations:
  //------------------------------------------------------------------------
  constexpr int kButtonModemserial = 100;
  constexpr int kButtonSkirmish = 101;
  constexpr int kButtonIpx = 102;
  constexpr int kButtonWol = 103;  //	ajw
  constexpr int kButtonCancel = 104;
  //	BUTTON_WOL keeps its slot either way; the pointer stays null when
  //	the button is not built.
  constexpr int kNumOfButtons = 5;  //	ajw

  // Sampled once: the button list, its length and the cancel-button fixup
  // below all have to agree on how many buttons this dialog has.
  const bool has_ipx = Ipx.Is_IPX();
  //	The IPX and Westwood Online buttons are each present or not; the count
  //	drives keyboard navigation, so it has to match what was actually built.
  const int num_of_buttons =
      kNumOfButtons - (has_ipx ? 0 : 1) - (config::kWolapiEnabled ? 0 : 1);
  //------------------------------------------------------------------------
  //	Redraw values: in order from "top" to "bottom" layer of the dialog
  //------------------------------------------------------------------------
  enum class RedrawType {
    REDRAW_NONE = 0,
    REDRAW_BUTTONS = 1,     // includes map interior & coord values
    REDRAW_BACKGROUND = 2,  // includes box, map bord, key, coord labels, btns
    REDRAW_ALL = REDRAW_BACKGROUND
  };
  using enum RedrawType;

  //------------------------------------------------------------------------
  //	Dialog variables:
  //------------------------------------------------------------------------
  GameType retval = GAME_NORMAL;  // return value
  int selection = 0;
  TextButtonClass* buttons[kNumOfButtons] = {};

  //------------------------------------------------------------------------
  //	Buttons
  //------------------------------------------------------------------------
  ControlClass* commands = nullptr;  // the button list

  //------------------------------------------------------------------------
  // If IPX not active then do only the modem serial dialog
  //------------------------------------------------------------------------
  //	if ( !Ipx.Is_IPX() ) {
  //		return( Select_Serial_Dialog() );
  //	}

  TextButtonClass modemserialbtn(kButtonModemserial, TXT_MODEM_SERIAL,
                                 kTpfButton, d_modemserial_x, d_modemserial_y,
                                 d_modemserial_w, d_modemserial_h);

  TextButtonClass skirmishbtn(kButtonSkirmish, TXT_SKIRMISH, kTpfButton,
                              d_skirmish_x, d_skirmish_y, d_skirmish_w,
                              d_skirmish_h);

  TextButtonClass ipxbtn(kButtonIpx, TXT_NETWORK, kTpfButton, d_ipx_x, d_ipx_y,
                         d_ipx_w, d_ipx_h);

  //	ajw
  TextButtonClass wolbtn(kButtonWol, TXT_WOL_INTERNETBUTTON, kTpfButton,
                         d_wol_x, d_wol_y, d_wol_w, d_wol_h);

  if (!has_ipx) {
    d_cancel_y = d_ipx_y;
    d_dialog_h -= d_cancel_h;
  }

  TextButtonClass cancelbtn(kButtonCancel, TXT_CANCEL, kTpfButton, d_cancel_x,
                            d_cancel_y, d_cancel_w, d_cancel_h);

  //------------------------------------------------------------------------
  //	Initialize
  //------------------------------------------------------------------------
  Set_Logic_Page(SeenBuff);
  //------------------------------------------------------------------------
  //	Create the list
  //------------------------------------------------------------------------
  commands = &modemserialbtn;
  skirmishbtn.Add_Tail(*commands);
  if (has_ipx) {
    ipxbtn.Add_Tail(*commands);
  }
  if constexpr (config::kWolapiEnabled) {
    wolbtn.Add_Tail(*commands);  //	ajw
  }
  cancelbtn.Add_Tail(*commands);

  //------------------------------------------------------------------------
  //	Fill array of button ptrs
  //------------------------------------------------------------------------
  int curbutton = 0;
  base::At(buttons, 0) = &modemserialbtn;
  base::At(buttons, 1) = &skirmishbtn;
  int iButton = 2;
  if (has_ipx) {
    base::At(buttons, iButton++) = &ipxbtn;
  }
  if constexpr (config::kWolapiEnabled) {
    base::At(buttons, iButton++) = &wolbtn;  //	ajw
  }
  base::At(buttons, iButton) = &cancelbtn;
  base::At(buttons, curbutton)->Turn_On();

  Keyboard->Clear();

  Fancy_Text_Print(TXT_NONE, 0, 0, GadgetClass::Get_Color_Scheme(), kTBlack,
                   TPF_CENTER | kTpfText);

  //------------------------------------------------------------------------
  //	Main Processing Loop
  //------------------------------------------------------------------------
  RedrawType display = REDRAW_ALL;  // true = re-draw everything
  bool process = true;              // loop while true
  bool pressed = false;
  while (process) {
    //.....................................................................
    //	Invoke game callback
    //.....................................................................
    Call_Back();

    //.....................................................................
    //	Refresh display if needed
    //.....................................................................
    if (display != REDRAW_NONE) {
      Hide_Mouse();
      if (display >= REDRAW_BACKGROUND) {
        //...............................................................
        //	Refresh the backdrop
        //...............................................................
        Load_Title_Page(true);
        CCPalette.Set();

        //...............................................................
        //	Draw the background
        //...............................................................
        Dialog_Box(d_dialog_x, d_dialog_y, d_dialog_w, d_dialog_h);
        Draw_Caption(TXT_SELECT_MPLAYER_GAME, d_dialog_x, d_dialog_y,
                     d_dialog_w);
      }

      //..................................................................
      //	Redraw buttons
      //..................................................................
      if (display >= REDRAW_BUTTONS) {
        commands->Flag_List_To_Redraw();
      }
      Show_Mouse();
      display = REDRAW_NONE;
    }

    //.....................................................................
    //	Get user input
    //.....................................................................
    const KeyNumType input = commands->Input();  // input from user

    //.....................................................................
    //	Process input
    //.....................................................................
    switch (static_cast<int>(input)) {
      case ButtonKey(kButtonModemserial):
        selection = kButtonModemserial;
        pressed = true;
        break;

      case ButtonKey(kButtonSkirmish):
        selection = kButtonSkirmish;
        pressed = true;
        break;

      case ButtonKey(kButtonIpx):
        selection = kButtonIpx;
        pressed = true;
        break;

      case ButtonKey(kButtonWol):  //	ajw
        selection = kButtonWol;
        pressed = true;
        break;

      case KN_ESC:
      case ButtonKey(kButtonCancel):
        selection = kButtonCancel;
        pressed = true;
        break;

      case KN_UP:
        base::At(buttons, curbutton)->Turn_Off();
        base::At(buttons, curbutton)->Flag_To_Redraw();
        curbutton--;
        if (curbutton < 0) {
          curbutton = num_of_buttons - 1;
        }
        base::At(buttons, curbutton)->Turn_On();
        base::At(buttons, curbutton)->Flag_To_Redraw();
        break;

      case KN_DOWN:
        base::At(buttons, curbutton)->Turn_Off();
        base::At(buttons, curbutton)->Flag_To_Redraw();
        curbutton++;
        if (curbutton > num_of_buttons - 1) {
          curbutton = 0;
        }
        base::At(buttons, curbutton)->Turn_On();
        base::At(buttons, curbutton)->Flag_To_Redraw();
        break;

      case KN_RETURN:
        selection = curbutton + kButtonModemserial;
        pressed = true;
        break;

      default:
        break;
    }

    if (pressed) {
      //..................................................................
      // to make sure the selection is correct in case they used the mouse
      //..................................................................
      base::At(buttons, curbutton)->Turn_Off();
      base::At(buttons, curbutton)->Flag_To_Redraw();
      curbutton = selection - kButtonModemserial;
      //	BUTTON_WOL is in the enum either way, but only takes a slot in
      //	buttons[] when it was actually built.
      if (!config::kWolapiEnabled && selection > kButtonWol) {
        curbutton--;
      }
      if (selection == kButtonCancel && !has_ipx) {
        curbutton--;
      }
      DCHECK(base::At(buttons, curbutton) != nullptr);
      base::At(buttons, curbutton)->Turn_On();
      base::At(buttons, curbutton)->IsPressed = true;
      base::At(buttons, curbutton)->Draw_Me(true);

      switch (selection) {
        case kButtonModemserial:

          //............................................................
          // Pop up the modem/serial/com port dialog
          //............................................................
          retval = Select_Serial_Dialog();

          if (retval != GAME_NORMAL) {
            process = false;
          } else {
            base::At(buttons, curbutton)->IsPressed = false;
            display = REDRAW_ALL;
          }
          break;

        case kButtonSkirmish:
          Session.Type = GAME_SKIRMISH;
          if (Com_Scenario_Dialog(true)) {
            retval = GAME_SKIRMISH;
            process = false;
            bAftermathMultiplayer = Is_Aftermath_Installed();
            //	ajw I'll bet this was needed before also...
            Session.ScenarioIsOfficial =
                Session.Scenarios.at(Session.Options.ScenarioIndex)
                    ->Get_Official();
          } else {
            base::At(buttons, curbutton)->IsPressed = false;
            Session.Type = GAME_NORMAL;
            display = REDRAW_ALL;
          }
          break;

        case kButtonIpx:
          retval = GAME_IPX;
          process = false;
          break;

        case kButtonWol:  //	ajw
          retval = GAME_INTERNET;
          process = false;
          break;

        case kButtonCancel:
          retval = GAME_NORMAL;
          process = false;
          break;
        default:
          break;
      }

      pressed = false;
    }
  }
  return retval;

} /* end of Select_MPlayer_Game */

/***************************************************************************
 * Clear_Listbox -- clears the given list box                              *
 *                                                                         *
 * This routine assumes the items in the given list box are character * buffers;
 *it deletes each item in the list, then clears the list. *
 *                                                                         *
 * INPUT:                                                                  *
 *		list			ptr to listbox
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		none.
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   11/29/1995 BRR : Created.                                             *
 *=========================================================================*/
void Clear_Listbox(ListClass* list) { list->Clear(); }  // end of Clear_Listbox

/***************************************************************************
 * Clear_Vector -- clears the given NodeNameType vector                    *
 *                                                                         *
 * INPUT:                                                                  *
 *		vector		ptr to vector to clear
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		none.
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   11/29/1995 BRR : Created.                                             *
 *=========================================================================*/
void Clear_Vector(DynamicVectorClass<NodeNameType*>* vector) {

  //------------------------------------------------------------------------
  //	Clear the 'Players' Vector
  //------------------------------------------------------------------------
  for (int i = 0; i < vector->Count(); i++) {
    delete (*vector).at(i);
  }
  vector->Clear();

}  // end of Clear_Vector

/***************************************************************************
 * Computer_Message -- "sends" a message from the computer                 *
 *                                                                         *
 * INPUT:                                                                  *
 *      none.                                                              *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      none.                                                              *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   06/06/1995 BRR : Created.                                             *
 *=========================================================================*/
void Computer_Message() {
} /* end of Computer_Message */

/***************************************************************************
 * Surrender_Dialog -- Prompts user for surrendering                       *
 *                                                                         *
 * INPUT:                                                                  *
 *      none.                                                              *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      0 = user cancels, 1 = user wants to surrender.                     *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   07/05/1995 BRR : Created.                                             *
 *=========================================================================*/
int Surrender_Dialog(int text) { return Surrender_Dialog(Text_String(text)); }

int Surrender_Dialog(const char* text) {
  //------------------------------------------------------------------------
  //	Dialog & button dimensions
  //------------------------------------------------------------------------
  constexpr int kDDialogW = 480;                           // dialog width
  constexpr int kDDialogH = 126;                           // dialog height
  constexpr int kDDialogX = (640 - kDDialogW) / 2;         // centered x-coord
  constexpr int kDDialogY = (400 - kDDialogH) / 2;         // centered y-coord
  constexpr int kDDialogCx = kDDialogX + (kDDialogW / 2);  // coord of x-center
  constexpr int kDMargin = 10;                    // margin width/height
  constexpr int kDTopmargin = 40;                 // top margin
  constexpr int kDOkW = 90;                       // OK width
  constexpr int kDOkH = 18;                       // OK height
  constexpr int kDOkX = kDDialogCx - kDOkW - 10;  // OK x
  constexpr int kDOkY = kDDialogY + kDDialogH - kDOkH - (kDMargin * 2);  // OK y
  constexpr int kDCancelW = 90;               // Cancel width
  constexpr int kDCancelH = 18;               // Cancel height
  constexpr int kDCancelX = kDDialogCx + 10;  // Cancel x
  constexpr int kDCancelY =
      kDDialogY + kDDialogH - kDCancelH - (kDMargin * 2);  // Cancel y

  //------------------------------------------------------------------------
  //	Button enumerations
  //------------------------------------------------------------------------
  constexpr int kButtonOk = 100;
  constexpr int kButtonCancel = 101;

  //------------------------------------------------------------------------
  //	Buttons
  //------------------------------------------------------------------------
  ControlClass* commands = nullptr;  // the button list

  TextButtonClass okbtn(kButtonOk, TXT_OK, kTpfButton, kDOkX, kDOkY, kDOkW,
                        kDOkH);

  TextButtonClass cancelbtn(kButtonCancel, TXT_CANCEL, kTpfButton, kDCancelX,
                            kDCancelY, kDCancelW, kDCancelH);

  TextButtonClass* buttons[2];
  int curbutton = 0;

  //------------------------------------------------------------------------
  //	Initialize
  //------------------------------------------------------------------------
  Set_Logic_Page(SeenBuff);

  //------------------------------------------------------------------------
  //	Create the button list
  //------------------------------------------------------------------------
  commands = &okbtn;
  cancelbtn.Add_Tail(*commands);

  base::At(buttons, 0) = &okbtn;
  base::At(buttons, 1) = &cancelbtn;
  base::At(buttons, curbutton)->Turn_On();

  //------------------------------------------------------------------------
  //	Main Processing Loop
  //------------------------------------------------------------------------
  int retcode = 0;
  bool display = true;
  bool process = true;
  while (process) {
    //.....................................................................
    //	Invoke game callback
    //.....................................................................
    if ((Session.Type != GAME_SKIRMISH) && Main_Loop()) {
      retcode = 0;
      process = false;
    }

    //.....................................................................
    //	Refresh display if needed
    //.....................................................................
    if (display) {
      display = false;

      //..................................................................
      //	Display the dialog box
      //..................................................................
      Hide_Mouse();
      Dialog_Box(kDDialogX, kDDialogY, kDDialogW, kDDialogH);
      Draw_Caption(TXT_NONE, kDDialogX, kDDialogY, kDDialogW);

      //...............................................................
      //	Draw the captions
      //...............................................................
      // Stalemate games.
      Fancy_Text_Print(text, kDDialogCx, kDDialogY + kDTopmargin,
                       GadgetClass::Get_Color_Scheme(), kTBlack,
                       TPF_CENTER | kTpfText);

      //..................................................................
      //	Redraw the buttons
      //..................................................................
      commands->Flag_List_To_Redraw();
      Show_Mouse();
    }

    //.....................................................................
    //	Get user input
    //.....................................................................
    const KeyNumType input = commands->Input();

    //.....................................................................
    //	Process input
    //.....................................................................
    switch (static_cast<int>(input)) {
      case ButtonKey(kButtonOk):
        retcode = 1;
        process = false;
        break;

      case ButtonKey(kButtonCancel):
        retcode = 0;
        process = false;
        break;

      case KN_RETURN:
        if (curbutton == 0) {
          retcode = 1;
        } else {
          retcode = 0;
        }
        process = false;
        break;

      case KN_ESC:
        retcode = 0;
        process = false;
        break;

      case KN_RIGHT:
        base::At(buttons, curbutton)->Turn_Off();
        curbutton++;
        if (curbutton > 1) {
          curbutton = 0;
        }
        base::At(buttons, curbutton)->Turn_On();
        break;

      case KN_LEFT:
        base::At(buttons, curbutton)->Turn_Off();
        curbutton--;
        if (curbutton < 0) {
          curbutton = 1;
        }
        base::At(buttons, curbutton)->Turn_On();
        break;

      default:
        break;
    }
  }

  //------------------------------------------------------------------------
  //	Redraw the display
  //------------------------------------------------------------------------
  HidPage.Clear();
  Map.Flag_To_Redraw(true);
  Map.Render();

  return retcode;
}

/***************************************************************************
 * Abort_Dialog -- Prompts user for confirmation on aborting the mission
 **
 *                                                                         *
 * INPUT:                                                                  *
 *      none.                                                              *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      1 = user confirms abort, 0 = user cancels
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   07/05/1995 BRR : Created.                                             *
 *=========================================================================*/
int Abort_Dialog() {
  //------------------------------------------------------------------------
  //	Dialog & button dimensions
  //------------------------------------------------------------------------
  constexpr int kDDialogW = 340;                           // dialog width
  constexpr int kDDialogH = 126;                           // dialog height
  constexpr int kDDialogX = (640 - kDDialogW) / 2;         // centered x-coord
  constexpr int kDDialogY = (400 - kDDialogH) / 2;         // centered y-coord
  constexpr int kDDialogCx = kDDialogX + (kDDialogW / 2);  // coord of x-center
  constexpr int kDMargin = 10;                      // margin width/height
  constexpr int kDTopmargin = 40;                   // top margin
  constexpr int kDYesW = 90;                        // YES width
  constexpr int kDYesH = 18;                        // YES height
  constexpr int kDYesX = kDDialogCx - kDYesW - 10;  // YES x
  constexpr int kDYesY =
      kDDialogY + kDDialogH - kDYesH - (kDMargin * 2);  // YES y
  constexpr int kDNoW = 90;                             // Cancel width
  constexpr int kDNoH = 18;                             // Cancel height
  constexpr int kDNoX = kDDialogCx + 10;                // Cancel x
  constexpr int kDNoY =
      kDDialogY + kDDialogH - kDNoH - (kDMargin * 2);  // Cancel y

  //------------------------------------------------------------------------
  //	Button enumerations
  //------------------------------------------------------------------------
  constexpr int kButtonYes = 100;
  constexpr int kButtonNo = 101;

  //------------------------------------------------------------------------
  //	Buttons
  //------------------------------------------------------------------------
  ControlClass* commands = nullptr;  // the button list

  TextButtonClass yesbtn(kButtonYes, TXT_YES, kTpfButton, kDYesX, kDYesY,
                         kDYesW, kDYesH);

  TextButtonClass nobtn(kButtonNo, TXT_NO, kTpfButton, kDNoX, kDNoY, kDNoW,
                        kDNoH);

  TextButtonClass* buttons[2];
  int curbutton = 0;

  //------------------------------------------------------------------------
  //	Initialize
  //------------------------------------------------------------------------
  Set_Logic_Page(SeenBuff);

  //------------------------------------------------------------------------
  //	Create the button list
  //------------------------------------------------------------------------
  commands = &yesbtn;
  nobtn.Add_Tail(*commands);

  base::At(buttons, 0) = &yesbtn;
  base::At(buttons, 1) = &nobtn;
  base::At(buttons, curbutton)->Turn_On();

  //------------------------------------------------------------------------
  //	Main Processing Loop
  //------------------------------------------------------------------------
  int retcode = 0;
  bool display = true;
  bool process = true;
  while (process) {
    //.....................................................................
    //	Invoke game callback
    //.....................................................................
    if ((Session.Type != GAME_SKIRMISH) && Main_Loop()) {
      retcode = 0;
      process = false;
    }

    //.....................................................................
    //	Refresh display if needed
    //.....................................................................
    if (display) {
      display = false;

      //..................................................................
      //	Display the dialog box
      //..................................................................
      Hide_Mouse();
      Dialog_Box(kDDialogX, kDDialogY, kDDialogW, kDDialogH);
      Draw_Caption(TXT_NONE, kDDialogX, kDDialogY, kDDialogW);

      //...............................................................
      //	Draw the captions
      //...............................................................
      Fancy_Text_Print(Text_String(TXT_CONFIRM_EXIT), kDDialogCx,
                       kDDialogY + kDTopmargin, GadgetClass::Get_Color_Scheme(),
                       kTBlack, TPF_CENTER | kTpfText);

      //..................................................................
      //	Redraw the buttons
      //..................................................................
      commands->Flag_List_To_Redraw();
      Show_Mouse();
    }

    //.....................................................................
    //	Get user input
    //.....................................................................
    const KeyNumType input = commands->Input();

    //.....................................................................
    //	Process input
    //.....................................................................
    switch (static_cast<int>(input)) {
      case ButtonKey(kButtonYes):
        retcode = 1;
        process = false;
        break;

      case ButtonKey(kButtonNo):
        retcode = 0;
        process = false;
        break;

      case KN_RETURN:
        if (curbutton == 0) {
          retcode = 1;
        } else {
          retcode = 0;
        }
        process = false;
        break;

      case KN_ESC:
        retcode = 0;
        process = false;
        break;

      case KN_RIGHT:
        base::At(buttons, curbutton)->Turn_Off();
        curbutton++;
        if (curbutton > 1) {
          curbutton = 0;
        }
        base::At(buttons, curbutton)->Turn_On();
        break;

      case KN_LEFT:
        base::At(buttons, curbutton)->Turn_Off();
        curbutton--;
        if (curbutton < 0) {
          curbutton = 1;
        }
        base::At(buttons, curbutton)->Turn_On();
        break;

      default:
        break;
    }
  }

  //------------------------------------------------------------------------
  //	Redraw the display
  //------------------------------------------------------------------------
  HidPage.Clear();
  Map.Flag_To_Redraw(true);
  Map.Render();

  return retcode;
}

/************************** end of mplayer.cpp *****************************/
