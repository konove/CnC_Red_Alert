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

//	Wol_CGam.cpp - Create game dialog.
//	ajw 09/9/98

#include "absl/log/check.h"
#include "port/ex_string.h"
#include "port/safe_string.h"
#include "port/sleep.h"
#include "port/win32/win32_registry.h"
#include "port/win32/win32_system.h"
#include "ra/bigcheck.h"
#include "ra/cheklist.h"
#include "ra/dialog.h"
#include "ra/drop.h"
#include "ra/edit.h"
#include "ra/externs.h"
#include "ra/gadget.h"
#include "ra/gauge.h"
#include "ra/iconlist.h"
#include "ra/init.h"
#include "ra/inline.h"
#include "ra/jshell.h"
#include "ra/msgbox.h"
#include "ra/seditdlg.h"
#include "ra/shapebtn.h"
#include "ra/statbtn.h"
#include "ra/textbtn.h"
#include "ra/theme.h"
#include "ra/wolapiob.h"
#include "ra/wolstrng.h"
#include "ra/ww_audio.h"
#include "sdllib/font.h"
#include "sdllib/timer.h"
#include "sdllib/ww_mouse.h"

// extern char* LoadShpFile( const char* szShpFile );
void SetPlayerCountList(IconListClass& PlayerCountList, int iPlayerMax,
                        char* pShpBoxCheck, char* pShpBoxEmpty);

