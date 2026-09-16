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

//	Wol_Opt.cpp - WW online options dialog.
//	ajw 09/1/98

#include "port/sleep.h"
#include "ra/bigcheck.h"
#include "ra/config.h"
#include "ra/conquer.h"
#include "ra/defines.h"
#include "ra/dialog.h"
#include "ra/externs.h"
#include "ra/gadget.h"
#include "ra/globals.h"
#include "ra/jshell.h"
#include "ra/msgbox.h"
#include "ra/textbtn.h"
#include "ra/wol_main.h"
#include "ra/wolapiob.h"
#include "ra/wolstrng.h"
#include "sdllib/gbuffer.h"
#include "sdllib/keyboard.h"
#include "sdllib/ww_mouse.h"
// #include "ra/woldebug.h"

//***********************************************************************************************
bool WOL_Options_Dialog(WolapiObject* pWO, bool bCalledFromGame) {
  //	Returns true only if called from inside game, and the game ended on us
  // unexpectedly.
  bool bReturn = false;

  bool bEscapeDown = false;
  bool bReturnDown = false;

  bool bIgnoreReturnDown = false;
  if (KeyboardClass::Down(KN_RETURN)) {
    //	The return key is already down, as we enter the dialog.
    //	Until it comes up again, ignore this fact, so that we don't act on a
    // return press that's not valid.
    bIgnoreReturnDown = true;
  }

  /*
  **	Dialog & button dimensions
  */
  const int d_list_w = config::kIsGerman ? 360 : 330;

  const int d_dialog_w = d_list_w + 80;  // dialog width
  const int d_dialog_h = 180;            // dialog height
  const int d_dialog_x = ((640 - d_dialog_w) / 2);
  const int d_dialog_y = ((400 - d_dialog_h) / 2);
  const int d_dialog_cx = d_dialog_x + (d_dialog_w / 2);  // coord of x-center

  const int d_margin = 14;  // margin width/height

  //	int d_list_w = 100 * 2;
  const int d_list_h = 14;
  const int d_list_x = d_dialog_cx - (d_list_w / 2);
  const int d_list_y = d_dialog_y + d_margin + 24;

  const int d_ok_w = 80;
  const int d_ok_h = 26;
  const int d_ok_x = d_dialog_cx - (d_ok_w / 2);
  const int d_ok_y = d_dialog_y + d_dialog_h - d_ok_h - d_margin;

  /*
  **	Button enumerations
  */
  constexpr int kButtonOk = 100;
  constexpr int kCheckFind = 101;
  constexpr int kCheckPage = 102;
  constexpr int kCheckLanguage = 103;
  constexpr int kCheckAllgames = 104;
  constexpr int kCheckRankam = 105;

  /*
  **	Buttons
  */
  ControlClass* commands = nullptr;  // the button list

  TextButtonClass OkBtn(kButtonOk, TXT_OK, kTpfButton, d_ok_x, d_ok_y, d_ok_w);

  BigCheckBoxClass FindCheck(kCheckFind, d_list_x, d_list_y, d_list_w, d_list_h,
                             TXT_WOL_OPTFIND, TPF_6PT_GRAD | TPF_NOSHADOW,
                             pWO->bFindEnabled);
  BigCheckBoxClass PageCheck(kCheckPage, d_list_x, d_list_y + d_list_h + 2,
                             d_list_w, d_list_h, TXT_WOL_OPTPAGE,
                             TPF_6PT_GRAD | TPF_NOSHADOW, pWO->bPageEnabled);
  BigCheckBoxClass LanguageCheck(kCheckLanguage, d_list_x,
                                 d_list_y + (2 * (d_list_h + 2)), d_list_w,
                                 d_list_h, TXT_WOL_OPTLANGUAGE,
                                 TPF_6PT_GRAD | TPF_NOSHADOW, pWO->bLangFilter);
  BigCheckBoxClass GamescopeCheck(
      kCheckAllgames, d_list_x, d_list_y + (3 * (d_list_h + 2)), d_list_w,
      d_list_h, TXT_WOL_OPTGAMESCOPE, TPF_6PT_GRAD | TPF_NOSHADOW,
      !pWO->bAllGamesShown);
  BigCheckBoxClass RankAMCheck(kCheckRankam, d_list_x,
                               d_list_y + (4 * (d_list_h + 2)), d_list_w,
                               d_list_h, TXT_WOL_OPTRANKAM,
                               TPF_6PT_GRAD | TPF_NOSHADOW, !pWO->bShowRankRA);

  /*
  **	Initialize.
  */
  Set_Logic_Page(SeenBuff);

  /*
  **	Create the button list.
  */
  commands = &OkBtn;
  FindCheck.Add_Tail(*commands);
  PageCheck.Add_Tail(*commands);
  LanguageCheck.Add_Tail(*commands);
  GamescopeCheck.Add_Tail(*commands);
  RankAMCheck.Add_Tail(*commands);

  /*
  **	Main Processing Loop.
  */
  Keyboard->Clear();
  bool display = true;
  bool process = true;
  while (process) {
    /*
    **	Invoke game callback.
    */
    if (!bCalledFromGame) {
      Call_Back();
    } else {
      if (Main_Loop())  //	Game ended on us in the background.
      {
        process = false;
        bReturn = true;
      }
    }

    /*
    **	Refresh display if needed.
    */
    if (display) {
      /*
      **	Display the dialog box.
      */
      Hide_Mouse();
      Dialog_Box(d_dialog_x, d_dialog_y, d_dialog_w, d_dialog_h);
      Draw_Caption(TXT_WOL_OPTTITLE, d_dialog_x, d_dialog_y, d_dialog_w);
      commands->Flag_List_To_Redraw();
      Show_Mouse();
      display = false;
    }

    //	Force mouse visible, as some beta testers report unexplicable
    // disappearing cursors.
    while (Get_Mouse_State()) {
      Show_Mouse();
    }
    //	Be nice to other apps.
    port::SleepMs(50);

    /*
    **	Get user input.
    */
    KeyNumType input = commands->Input();

    //	My hack for triggering escape and return on key up instead of down...
    //	The problem that was occurring was that the calling dialog would act on
    // the key up, 	though this dialog handled the key down. ajw
    if (KeyboardClass::Down(KN_ESC)) {
      bEscapeDown = true;
    } else if (bEscapeDown) {
      input = ButtonKey(kButtonOk);
      bEscapeDown = false;
    }
    if (KeyboardClass::Down(KN_RETURN)) {
      if (!bIgnoreReturnDown) {
        bReturnDown = true;
      }
    } else {
      bIgnoreReturnDown = false;
      if (bReturnDown) {
        input = ButtonKey(kButtonOk);
        bReturnDown = false;
      }
    }

    /*
    **	Process input.
    */

    if (cancel_current_msgbox) {
      cancel_current_msgbox = false;
      input = ButtonKey(kButtonOk);
    }
    switch (static_cast<int>(input)) {
      case ButtonKey(kButtonOk):
        process = false;
        break;

      case ButtonKey(kCheckFind):
      case ButtonKey(kCheckPage):
      case ButtonKey(kCheckLanguage):
      case ButtonKey(kCheckAllgames):
        pWO->SetOptions(FindCheck.IsOn, PageCheck.IsOn, LanguageCheck.IsOn,
                        !GamescopeCheck.IsOn);
        break;

      case ButtonKey(kCheckRankam):
        pWO->bShowRankRA = !RankAMCheck.IsOn;
        pWO->bMyRecordUpdated = true;
        pWO->bShowRankUpdated = true;
        break;

      default:
        break;
    }
  }
  return bReturn;
}
