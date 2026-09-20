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

//	Wol_Main.cpp - Bottom level wolapi-stuff function.
//	ajw 07/16/98

#include "ra/wol_main.h"

#include "port/win32/win32_com.h"
#include "port/win32/win32_types.h"
#include "ra/defines.h"
#include "ra/installation.h"
#include "ra/network.h"
#include "ra/wol_gsup.h"
#include "ra/wolapiob.h"
#include "ra/wolstrng.h"

static bool ReregisterWolapiDLL();
static void HandleDLLFail();

#include "port/bytes_of.h"
#include "port/platform.h"
#include "port/sleep.h"
#include "port/win32/win32_registry.h"
#include "port/win32/win32_system.h"
#include "ra/externs.h"
#include "ra/msgbox.h"
#include "ra/theme.h"

//***********************************************************************************************
//	The first time through, pWolapi is NULL thus wolapi gets
// set up.
// WOL_Login_Dialog presents the user 	with the login dialog and attempts to
// log us on to the server. If the user continues on all the 	way to a game
// start, we will drop out of here with pWolapi still pointing to
// a valid WolapiObject, 	and with pWolapi's
// iLobbyReturnAfterGame set to the number of the lobby to return to
// automatically 	after the game ends. Init() automatically brings us here
// if pWolapi is non-null.
//***********************************************************************************************
int WOL_Main() {
  //	Return values:
  //		0 = cancel
  //		1 = start game
  //		-1 = patch downloaded, shut down app
  int iReturn = 0;

  if (TheNetwork().wolapi()) {
    //	We have returned from a game started through ww online.

    //	Start theme up again.
    Theme.Play_Song(THEME_INTRO);

    //	Verify that we are still connected. If we aren't, kill WolapiObject and
    // start over. 	(This will likely occur during the game, if connection
    // is lost. Ensure that it is done here.)
    TheNetwork()
        .wolapi()
        ->pChat->PumpMessages();  //	Causes OnNetStatus() call if no longer
                                  // connected.
    if (TheNetwork().wolapi()->bConnectionDown) {
      // debugprint( "Re-entering WOL_Main(),
      // pWolapi->bConnectionDown is true. Deleting old
      // WolapiObject...\n" );
      WWMessageBox().Process(TXT_WOL_WOLAPIREINIT);
      //	Kill wolapi.
      TheNetwork().wolapi()->UnsetupCOMStuff();
      delete TheNetwork().wolapi();
      TheNetwork().wolapi() = nullptr;
    }
  }

  if (!TheNetwork().wolapi()) {
    //	Start up wolapi.
    TheNetwork().wolapi() = new WolapiObject;
    if (!TheNetwork().wolapi()->bSetupCOMStuff()) {
      //	Things are really bad if this happens. A COM call failed.

      //	We first assume that their wolapi.dll failed to register during
      // wolsetup.exe, part of the patch process. 	This happens if they
      // have an outdated oleaut32.dll, such as the one that comes with original
      // version of Windows 95.

      //			debugprint( "bSetupCOMStuff failed. Attemping to
      // reregister wolapi.dll...\n" ); 	Attempt to re-register
      // wolapi.dll...
      if (ReregisterWolapiDLL()) {
        if (!TheNetwork().wolapi()->bSetupCOMStuff()) {
          //	Still failed after reregistering seemed to work.
          HandleDLLFail();
          return 0;
        }
      } else {
        HandleDLLFail();
        return 0;
      }
    }
    TheNetwork().wolapi()->PrepareButtonsAndIcons();
    //	Undocumented hack needed for patch downloading, per Neal.
    TheNetwork().wolapi()->pChat->SetAttributeValue("RegPath",
                                                    Game_Registry_Key());
    //	(Not that anything's really "documented".)
  }

  TheNetwork().wolapi()->bInGame = false;

  const int iLoginResult = WOL_Login_Dialog(TheNetwork().wolapi());
  if (iLoginResult == 1) {
    TheNetwork().wolapi()->SetOptionDefaults();
    bool bKeepGoing = true;
    while (bKeepGoing) {
      bool bCreator = false;  //	True when this player made the channel.
      switch (WOL_Chat_Dialog(TheNetwork().wolapi())) {
        case -1:
          bKeepGoing = false;
          break;
        case 1:
          //	User created game channel.
          bCreator = true;
          break;
        case 2:
          //	User joined game channel.
          bCreator = false;
          break;
        default:
          break;
      }
      if (bKeepGoing) {
        WOL_GameSetupDialog GSupDlg(TheNetwork().wolapi(), bCreator);
        switch (GSupDlg.Run()) {
          case RESULT_WOLGSUP_LOGOUT:
            //	User logged out.
            bKeepGoing = false;
            break;
          case RESULT_WOLGSUP_BACKTOCHAT:
          case RESULT_WOLGSUP_HOSTLEFT:
          case RESULT_WOLGSUP_RULESMISMATCH:
            //	Return to chat.
            break;
          case RESULT_WOLGSUP_STARTGAMEHOST:
            //	Proceed with game.
            bKeepGoing = false;
            iReturn = 1;
            TheNetwork().wolapi()->bGameServer = true;
            break;
          case RESULT_WOLGSUP_STARTGAME:
            //	Proceed with game.
            bKeepGoing = false;
            iReturn = 1;
            TheNetwork().wolapi()->bGameServer = false;
            break;
          case RESULT_WOLGSUP_FATALERROR:
            //					debugprint(
            //"RESULT_WOLGSUP_FATALERROR from game setup dialog.\n" );
            // Fatal( "RESULT_WOLGSUP_FATALERROR from game setup dialog.\n" );
            if (TheNetwork().wolapi()->pChatSink->bConnected) {
              TheNetwork().wolapi()->Logout();
            }
            bKeepGoing = false;
            break;
          default:
            break;
        }
      }
    }
  }

  if (iReturn != 1) {
    //	Kill wolapi.
    TheNetwork().wolapi()->UnsetupCOMStuff();
    delete TheNetwork().wolapi();
    TheNetwork().wolapi() = nullptr;
  } else {
    TheNetwork().wolapi()->bInGame = true;
    TheNetwork().wolapi()->bConnectionDown = false;
  }

  if (iLoginResult == -1) {
    WWMessageBox().Process(TXT_WOL_DOWNLOADEXITWARNING);
    iReturn = -1;
  }

  return iReturn;
}