//***********************************************************************************************
CREATEGAMEINFO WOL_CreateGame_Dialog(WolapiObject* pWO) {
  CREATEGAMEINFO cgiReturn;
  cgiReturn.bCreateGame = false;
  cgiReturn.iPlayerMax = 2;
  cgiReturn.bTournament = false;
  cgiReturn.bPrivate = false;
  cgiReturn.GameKind = CREATEGAMEINFO::RAGAME;
  *cgiReturn.szPassword = 0;

  bool bEscapeDown = false;
  bool bReturnDown = false;

  /*
  **	Dialog & button dimensions
  */
  int d_dialog_w = 300;  // dialog width
  int d_dialog_h = 270;  // dialog height
  int d_dialog_x = ((640 - d_dialog_w) / 2);
  int d_dialog_y = ((400 - d_dialog_h) / 2);
  int d_dialog_cx = d_dialog_x + (d_dialog_w / 2);  // coord of x-center

  int d_margin = 14;  // margin width/height

  int d_gaugeplayers_w = 140;
  int d_gaugeplayers_h = 18;
  int d_gaugeplayers_x = d_dialog_cx - d_gaugeplayers_w / 2;
  int d_gaugeplayers_y = d_dialog_y + d_margin + 42;

  int d_checktourn_w = 150;
  int d_checktourn_h = 18;
  int d_checktourn_x = d_dialog_cx - d_checktourn_w / 2;
  int d_checktourn_y = d_gaugeplayers_y + d_gaugeplayers_h + 10;

  int d_checkpriv_w = d_checktourn_w;
  int d_checkpriv_h = 18;
  int d_checkpriv_x = d_checktourn_x;
  int d_checkpriv_y = d_checktourn_y + d_checktourn_h + 10;

  int d_checkra_w = d_checktourn_w;
  int d_checkra_h = 18;
  int d_checkra_x = d_checktourn_x;
  int d_checkra_y = d_checkpriv_y + d_checkpriv_h + 20;

  int d_checkcs_w = d_checktourn_w;
  int d_checkcs_h = 18;
  int d_checkcs_x = d_checktourn_x;
  int d_checkcs_y = d_checkra_y + d_checkra_h + 5;

  int d_checkam_w = d_checktourn_w;
  int d_checkam_h = 18;
  int d_checkam_x = d_checktourn_x;
  int d_checkam_y = d_checkcs_y + d_checkcs_h + 5;

#if (GERMAN | FRENCH)
  int d_ok_w = 60;
#else
  int d_ok_w = 60;
#endif
  int d_ok_h = 26;
  int d_ok_x = d_dialog_x + (d_dialog_w / 3) - (d_ok_w / 2);
  int d_ok_y = d_dialog_y + d_dialog_h - d_ok_h - d_margin;

  int d_cancel_w = 80;
  int d_cancel_x = d_dialog_x + ((d_dialog_w * 2) / 3) - (d_cancel_w / 2);
  int d_cancel_y = d_ok_y;

  /*
  **	Button enumerations
  */
  enum {
    BUTTON_OK = 100,
    BUTTON_CANCEL,
    GAUGE_PLAYERCOUNT,
    CHECK_TOURNAMENT,
    CHECK_PRIVACY,
    CHECK_RA,
    CHECK_CS,
    CHECK_AM,
  };

  /*
  **	Buttons
  */
  ControlClass* commands = nullptr;  // the button list

  TextButtonClass OkBtn(BUTTON_OK, TXT_OK, kTpfButton, d_ok_x, d_ok_y, d_ok_w);
  TextButtonClass CancelBtn(BUTTON_CANCEL, TXT_CANCEL, kTpfButton, d_cancel_x,
                            d_cancel_y, d_cancel_w);

  StaticButtonClass PlayerCountStatic(0, "               ", kTpfText,
                                      d_gaugeplayers_x, d_gaugeplayers_y - 16);
  GaugeClass PlayerCountGauge(GAUGE_PLAYERCOUNT, d_gaugeplayers_x,
                              d_gaugeplayers_y, d_gaugeplayers_w,
                              d_gaugeplayers_h);

  if (pWO->bEgg8Player) {
    PlayerCountGauge.Set_Maximum(6);
  } else {
    PlayerCountGauge.Set_Maximum(2);
  }
  PlayerCountGauge.Set_Value(cgiReturn.iPlayerMax - 2);

  BigCheckBoxClass TournamentCheck(
      CHECK_TOURNAMENT, d_checktourn_x, d_checktourn_y, d_checktourn_w,
      d_checktourn_h, TXT_WOL_CG_TOURNAMENT, TPF_6PT_GRAD | TPF_NOSHADOW,
      cgiReturn.bTournament);

  BigCheckBoxClass PrivacyCheck(
      CHECK_PRIVACY, d_checkpriv_x, d_checkpriv_y, d_checkpriv_w, d_checkpriv_h,
      TXT_WOL_CG_PRIVACY, TPF_6PT_GRAD | TPF_NOSHADOW, cgiReturn.bPrivate);

  BigCheckBoxClass RA_Check(CHECK_RA, d_checkra_x, d_checkra_y, d_checkra_w,
                            d_checkra_h, TXT_WOL_CG_RAGAME,
                            TPF_6PT_GRAD | TPF_NOSHADOW,
                            cgiReturn.GameKind == CREATEGAMEINFO::RAGAME);
  BigCheckBoxClass CS_Check(CHECK_CS, d_checkcs_x, d_checkcs_y, d_checkcs_w,
                            d_checkcs_h, TXT_WOL_CG_CSGAME,
                            TPF_6PT_GRAD | TPF_NOSHADOW,
                            cgiReturn.GameKind == CREATEGAMEINFO::CSGAME);
  BigCheckBoxClass AM_Check(CHECK_AM, d_checkam_x, d_checkam_y, d_checkam_w,
                            d_checkam_h, TXT_WOL_CG_AMGAME,
                            TPF_6PT_GRAD | TPF_NOSHADOW,
                            cgiReturn.GameKind == CREATEGAMEINFO::AMGAME);

  if (!Is_Counterstrike_Installed()) {
    CS_Check.Disable();
  }

  if (!Is_Aftermath_Installed()) {
    AM_Check.Disable();
  }

  /*
  **	Initialize.
  */
  Set_Logic_Page(SeenBuff);

  /*
  **	Create the button list.
  */
  commands = &OkBtn;
  CancelBtn.Add_Tail(*commands);
  PlayerCountStatic.Add_Tail(*commands);
  PlayerCountGauge.Add_Tail(*commands);
  TournamentCheck.Add_Tail(*commands);
  PrivacyCheck.Add_Tail(*commands);
  RA_Check.Add_Tail(*commands);
  CS_Check.Add_Tail(*commands);
  AM_Check.Add_Tail(*commands);

  char szPlayerCount[100];
  Format_Runtime_Text(szPlayerCount, sizeof(szPlayerCount), TXT_WOL_CG_PLAYERS,
                      cgiReturn.iPlayerMax);
  PlayerCountStatic.Set_Text(szPlayerCount);

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
    Call_Back();

    /*
    **	Refresh display if needed.
    */
    if (display) {
      /*
      **	Display the dialog box.
      */
      Hide_Mouse();
      Dialog_Box(d_dialog_x, d_dialog_y, d_dialog_w, d_dialog_h);
      Draw_Caption(TXT_WOL_CG_TITLE, d_dialog_x, d_dialog_y, d_dialog_w);
      //			Fancy_Text_Print( TXT_WOL_CG_PLAYERS,
      // d_gaugeplayers_x - 2*2, d_gaugeplayers_y,
      //								GadgetClass::Get_Color_Scheme(),
      // TBLACK, kTpfText | TPF_RIGHT );
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
    if (Keyboard->Down(KN_ESC)) {
      bEscapeDown = true;
    } else if (bEscapeDown) {
      input = ButtonKey(BUTTON_CANCEL);
      bEscapeDown = false;
    }
    if (Keyboard->Down(KN_RETURN)) {
      bReturnDown = true;
    } else if (bReturnDown) {
      input = ButtonKey(BUTTON_OK);
      bReturnDown = false;
    }

    /*
    **	Process input.
    */

    switch (input) {
      case ButtonKey(BUTTON_OK):
        cgiReturn.bCreateGame = true;
        process = false;
        break;

      case ButtonKey(BUTTON_CANCEL):
        process = false;
        break;

      case ButtonKey(GAUGE_PLAYERCOUNT):
        if (PlayerCountGauge.Get_Value() != 0 && cgiReturn.bTournament) {
          WWMessageBox().Process(TXT_WOL_TOURNAMENTPLAYERLIMIT);
          PlayerCountGauge.Set_Value(0);
          display = true;
        }
        cgiReturn.iPlayerMax = PlayerCountGauge.Get_Value() + 2;
        Format_Runtime_Text(szPlayerCount, sizeof(szPlayerCount),
                            TXT_WOL_CG_PLAYERS, cgiReturn.iPlayerMax);
        PlayerCountStatic.Set_Text(szPlayerCount);
        PlayerCountStatic.Draw_Me();
        break;

      case ButtonKey(CHECK_TOURNAMENT):
        cgiReturn.bTournament = TournamentCheck.IsOn;
        if (cgiReturn.bTournament) {
          PlayerCountGauge.Set_Value(0);
          //					PlayerCountGauge.Disable();
          cgiReturn.iPlayerMax = 2;
          Format_Runtime_Text(szPlayerCount, sizeof(szPlayerCount),
                              TXT_WOL_CG_PLAYERS, cgiReturn.iPlayerMax);
          PlayerCountStatic.Set_Text(szPlayerCount);
          PlayerCountStatic.Draw_Me();
        }
        //				else
        //					PlayerCountGauge.Enable();
        break;

      case ButtonKey(CHECK_PRIVACY):
        cgiReturn.bPrivate = PrivacyCheck.IsOn;
        break;

      case ButtonKey(CHECK_RA):
        if (RA_Check.IsOn) {
          //	Box was checked.
          CS_Check.Turn_Off();
          AM_Check.Turn_Off();
          cgiReturn.GameKind = CREATEGAMEINFO::RAGAME;
        } else {
          //	Box was unchecked. Has no effect.
          RA_Check.Turn_On();
        }
        break;
      case ButtonKey(CHECK_CS):
        if (CS_Check.IsOn) {
          //	Box was checked.
          RA_Check.Turn_Off();
          AM_Check.Turn_Off();
          cgiReturn.GameKind = CREATEGAMEINFO::CSGAME;
        } else {
          //	Box was unchecked. Has no effect.
          CS_Check.Turn_On();
        }
        break;
      case ButtonKey(CHECK_AM):
        if (AM_Check.IsOn) {
          //	Box was checked.
          RA_Check.Turn_Off();
          CS_Check.Turn_Off();
          cgiReturn.GameKind = CREATEGAMEINFO::AMGAME;
        } else {
          //	Box was unchecked. Has no effect.
          AM_Check.Turn_On();
        }
        break;

      default:
        break;
    }
  }

  if (cgiReturn.bCreateGame && cgiReturn.bPrivate) {
    //	Get a password for the channel.
    Fancy_Text_Print(TXT_NONE, 0, 0, nullptr, TBLACK,
                     kTpfText);  //	Required before String_Pixel_Width()
                                 // call, for god's sake.
    SimpleEditDlgClass* pEditDlg =
        new SimpleEditDlgClass(300, TXT_WOL_CREATEPRIVGAMETITLE,
                               TXT_WOL_PASSPROMPT, WOL_CHANKEY_LEN_MAX);
    pWO->bPump_In_Call_Back = true;
    if (strcmp(pEditDlg->Show(), Text_String(TXT_OK)) == 0 &&
        *pEditDlg->szEdit) {
      port::SafeCopy(cgiReturn.szPassword, pEditDlg->szEdit);
    } else {
      cgiReturn.bCreateGame = false;  //	Cancel creation.
    }
    pWO->bPump_In_Call_Back = false;
  }

  return cgiReturn;
}

