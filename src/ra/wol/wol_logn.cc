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

//	Wol_Logn.cpp - WW online name/password dialog.
//	ajw 07/16/98

#include <cstring>
#include <span>
#include <string_view>

#include "base/strings/safe_string.h"
#include "engine/file/mix_archive.h"
#include "engine/gfx/font.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/platform/sleep.h"
#include "engine/platform/win32/win32_com.h"
#include "engine/platform/win32/win32_types.h"
#include "engine/window/hardware_cursor.h"
#include "engine/window/keyboard.h"
#include "game/ui/gadget.h"
#include "game/ui/gadget_list.h"
#include "game/ui/input.h"
#include "game/ui/standard_colors.h"
#include "ra/assets/screen.h"
#include "ra/core/config.h"
#include "ra/core/defines.h"
#include "ra/data/text_ids.h"
#include "ra/startup/conquer.h"
#include "ra/startup/game_state.h"
#include "ra/startup/init.h"
#include "ra/ui/bigcheck.h"
#include "ra/ui/color_scheme.h"
#include "ra/ui/dialog.h"
#include "ra/ui/edit.h"
#include "ra/ui/iconlist.h"
#include "ra/ui/msgbox.h"
#include "ra/ui/textbtn.h"
#include "ra/wol/chatdefs.h"
#include "ra/wol/passedit.h"
#include "ra/wol/rawolapi.h"
#include "ra/wol/wol_main.h"
#include "ra/wol/wolapiob.h"
#include "ra/wol/woledit.h"
#include "ra/wol/wolstrng.h"