//***********************************************************************************************
bool ReregisterWolapiDLL() {
  //	Attempt to reregister wolapi.dll.
  //	Returns true if we think we succeeded.
  HKEY hKey = nullptr;
  char szInstallPath[port::kMaxPath];
  if (::RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Westwood\\WOLAPI", 0,
                     KEY_READ, &hKey) == ERROR_SUCCESS) {
    DWORD dwBufSize = port::kMaxPath;
    if (::RegQueryValueEx(hKey, "InstallPath", nullptr, nullptr,
                          port::BytesOf(szInstallPath),
                          &dwBufSize) == ERROR_SUCCESS) {
      WIN32_FIND_DATA wfd{};
      HANDLE handle = FindFirstFile(szInstallPath, &wfd);
      if (handle == INVALID_HANDLE_VALUE) {
        //	File is not there.
        FindClose(handle);
        ::RegCloseKey(hKey);
        return false;
      }
      //			debugprint( "Found dll -> %s\n", szInstallPath
      //); 	Get the DLL to register itself.
      HINSTANCE hLib = LoadLibrary(szInstallPath);
      if (!hLib) {
        //				debugprint( "LoadLibrary failed,
        // GetLastError is %i\n", GetLastError() );
        ::RegCloseKey(hKey);
        return false;
      }
      FARPROC lpDllRegisterFunction = GetProcAddress(hLib, "DllRegisterServer");
      if (!lpDllRegisterFunction) {
        ::RegCloseKey(hKey);
        return false;
      }
      if (lpDllRegisterFunction() != S_OK) {
        ::RegCloseKey(hKey);
        return false;
      }
      //	There is a bug in wolapi.dll that makes the following delay
      // necessary. 	Something about Neal's extra threads only getting
      // half-way set up before they get deleted. 	(The extra threads
      // shouldn't really be created in this case, anyway...)
      port::SleepMs(1000);
      FreeLibrary(hLib);
      FindClose(handle);
    } else {
      ::RegCloseKey(hKey);
      return false;
    }
    ::RegCloseKey(hKey);
  } else {
    return false;
  }
  return true;
}

//***********************************************************************************************
void HandleDLLFail() {
  //	The DLL failed to load. Either we failed to reregister it, or we think
  // we succeeded at this but it 	still is not working. Show an error
  // message and delete pWolapi.
  //
  //	ajw picked between "download IE3" and "call tech support" by finding
  //	oleaut32.dll in the Windows system directory and calling it out of date
  //	if it was under 232,720 bytes. There is no system directory to look in
  //	here and no oleaut32.dll to find, so the advice that does not name a
  //	Windows component is the only one that could ever be right.
  WWMessageBox().Process(TXT_WOL_DLLERROR_CALLUS);

  delete TheNetwork().wolapi();
  TheNetwork().wolapi() = nullptr;
}