//***********************************************************************************************
void SetPlayerCountList(IconListClass& PlayerCountList, int iPlayerMax,
                        char* pShpBoxCheck, char* pShpBoxEmpty) {
  //	Checks appropriate list item based on iPlayerMax.
  switch (iPlayerMax) {
    case 2:
      PlayerCountList.Set_Icon(0, 0, (void*)pShpBoxCheck, ICON_SHAPE);
      PlayerCountList.Set_Icon(1, 0, (void*)pShpBoxEmpty, ICON_SHAPE);
      PlayerCountList.Set_Icon(2, 0, (void*)pShpBoxEmpty, ICON_SHAPE);
      break;
    case 3:
      PlayerCountList.Set_Icon(0, 0, (void*)pShpBoxEmpty, ICON_SHAPE);
      PlayerCountList.Set_Icon(1, 0, (void*)pShpBoxCheck, ICON_SHAPE);
      PlayerCountList.Set_Icon(2, 0, (void*)pShpBoxEmpty, ICON_SHAPE);
      break;
    case 4:
      PlayerCountList.Set_Icon(0, 0, (void*)pShpBoxEmpty, ICON_SHAPE);
      PlayerCountList.Set_Icon(1, 0, (void*)pShpBoxEmpty, ICON_SHAPE);
      PlayerCountList.Set_Icon(2, 0, (void*)pShpBoxCheck, ICON_SHAPE);
      break;
  }
}