namespace ra {

static bool ReadSavedNicks(const WolapiObject* pWO, IconListClass& NickList,
                           std::span<char> szNameBuffer,
                           std::span<char> szPassBuffer);
static bool bSaveNick(const WolapiObject* pWO, const char* szNickToSave,
                      const char* szPassToSave, bool bPassIsMangled);
static void DeleteNick(const WolapiObject* pWO, int iOneBasedEntryToDelete);
// char* LoadShpFile( const char* szShpFile );

// #include "ra/wol/woldebug.h"

//***********************************************************************************************
int WOL_Login_Dialog(WolapiObject* pWO) {
  //	Return values: 0 = user cancels, 1 = success, -1 = force game exit

  if (pWO->bLoggedIn()) {
    pWO->bReturningAfterGame = true;  //	Set trigger for chat dialog.
    return 1;  //	We are already logged in, and have just come back from a
               // game.
  }

  /*
  **	Dialog & button dimensions
  */
  const int d_dialog_w = config::kIsFrench ? 320 : 300;  // dialog width
  const int d_dialog_h = 170;                            // dialog height
  const int d_dialog_x = ((640 - d_dialog_w) / 2);
  const int d_dialog_y = ((510 - d_dialog_h) / 2);

  const int top_margin = 0;

  const int d_name_w = 132;
  const int d_name_x = d_dialog_x + (config::kIsFrench ? 50 : 40);
  const int d_name_y = d_dialog_y + top_margin + 50;

  const int d_pass_w = 72;
  const int d_pass_x = d_name_x + d_name_w + 12;
  const int d_pass_y = d_name_y;

  const int d_list_w = d_name_w;
  const int d_list_h = 40;
  const int d_list_x = d_name_x;
  const int d_list_y = d_dialog_y + top_margin + 80;

  //	int d_save_w = d_pass_w;
  const int d_save_h = 18;
  const int d_save_x = d_pass_x + (d_pass_w / 2) - (d_pass_w / 2);
  const int d_save_y = d_list_y;  // + ( d_list_h / 2 ) - ( d_save_h / 2 );

  const int d_delete_w = d_pass_w;
  const int d_delete_h = 20;
  const int d_delete_x = d_save_x;
  const int d_delete_y = d_list_y + d_list_h - d_delete_h;

  const int d_connect_w = config::kIsFrench ? 90 : 80;
  const int d_connect_x = d_name_x + (d_name_w / 2) - (d_connect_w / 2);
  const int d_connect_y =
      d_dialog_y + top_margin +
      130;  // d_dialog_y + d_dialog_h - d_connect_h - d_margin;

  const int d_cancel_w = 80;
  const int d_cancel_x =
      d_pass_x + (d_pass_w / 2) - (d_cancel_w / 2);  // d_dialog_cx + d_margin;
  const int d_cancel_y = d_connect_y;

  /*
  **	Button enumerations
  */
  constexpr int kButtonConnect = 100;
  constexpr int kButtonCancel = 101;
  constexpr int kListboxNicks = 102;
  constexpr int kEditboxName = 103;
  constexpr int kEditboxPass = 104;
  constexpr int kButtonSavecheck = 105;
  constexpr int kButtonDelete = 106;

  /*
  **	Redraw values: in order from "top" to "bottom" layer of the dialog
  */

  /*
  **	Dialog variables
  */
  int iReturn = 1;  //	0 = user cancels, 1 = success, -1 = force game exit

  /*
  **	Other Variables
  */
  char szNameBuffer[WOL_NAME_LEN_MAX] = {0};  //	User name.
  char szPassBuffer[WOL_PASSWORD_LEN] = {0};  //	User password.

  /*
  **	Buttons
  */
  game::GadgetList commands;  // the button list

  TextButtonClass ConnectBtn(kButtonConnect, TXT_WOL_CONNECT, kButtonFont,
                             kButtonFlags, d_connect_x, d_connect_y,
                             d_connect_w);
  TextButtonClass CancelBtn(kButtonCancel, TXT_CANCEL, kButtonFont,
                            kButtonFlags, d_cancel_x, d_cancel_y, d_cancel_w);

  IconListClass NickList(kListboxNicks, d_list_x, d_list_y, d_list_w, d_list_h,
                         FontType::k6PointGradient, TPF_NOSHADOW,
                         MixArchive::RetrieveData("BTN-UP.SHP"),
                         MixArchive::RetrieveData("BTN-DN.SHP"), true, 1, 0);

  WOLEditClass NameEdit(kEditboxName, szNameBuffer, sizeof(szNameBuffer),
                        FontType::k6PointGradient, TPF_NOSHADOW, d_name_x,
                        d_name_y, d_name_w, -1, EditClass::kAlphanumeric);

  PassEditClass PassEdit(kEditboxPass, szPassBuffer, sizeof(szPassBuffer),
                         FontType::k6PointGradient, TPF_NOSHADOW, d_pass_x,
                         d_pass_y, d_pass_w, -1, EditClass::kAlphanumeric);

  PixelView& view = TheScreen().visible_view();

  const int iSaveTextWidth =
      StringPixelWidth(TextFontStyle(FontType::k6PointGradient, TPF_NOSHADOW),
                       TXT_WOL_SAVELOGIN) +
      BIGCHECK_OFFSETX;
  BigCheckBoxClass SaveCheckBox(kButtonSavecheck, d_save_x, d_save_y,
                                iSaveTextWidth, d_save_h, TXT_WOL_SAVELOGIN,
                                FontType::k6PointGradient, TPF_NOSHADOW, true);

  TextButtonClass DeleteBtn(kButtonDelete, TXT_DELETE_BUTTON, kButtonFont,
                            kButtonFlags, d_delete_x, d_delete_y, d_delete_w);

  //	Get saved nickname/passwords from the registry.
  if (ReadSavedNicks(pWO, NickList, szNameBuffer, szPassBuffer)) {
    PassEdit.bClearOnNextSetFocus = true;
  } else {
    //	Offer user the chance to go to web site now to get a nick.
    if (pWO->DoWebRegistration()) {
      //	User chose to go to web page. Leave function so that we'll
      // re-read nicks when they return.
      return 0;
    }
  }

  /*
  **	Create the button list.
  */
  commands.Add(ConnectBtn);
  commands.Add(CancelBtn);
  commands.Add(NickList);
  commands.Add(NameEdit);
  commands.Add(PassEdit);
  commands.Add(SaveCheckBox);
  commands.Add(DeleteBtn);
  NameEdit.SetFocus();

  if (NickList.Count() == 0) {
    DeleteBtn.Disable();
  }

  /*
  **	Main Processing Loop.
  */
  game::TheKeyboard().Clear();
  bool firsttime = true;
  bool display = true;
  bool process = true;
  while (process) {
    /*
    **	Invoke game callback.
    */
    ServiceRealTime();

    /*
    **	Refresh display if needed.
    */
    if (display) {
      //------------------------------------------------------------------------
      //	Clear screen
      //------------------------------------------------------------------------
      game::TheMouse().Hide();
      Load_Title_Page(true);
      //			ShowMouse();

      /*
      **	Display the dialog box.
      */
      //			HideMouse();

      Dialog_Box(d_dialog_x, d_dialog_y, d_dialog_w, d_dialog_h);
      Draw_Caption(view, TXT_WOL_LOGINDIALOG, d_dialog_x, d_dialog_y,
                   d_dialog_w);

      /*
      **	Redraw the buttons.
      */
      {
        const TextPrinter centered(
            view, {Get_Color_Scheme(), game::kTransparentBlack, kTextFont,
                   kTextFlags | TPF_CENTER});
        centered.Print(TXT_WOL_NAME,
                       {d_name_x + (d_name_w / 2), d_name_y - 14});
        centered.Print(TXT_WOL_PASSWORD,
                       {d_pass_x + (d_pass_w / 2), d_pass_y - 14});
        commands.FlagToRedraw();
      }
      game::TheMouse().Show();
      display = false;
    }

    //	Force mouse visible, as some beta testers report unexplicable
    // disappearing cursors.
    game::TheMouse().ForceShow();
    //	Be nice to other apps.
    engine::platform::SleepMs(50);

    /*
    **	Get user input.
    */
    TheGameState().tab_key_pressed() = false;
    const engine::window::KeyNumber input = commands.Input(view).key;

    /*
    **	The first time through the processing loop, set the edit
    **	gadget to have the focus. The
    **	focus must be set here since the gadget list has changed
    **	and this change will cause any previous focus setting to be
    **	cleared by the input processing routine.
    */
    if (firsttime) {
      firsttime = false;
      NameEdit.SetFocus();
      NameEdit.FlagToRedraw();
    }

    //		/*
    //		**	If the <RETURN> key was pressed, then default to the
    // appropriate
    //		**	action button according to the style of this dialog box.
    //		*/
    /*		if (input == KN_RETURN || input == (kButtonConnect|KN_BUTTON)) {
                            ToggleClass * toggle = NULL;
                            input = (KeyNumber)(kButtonConnect|KN_BUTTON);
                            CancelBtn.Turn_Off();
                            toggle =
       (ToggleClass*)commands.Extract(kButtonConnect); if (toggle !=
       NULL) { toggle->Turn_On(); toggle->IsPressed = true;
                            }

                            HideMouse();
                            commands.DrawAll(true);
                            ShowMouse();
                    }
    */
    /*
    **	Process input.
    */
    //		if( input )
    //			debugprint( "input: %i\n", input );

    if (TheGameState().tab_key_pressed()) {
      if (NameEdit.HasFocus()) {
        PassEdit.SetFocus();
      } else {
        NameEdit.SetFocus();
      }
      NameEdit.FlagToRedraw();
      PassEdit.FlagToRedraw();
    }

    switch (static_cast<int>(input)) {
      /*
      ** ESC/Cancel: break
      */
      case engine::window::KN_ESC:
      case engine::window::ButtonKey(kButtonCancel):
        iReturn = 0;
        process = false;
        break;

      case engine::window::KN_RETURN:
      case engine::window::ButtonKey(kEditboxName):
      case engine::window::ButtonKey(kEditboxPass):
      case engine::window::ButtonKey(kButtonConnect): {
        if (std::string_view(szNameBuffer).empty()) {
          WWMessageBox().Process(TXT_WOL_MISSINGNAME);
          firsttime = true;  //	Bloody hack.
          NameEdit.SetFocus();
          game::TheKeyboard().Clear();
          display = true;
          break;
        }
        if (std::string_view(szPassBuffer).empty()) {
          WWMessageBox().Process(TXT_WOL_MISSINGPASSWORD);
          firsttime = true;  //	Bloody hack.
          PassEdit.SetFocus();
          game::TheKeyboard().Clear();
          display = true;
          break;
        }

        //	If we have not done RequestServerList() yet, do it now.
        if (!pWO->pChatSink->pServer) {
          bool bBreak = false;
          const HRESULT hRes = pWO->GetChatServer();
          switch (hRes) {
            case E_FAIL:
              bBreak = true;
              WWMessageBox().Process(TXT_WOL_CANTCONNECT);
              firsttime = true;  //	Bloody hack.
              NameEdit.SetFocus();
              game::TheKeyboard().Clear();
              display = true;
              break;
            case USERCANCELLED:
              bBreak = true;
              WWMessageBox().Process(TXT_WOL_LOGINCANCEL);
              firsttime = true;  //	Bloody hack.
              NameEdit.SetFocus();
              game::TheKeyboard().Clear();
              display = true;
              break;
            case PATCHAVOIDED:
              bBreak = true;
              firsttime = true;  //	Bloody hack.
              NameEdit.SetFocus();
              game::TheKeyboard().Clear();
              display = true;
              break;
            case PATCHDOWNLOADED:
              bBreak = true;
              process = false;
              iReturn = -1;
              break;
            default:
              break;
          }
          if (bBreak) {
            break;
          }
        }

        //	RequestConnection()...
        const HRESULT hRes = pWO->AttemptLogin(szNameBuffer, szPassBuffer,
                                               PassEdit.bClearOnNextSetFocus);
        if (hRes == S_OK) {
          if (SaveCheckBox.IsOn && !bSaveNick(pWO, szNameBuffer, szPassBuffer,
                                              PassEdit.bClearOnNextSetFocus)) {
            //	Nick/pass save failed.
            WWMessageBox().Process(TXT_WOL_CANTSAVENICK);
          }
          process = false;
        } else {
          switch (hRes) {
            case USERCANCELLED:
              WWMessageBox().Process(TXT_WOL_LOGINCANCEL);
              break;
            case CHAT_E_TIMEOUT:
              WWMessageBox().Process(TXT_WOL_TIMEOUT);
              break;
            case CHAT_E_BADPASS:
              WWMessageBox().Process(TXT_WOL_BADPASS);
              break;
            case CHAT_E_NICKINUSE:
              WWMessageBox().Process(TXT_WOL_NICKINUSE);
              break;
            case CHAT_E_CON_ERROR:
              //	This error value I pass back myself, when the emergency
              // timeout is hit.
              WWMessageBox().Process(TXT_WOL_TIMEOUT);
              break;
            default:
              break;
          }
          firsttime = true;  //	Bloody hack.
          NameEdit.SetFocus();
          game::TheKeyboard().Clear();
          display = true;
        }
        break;
      }

      case engine::window::ButtonKey(kListboxNicks):
        base::SafeCopy(szNameBuffer,
                       NickList.Get_Item(NickList.Current_Index()));
        base::SafeCopy(szPassBuffer, NickList.Get_Item_ExtraDataString(
                                         NickList.Current_Index()));
        NameEdit.FlagToRedraw();
        PassEdit.FlagToRedraw();
        //	Because the password is mangled, if the user begins to edit it
        // now, we clear it. 	Otherwise we could get a half-mangled,
        // half-unmangled password field. 	PassEdit.bClearOnNextSetFocus
        // also acts as a flag telling us whether or not the 	password field
        // is mangled or not.
        PassEdit.bClearOnNextSetFocus = true;
        //				display = true;
        break;

      case engine::window::ButtonKey(kButtonSavecheck):
        break;

      case engine::window::ButtonKey(kButtonDelete):
        if (NickList.Count() > 0) {
          DeleteNick(pWO, NickList.Current_Index() + 1);
          NickList.Remove_Item(NickList.Current_Index());
          NickList.FlagToRedraw();
          if (NickList.Count() == 0) {
            DeleteBtn.Disable();
            DeleteBtn.FlagToRedraw();
          }
        }
        break;

      default:
        break;
    }
  }

  return iReturn;
}

//***********************************************************************************************
bool ReadSavedNicks(const WolapiObject* pWO, IconListClass& NickList,
                    std::span<char> szNameBuffer,
                    std::span<char> szPassBuffer) {
  //	Read saved nickname/passwords from the registry.
  //	Set up the list of nick/passwords.
  //	Copy the first nick into the nick/password edits.

  //	Returns true if edits are set with a default nick/pass because a nick
  // was found.

  LPCSTR szNick = nullptr;
  LPCSTR szPass = nullptr;
  bool bReturn = false;

  for (int i = 1; i != 3; i++) {
    if ((pWO->pChat->GetNick(i, &szNick, &szPass) == S_OK) && (*szNick)) {
      NickList.Add_Item(szNick, nullptr, nullptr, ICON_SHAPE, szPass);
      if (i == 1) {
        base::SafeCopy(szNameBuffer, szNick);
        base::SafeCopy(szPassBuffer, szPass);
        bReturn = true;
      }
    }
  }
  return bReturn;
}

//***********************************************************************************************
bool bSaveNick(const WolapiObject* pWO, const char* szNickToSave,
               const char* szPassToSave, bool bPassIsMangled) {
  //	Saves specified nick and password in the registry, using SetNick.
  //	Returns false if nick can't be saved.

  //	If slot 1 empty, use slot 1.
  //	Else push nick 1 down to second slot and save new nick in slot 1, unless
  //	nick 1 name matches new entry.

  LPCSTR szNick = nullptr;
  LPCSTR szPass = nullptr;
  bool bPushSlot1 = true;

  switch (pWO->pChat->GetNick(1, &szNick, &szPass)) {
    case E_FAIL:
      //	Assume that this is because there is no registry entry. We can
      // use this slot.
      bPushSlot1 = false;
      break;
    case S_OK:
      // We can use this blank slot, or this slot as the name is the same.
      if (*szNick == 0 || std::string_view(szNick) == szNickToSave) {
        bPushSlot1 = false;
      }
      break;
    default:
      break;
  }

  if (bPushSlot1) {
    //	Move nick in slot 1 to slot 2.
    pWO->pChat->SetNick(2, szNick, szPass, 0);  //	(Already mangled.)
  }

  //	Save new nick in slot 1.
  return (pWO->pChat->SetNick(1, szNickToSave, szPassToSave,
                              bPassIsMangled ? 0 : 1) == S_OK);

  /*
          int iSlot;
          bool bStop = false;
          for( iSlot = 1; iSlot != 3; iSlot++ )
          {
                  switch( pWO->pChat->GetNick( iSlot, &szNick, &szPass ) )
                  {
                  case E_FAIL:
                          //	Assume that this is because there is no registry
     entry. We can use this slot. bStop = true; break; case S_OK: if( *szNick ==
     0 ) bStop = true;		//	We can use this blank slot. else if(
     strcmp( szNick, szNickToSave ) == 0 ) bStop = true;	//	We can
     use this slot as the name is the same. break;
                  }
                  if( bStop )
                          break;
          }
          if( iSlot == 3 )
          {
                  //	No open slots were found.
                  //	Get nick 1.
                  pWO->pChat->GetNick( 1, &szNick, &szPass );
                  //	Save as nick 2.
                  pWO->pChat->SetNick( 2, szNick, szPass, false );	//
     (Already mangled.)
                  //	Save new nick 1.
                  return ( pWO->pChat->SetNick( 1, szNickToSave, szPassToSave,
     !bPassIsMangled ) == S_OK );
          }
          else
          {
                  //	iSlot points to an open slot.
                  return ( pWO->pChat->SetNick( iSlot, szNickToSave,
     szPassToSave, !bPassIsMangled ) == S_OK );
          }
  */
}

//***********************************************************************************************
void DeleteNick(const WolapiObject* pWO, int iOneBasedEntryToDelete) {
  //	Delete a nick from the registry via wolapi SetNick.
  //	If nick to delete is in position one, and there is a second nick, move
  // the second nick into position one.
  if (iOneBasedEntryToDelete == 1) {
    //	Check for nick 2.
    LPCSTR szNick = nullptr;
    LPCSTR szPass = nullptr;
    if (pWO->pChat->GetNick(2, &szNick, &szPass) == S_OK && *szNick != 0) {
      //	Copy nick in slot 2 to slot 1.
      pWO->pChat->SetNick(1, szNick, szPass, 0);  //	(Already
                                                  // mangled.)
      //	Delete slot 2.
      const HRESULT hRes = pWO->pChat->SetNick(2, "", "", 0);
      DebugChatDef(hRes);
    } else {
      //	No second nick.
      const HRESULT hRes = pWO->pChat->SetNick(1, "", "", 0);
      DebugChatDef(hRes);
    }
  } else {
    const HRESULT hRes = pWO->pChat->SetNick(2, "", "", 0);
    DebugChatDef(hRes);
  }
}

}  // namespace ra
