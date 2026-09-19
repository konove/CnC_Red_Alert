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

//	WolapiOb.cpp - Implementation of class WolapiObject.
//	ajw 07/10/98

#include "ra/wolapiob.h"

#include <cstdlib>
#include <span>

#include "absl/log/log.h"
#include "absl/strings/match.h"
#include "absl/strings/str_format.h"
#include "base/array.h"
#include "base/buffer.h"
#include "port/win32/win32_types.h"
#include "ra/conquer.h"
#include "ra/defines.h"
#include "ra/dialog.h"
#include "ra/dib.h"
#include "ra/globals.h"
#include "ra/iconlist.h"
#include "ra/installation.h"
#include "ra/text_ids.h"
#include "ra/type.h"
#include "ra/wolapi/chatdefs.h"
#include "ra/wolapi/wolapi.h"
#include "sdllib/keyboard.h"
#include "sdllib/wwstd.h"
#include "tech/mix_archive.h"
#include "tech/number_parse.h"
#include "tech/rgb.h"

#ifdef _WIN32
#include <winsock.h>
#else
#include <arpa/inet.h>
#endif

#include <SDL_video.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "absl/log/check.h"
#include "absl/strings/str_split.h"
#include "base/numeric.h"
#include "port/bytes_of.h"
#include "port/format.h"
#include "port/inet_text.h"
#include "port/platform.h"
#include "port/safe_string.h"
#include "port/sleep.h"
#include "port/win32/win32_com.h"
#include "port/win32/win32_registry.h"
#include "port/win32/win32_system.h"
#include "ra/audio.h"
#include "ra/config.h"
#include "ra/externs.h"
#include "ra/inline.h"
#include "ra/jshell.h"
#include "ra/msgbox.h"
#include "ra/palette.h"
#include "ra/rawolapi.h"
#include "ra/seditdlg.h"
#include "ra/statbtn.h"
#include "ra/tooltip.h"
#include "ra/wol_gsup.h"
#include "ra/wol_main.h"
#include "ra/wolstrng.h"
#include "sdllib/timer.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/ww_win.h"

static void HostNameFromGameChannelName(std::span<char> szNameToSet,
                                        const char* szChannelName);

namespace {

// MainWindow is the SDL window, not an HWND, so the Win32 ShowWindow and
// SetForegroundWindow calls the browser launch used cannot take it on any
// platform. SDL does the same job.
SDL_Window* GameWindow() { return static_cast<SDL_Window*>(MainWindow); }

void Restore_Game_Window() {
  SDL_RestoreWindow(GameWindow());
  SDL_RaiseWindow(GameWindow());
}

bool Game_Window_Has_Focus() {
  return (SDL_GetWindowFlags(GameWindow()) & SDL_WINDOW_INPUT_FOCUS) != 0;
}

}  // namespace

//***********************************************************************************************
WolapiObject::WolapiObject()
    :

      dwTimeNextWolapiPump(Get_Time_Ms() + WOLAPIPUMPWAIT) {
  *szMyName = 0;
  *szMyRecord = 0;
  *szMyRecordAM = 0;
  *szChannelListTitle = 0;
  *szChannelNameCurrent = 0;
  *szChannelReturnOnGameEnterFail = 0;

  *szLadderServerHost = 0;
  *szGameResServerHost1 = 0;
  *szGameResServerHost2 = 0;

  port::SafeCopy(base::At(DibIconInfos, kDibiconOwner).szFile, "dib_own.bmp");
  port::SafeCopy(base::At(DibIconInfos, kDibiconSquelch).szFile,
                 "dib_sqel.bmp");
  port::SafeCopy(base::At(DibIconInfos, kDibiconLatency).szFile, "latency.bmp");
  port::SafeCopy(base::At(DibIconInfos, kDibiconAccept).szFile, "dib_acpt.bmp");
  port::SafeCopy(base::At(DibIconInfos, kDibiconNotaccept).szFile,
                 "dib_acp2.bmp");
  port::SafeCopy(base::At(DibIconInfos, kDibiconUser).szFile, "dib_user.bmp");
  port::SafeCopy(base::At(DibIconInfos, kDibiconPrivate).szFile,
                 "privgame.bmp");
  port::SafeCopy(base::At(DibIconInfos, kDibiconTournament).szFile,
                 "tourgame.bmp");
  port::SafeCopy(base::At(DibIconInfos, kDibiconVoice).szFile, "voice.bmp");
  //	The name of the user's web browser. ajw found it by creating an empty
  //	.html file and asking ::FindExecutable what opened it -- "the 'correct'
  //	way to do this, but it's bloody stupid" -- which only means anything
  //	against the Windows registry of file associations. Leaving it empty is a
  //	state the code already handles: DoWebRegistration and the help links
  //	check it before trying to launch anything.
  *szWebBrowser = 0;
}

//***********************************************************************************************
WolapiObject::~WolapiObject() {
  DeleteSavedChat();

  //	The icons used to be GlobalAlloc handles that had to be unlocked and
  //	destroyed by hand. dib::Image owns its pixels, so they go with the
  //	structs that hold them.
  GameTypeInfos.clear();
  GameTypeInfos.clear();

  if (pChatSink) {
    UnsetupCOMStuff();
  }

  //	Delete buttons, etc., shared by dialogs.
  delete pShpBtnDiscon;
  delete pShpBtnLeave;
  delete pShpBtnRefresh;
  delete pShpBtnSquelch;
  delete pShpBtnBan;
  delete pShpBtnKick;
  delete pShpBtnFindpage;
  delete pShpBtnOptions;
  delete pShpBtnLadder;
  delete pShpBtnHelp;
  //	Delete shared tooltips.
  delete pTTipDiscon;
  delete pTTipLeave;
  delete pTTipRefresh;
  delete pTTipSquelch;
  delete pTTipBan;
  delete pTTipKick;
  delete pTTipFindpage;
  delete pTTipOptions;
  delete pTTipLadder;
  delete pTTipHelp;
}

//***********************************************************************************************
bool WolapiObject::bSetupCOMStuff() {
  //	debugprint( "++++Begin WolapiObject::bSetupCOMStuff\n" );


  //	Grab IChat, INetUtil, set up "sinks".
  //	debugprint( "CoCreateInstance\n" );
  CoCreateInstance(CLSID_Chat, nullptr, CLSCTX_INPROC_SERVER, IID_IChat,
                   ComOut(&pChat));
  if (!pChat) {
    return false;  //	Severe, essentially fatal.
  }
  CoCreateInstance(CLSID_NetUtil, nullptr, CLSCTX_INPROC_SERVER, IID_INetUtil,
                   ComOut(&pNetUtil));
  if (!pNetUtil) {
    return false;  //	Severe, essentially fatal.
  }

  //	Set up RAChatEventSink.
  pChatSink = new RAChatEventSink(this);
  pChatSink->AddRef();
  //	Set up RANetUtilEventSink.
  pNetUtilSink = new RANetUtilEventSink(this);
  pNetUtilSink->AddRef();

  //	If we could use ATL stuff, this would be different. (We'd use
  // AtlAdvise.)

  IConnectionPoint* pConnectionPoint = nullptr;
  IConnectionPointContainer* pContainer = nullptr;

  // Get a connection point from the chat class for the chatsink.
  //	debugprint( "QueryInterface\n" );
  HRESULT hRes =
      pChat->QueryInterface(IID_IConnectionPointContainer, ComOut(&pContainer));
  if (!SUCCEEDED(hRes)) {
    return false;  //	Severe, essentially fatal.
                   //	debugprint( "FindConnectionPoint\n" );
  }
  hRes = pContainer->FindConnectionPoint(IID_IChatEvent, &pConnectionPoint);
  if (!SUCCEEDED(hRes)) {
    return false;  //	Severe, essentially fatal.
  }
  //	Connect chat to chatsink.
  //	debugprint( "Advise. pChatSink = %i, pConnectionPoint = %i\n",
  // pChatSink, pConnectionPoint );
  hRes = pConnectionPoint->Advise(static_cast<IChatEvent*>(pChatSink),
                                  &dwChatAdvise);
  if (!SUCCEEDED(hRes)) {
    return false;  //	Severe, essentially fatal.
  }

  pContainer->Release();
  pConnectionPoint->Release();

  pConnectionPoint = nullptr;
  pContainer = nullptr;
  // Get a connection point from the netutil class for the netutilsink.
  //	debugprint( "QueryInterface\n" );
  hRes = pNetUtil->QueryInterface(IID_IConnectionPointContainer,
                                  ComOut(&pContainer));
  if (!SUCCEEDED(hRes)) {
    return false;  //	Severe, essentially fatal.
                   //	debugprint( "FindConnectionPoint\n" );
  }
  hRes = pContainer->FindConnectionPoint(IID_INetUtilEvent, &pConnectionPoint);
  if (!SUCCEEDED(hRes)) {
    return false;  //	Severe, essentially fatal.
  }
  //	Connect netutil to netutilsink.
  //	debugprint( "Advise. pChatSink = %i, pConnectionPoint = %i\n",
  // pChatSink, pConnectionPoint );
  hRes = pConnectionPoint->Advise(static_cast<INetUtilEvent*>(pNetUtilSink),
                                  &dwNetUtilAdvise);
  if (!SUCCEEDED(hRes)) {
    return false;  //	Severe, essentially fatal.
  }

  pContainer->Release();
  pConnectionPoint->Release();

  //	debugprint( "++++End WolapiObject::bSetupCOMStuff\n" );
  return true;
}

//***********************************************************************************************
void WolapiObject::UnsetupCOMStuff() {
  //	debugprint( "----Begin WolapiObject::UnsetupCOMStuff\n" );


  //	If we could use ATL stuff, this would be different. (We'd use
  // AtlUnadvise.)

  //	Unsetup RAChatEventSink and RANetUtilEventSink, release IChat.
  IConnectionPoint* pConnectionPoint = nullptr;
  IConnectionPointContainer* pContainer = nullptr;

  //	debugprint( "QueryInterface\n" );
  HRESULT hRes =
      pChat->QueryInterface(IID_IConnectionPointContainer, ComOut(&pContainer));
  LOG_IF(WARNING, FAILED(hRes)) << "WOL COM call failed";
  //	debugprint( "FindConnectionPoint\n" );
  hRes = pContainer->FindConnectionPoint(IID_IChatEvent, &pConnectionPoint);
  LOG_IF(WARNING, FAILED(hRes)) << "WOL COM call failed";
  //	debugprint( "Unadvise: %i\n", dwChatAdvise );
  pConnectionPoint->Unadvise(dwChatAdvise);

  pContainer->Release();
  pConnectionPoint->Release();

  pConnectionPoint = nullptr;
  pContainer = nullptr;
  //	debugprint( "QueryInterface\n" );
  hRes = pNetUtil->QueryInterface(IID_IConnectionPointContainer,
                                  ComOut(&pContainer));
  LOG_IF(WARNING, FAILED(hRes)) << "WOL COM call failed";
  //	debugprint( "FindConnectionPoint\n" );
  hRes = pContainer->FindConnectionPoint(IID_INetUtilEvent, &pConnectionPoint);
  LOG_IF(WARNING, FAILED(hRes)) << "WOL COM call failed";
  //	debugprint( "Unadvise: %i\n", dwNetUtilAdvise );
  pConnectionPoint->Unadvise(dwNetUtilAdvise);

  pContainer->Release();
  pConnectionPoint->Release();

  //	debugprint( "pChat->Release\n" );
  pChat->Release();

  //	debugprint( "pChatSink->Release\n" );
  pChatSink->Release();  //	This results in pChatSink deleting itself for
                         // us.
  pChatSink = nullptr;

  //	debugprint( "pNetUtil->Release\n" );
  pNetUtil->Release();

  //	debugprint( "pNetUtilSink->Release\n" );
  pNetUtilSink
      ->Release();  //	This results in pChatSink deleting itself for us.
  pNetUtilSink = nullptr;

  //	debugprint( "----End WolapiObject::UnsetupCOMStuff\n" );
}

//***********************************************************************************************
void WolapiObject::LinkToChatDlg(IconListClass* chat_list,
                                 IconListClass* channels_list,
                                 IconListClass* users_list,
                                 StaticButtonClass* static_users) {
  //	Called to initialize this before the chat dialog is shown.

  //	Set pointers to lists in dialog.
  this->pILChat = chat_list;
  this->pILChannels = channels_list;
  this->pILUsers = users_list;

  this->pStaticUsers = static_users;
}

//***********************************************************************************************
void WolapiObject::ClearListPtrs() {
  //	Called to clear list pointers when chat or gamesetup dialog goes away,
  // for safety.
  pILChat = nullptr;
  pILChannels = nullptr;
  pILUsers = nullptr;

  pILPlayers = nullptr;

  pStaticUsers = nullptr;
}

//***********************************************************************************************
void WolapiObject::LinkToGameDlg(IconListClass* pILDisc,
                                 IconListClass* players_list) {
  //	Called to initialize this before the gamesetup dialog is shown.

  //	Set pointers to lists in dialog.
  pILChat = pILDisc;
  this->pILPlayers = players_list;
}

//***********************************************************************************************
void WolapiObject::PrepareButtonsAndIcons() {
  //	Load shapes for buttons. Store images in this order: up, down, disabled.
  // pShpDiscon = LoadShpFile( "discon.shp" ); etc
  pShpDiscon = MixArchive::RetrieveData("discon.shp");
  pShpLeave = MixArchive::RetrieveData("leave.shp");
  pShpRefresh = MixArchive::RetrieveData("refresh.shp");
  pShpSquelch = MixArchive::RetrieveData("squelch.shp");
  pShpBan = MixArchive::RetrieveData("ban.shp");
  pShpKick = MixArchive::RetrieveData("kick.shp");
  pShpFindpage = MixArchive::RetrieveData("findpage.shp");
  pShpOptions = MixArchive::RetrieveData("ops.shp");
  pShpLadder = MixArchive::RetrieveData("ladder.shp");
  pShpHelp = MixArchive::RetrieveData("help.shp");

  //	Set up standard wol buttons, used by both main dialogs. Note hardcoded
  // ID values: must match values in dialog.
  const int iWolButtons_x = 34;
  const int iWolButtons_y = 20;
  const int iWolButtons_dx = 53;
  int xWolButton = iWolButtons_x;
  const int xTTip = 10;  //	Offset for tooltip.
  const int yTTip = -5;  //	Offset for tooltip.
  pShpBtnDiscon =
      new ShapeButtonClass(100, pShpDiscon, xWolButton, iWolButtons_y);
  pTTipDiscon = new ToolTipClass(pShpBtnDiscon, TXT_WOL_TTIP_DISCON,
                                 xWolButton + xTTip, iWolButtons_y + yTTip);
  xWolButton += iWolButtons_dx;
  pShpBtnLeave =
      new ShapeButtonClass(101, pShpLeave, xWolButton, iWolButtons_y);
  pTTipLeave = new ToolTipClass(pShpBtnLeave, TXT_WOL_TTIP_LEAVE,
                                xWolButton + xTTip, iWolButtons_y + yTTip);
  xWolButton += iWolButtons_dx;
  pShpBtnRefresh =
      new ShapeButtonClass(102, pShpRefresh, xWolButton, iWolButtons_y);
  pTTipRefresh = new ToolTipClass(pShpBtnRefresh, TXT_WOL_TTIP_REFRESH,
                                  xWolButton + xTTip, iWolButtons_y + yTTip);
  xWolButton += iWolButtons_dx;
  pShpBtnSquelch =
      new ShapeButtonClass(103, pShpSquelch, xWolButton, iWolButtons_y);
  pTTipSquelch = new ToolTipClass(pShpBtnSquelch, TXT_WOL_TTIP_SQUELCH,
                                  xWolButton + xTTip, iWolButtons_y + yTTip);
  xWolButton += iWolButtons_dx;
  pShpBtnBan = new ShapeButtonClass(104, pShpBan, xWolButton, iWolButtons_y);
  pTTipBan = new ToolTipClass(pShpBtnBan, TXT_WOL_TTIP_BAN, xWolButton + xTTip,
                              iWolButtons_y + yTTip);
  xWolButton += iWolButtons_dx;
  pShpBtnKick = new ShapeButtonClass(105, pShpKick, xWolButton, iWolButtons_y);
  pTTipKick = new ToolTipClass(pShpBtnKick, TXT_WOL_TTIP_KICK,
                               xWolButton + xTTip, iWolButtons_y + yTTip);
  xWolButton += iWolButtons_dx;
  pShpBtnFindpage =
      new ShapeButtonClass(106, pShpFindpage, xWolButton, iWolButtons_y);
  pTTipFindpage = new ToolTipClass(pShpBtnFindpage, TXT_WOL_TTIP_FINDPAGE,
                                   xWolButton + xTTip, iWolButtons_y + yTTip);
  xWolButton = 452;
  pShpBtnOptions =
      new ShapeButtonClass(107, pShpOptions, xWolButton, iWolButtons_y);
  pTTipOptions =
      new ToolTipClass(pShpBtnOptions, TXT_WOL_TTIP_OPTIONS, xWolButton + xTTip,
                       iWolButtons_y + yTTip, true);
  xWolButton += iWolButtons_dx;
  pShpBtnLadder =
      new ShapeButtonClass(108, pShpLadder, xWolButton, iWolButtons_y);
  pTTipLadder =
      new ToolTipClass(pShpBtnLadder, TXT_WOL_TTIP_LADDER, xWolButton + xTTip,
                       iWolButtons_y + yTTip, true);
  xWolButton += iWolButtons_dx;
  pShpBtnHelp = new ShapeButtonClass(109, pShpHelp, xWolButton, iWolButtons_y);
  pTTipHelp = new ToolTipClass(pShpBtnHelp, TXT_WOL_TTIP_HELP,
                               xWolButton + xTTip, iWolButtons_y + yTTip, true);

  //	Load standard hard-coded icons.
  const std::array<dib::Color, dib::kPaletteSize> Palette =
      CurrentScreenPalette();

  for (auto& IconInfo : DibIconInfos) {
    const auto file_bytes = MixArchive::RetrieveData(IconInfo.szFile);
    if (file_bytes.empty()) {
      // debugprint( "Couldn't find %s in mix.\n", IconInfo.szFile );
      continue;
    }
    IconInfo.Icon = dib::Image::FromBmp(base::UnsignedBytes(file_bytes));
    if (IconInfo.Icon.has_value()) {
      dib::RemapToPalette(*IconInfo.Icon, Palette);
    }
    //		else
    //			debugprint( "Not a bitmap we can draw!\n" );
  }

  const std::optional<dib::Image>& LatencyIcon =
      base::At(DibIconInfos, kDibiconLatency).Icon;
  iLatencyIconWidth = LatencyIcon.has_value() ? LatencyIcon->Width() : 0;
  fLatencyToIconWidth = static_cast<float>(iLatencyIconWidth) / 1000;

  //	All of the following is for the list of game icons...

  //	Load game icons from the wol api.
  LPCSTR szSkus = nullptr;
  if (pChat->GetGametypeList(&szSkus) == S_OK) {
    const std::vector<std::string_view> skus =
        absl::StrSplit(szSkus, ',', absl::SkipEmpty());
    //	There are actually 2 additional game types available in wolapi - 0 (ws
    // icon) and -1 (wwonline icon).
    nGameTypeInfos = static_cast<unsigned int>(skus.size()) + 2;
    //	Create structs to hold infos.
    //		debugprint( "Creating %i gametypeinfos\n", nGameTypeInfos );
    GameTypeInfos.resize(base::ToSize(nGameTypeInfos));
    int iMyIndex = 0;
    for (const std::string_view sku : skus) {
      GetGameTypeInfo(tech::ParseInteger<int>(sku).value_or(0),
                      GameTypeInfos.at(base::ToSize(iMyIndex)), Palette);
      iMyIndex++;
    }
    //	Get the two extra game type infos...
    GetGameTypeInfo(-1, GameTypeInfos.at(base::ToSize(iMyIndex++)), Palette);
    GetGameTypeInfo(0, GameTypeInfos.at(base::ToSize(iMyIndex++)), Palette);
  }
  //	else
  //		debugprint( "GetGametypeList() failed.\n" );

  //	Load icons that we'll need to represent Red Alert GameKinds.
  //	These are available in wolapi at their old sku number locations.
  GetGameTypeInfo(2, base::At(OldRAGameTypeInfos, 0), Palette);  //	RA
  GetGameTypeInfo(3, base::At(OldRAGameTypeInfos, 1), Palette);  //	CS
  GetGameTypeInfo(4, base::At(OldRAGameTypeInfos, 2), Palette);  //	AM
}

//***********************************************************************************************
void WolapiObject::GetGameTypeInfo(int iGameType,
                                   WOL_GAMETYPEINFO& GameTypeInfo,
                                   std::span<const dib::Color> Palette) const {
  unsigned char* pVirtualFile = nullptr;
  int iFileLength = 0;
  //	debugprint( "GetGametypeInfo, type %i\n", iGameType );
  LPCSTR szName = nullptr;
  LPCSTR szURL = nullptr;
  pChat->GetGametypeInfo(static_cast<unsigned int>(iGameType), 12, &pVirtualFile, &iFileLength, &szName,
                         &szURL);
  GameTypeInfo.iGameType = iGameType;
  port::SafeCopy(GameTypeInfo.szName, szName != nullptr ? szName : "");
  port::SafeCopy(GameTypeInfo.szURL, szURL != nullptr ? szURL : "");

  //	The icon arrives as the bytes of a .bmp file. FromBmp turns down
  //	anything that is not 8-bit and uncompressed, which is the check that
  //	used to be a separate look at biBitCount.
  if (pVirtualFile == nullptr || iFileLength <= 0) {
    return;
  }
  // WOLAPI returns the bitmap allocation together with its byte length.
  // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
  const std::span<const unsigned char> bitmap(
      pVirtualFile, static_cast<std::size_t>(iFileLength));
  GameTypeInfo.Icon = dib::Image::FromBmp(bitmap);
  if (!GameTypeInfo.Icon.has_value()) {
    return;  //	Load failed. Should not ever happen.
  }

  //	Remap colors...
  dib::RemapToPalette(*GameTypeInfo.Icon, Palette);
}

//***********************************************************************************************
void* WolapiObject::IconForGameType(int iGameType) {
  //	Returns a GameTypeInfos entry by gametype, instead of our (potentially
  // arbitrary) index. 	Returns NULL if type not found in list, which will of
  // course never happen...
  for (unsigned int i = 0; i != nGameTypeInfos; i++) {
    if (GameTypeInfos.at(base::ToSize(i)).iGameType == iGameType) {
      return IconPointer(GameTypeInfos.at(base::ToSize(i)));
    }
  }
  return nullptr;
}

//***********************************************************************************************
const char* WolapiObject::NameOfGameType(int iGameType) const {
  //	Returns the name of a sku by gametype, instead of our (potentially
  // arbitrary) index. 	Returns NULL if type not found in list, which will of
  // course never happen...
  for (unsigned int i = 0; i != nGameTypeInfos; i++) {
    if (GameTypeInfos.at(base::ToSize(i)).iGameType == iGameType) {
      return GameTypeInfos.at(base::ToSize(i)).szName;
    }
  }
  return nullptr;
}

//***********************************************************************************************
const char* WolapiObject::URLForGameType(int iGameType) const {
  //	Returns NULL if type not found in list, which will of course never
  // happen...
  for (unsigned int i = 0; i != nGameTypeInfos; i++) {
    if (GameTypeInfos.at(base::ToSize(i)).iGameType == iGameType) {
      return GameTypeInfos.at(base::ToSize(i)).szURL;
    }
  }
  return nullptr;
}

//***********************************************************************************************
void WolapiObject::PrintMessage(
    const char* szText, PlayerColorType iColorRemap /* = PCOLOR_NONE */) {
  if (pILChat) {
    WOL_PrintMessage(*pILChat, szText, iColorRemap);
  }
}

//***********************************************************************************************
void WolapiObject::PrintMessage(const char* szText,
                                RemapControlType* pColorRemap) {
  if (pILChat) {
    WOL_PrintMessage(*pILChat, szText, pColorRemap);
  }
}

//***********************************************************************************************
HRESULT WolapiObject::GetChatServer() {
  //	Calls RequestServerList() to get a chat server ready for us to login to.
  //	Returns S_OK and sets pChatSink->pServer if successful.
  WWMessageBox().Process(TXT_WOL_CONNECTING, TXT_NONE);

  // if( !( ::GetAsyncKeyState( VK_SHIFT ) & 0x8000 ) )			//
  // ajw - allow use of test servers
  //{

  //	Request chat server address from server server.
  pChatSink->bRequestServerListWait = true;
  //	debugprint( "Calling RequestServerList...\n" );
  if (!SUCCEEDED(pChat->RequestServerList(kGameSku, kGameVersion, "unused",
                                          "unused", 5))) {
    //		debugprint( "RequestServerList call failed\n" );
    return E_FAIL;
  }
  const DWORD dwTimeLimit = Get_Time_Ms();  //	ajw My own extra timeout at one
                                            // minute, in case wolapi chokes.
  //	debugprint( "Called RequestServerList...\n" );
  DWORD dwTimeNextPump = Get_Time_Ms() + PUMPSLEEPDURATION;
  Keyboard->Clear();  //	Set up for escape key checking.
  bool bCancel = false;
  hresPatchResults = 0;
  while (pChatSink->bRequestServerListWait &&
         Get_Time_Ms() - dwTimeLimit < 60000) {
    while (Get_Time_Ms() < dwTimeNextPump) {
      ServiceRealTime();
      if (KeyboardClass::Down(KN_ESC)) {
        bCancel = true;
        break;
      }
    }
    //		debugprint( "PumpMessages after RequestServerList...\n" );
    pChat->PumpMessages();
    dwTimeNextPump = Get_Time_Ms() + PUMPSLEEPDURATION;
    //        port::SleepMs( PUMPSLEEPDURATION );	//	Can't do because
    //        we want to ServiceRealTime()
    //	If an "update list" of patches has been received, instead of a server
    // list, this flag will have been set 	for us describing the results.
    // We'll either cancel log in or trigger game exit.
    if (hresPatchResults) {
      pChatSink->bRequestServerListWait = false;
      return hresPatchResults;
    }
  }
  //	debugprint( "RequestServerList wait finished\n" );
  if (bCancel) {
    Keyboard->Clear();
    return USERCANCELLED;
  }
  if (pChatSink->pServer) {
    return S_OK;
  }
  return E_FAIL;

  /*
  }
  else
  {
          //	Test using local server on LAN.

          //	Bypass RequestServerList, as it is unnecessary and may not be
  possible if serverserver can't be reached.
          //	Set SKU manually because normally RequestServerList does this
  for you. pChat->SetProductSKU( kGameSku ); if( pChatSink->pServer ) delete
  pChatSink->pServer; pChatSink->pServer = new Server; if( !(
  ::GetAsyncKeyState( VK_CONTROL ) & 0x8000 ) ) port::SafeCopy(
  (char*)pChatSink->pServer->conndata, "TCP;irc.westwood.com;9000" ); else
                  //	Control key down as well.
                  port::SafeCopy( (char*)pChatSink->pServer->conndata,
  "TCP;10.2.20.28;4000" ); port::SafeCopy( (char*)pChatSink->pServer->connlabel,
  "IRC"
  ); port::SafeCopy( (char*)pChatSink->pServer->name, "Chat"); return S_OK;
  }
  */
}

//***********************************************************************************************
HRESULT WolapiObject::AttemptLogin(const char* szName, const char* szPass,
                                   bool bPassIsMangled) {
  //	If RequestConnection() succeeds, sets pChatSink->bConnected true and
  // returns S_OK. 	Else returns RequestConnection() error result.
  WWMessageBox().Process(TXT_WOL_ATTEMPTLOGIN, TXT_NONE);

  //	debugprint( "~1\n" );
  port::SafeCopy(WolTextBuffer(pChatSink->pServer->login), szName);
  port::SafeCopy(WolTextBuffer(pChatSink->pServer->password), szPass);

  /*
  //	debugprint( "RequestConnection with:\n%s,%s,%s,%s,%s - %s\n",
                                          pChatSink->pServer->name,
                                          pChatSink->pServer->connlabel,
                                          pChatSink->pServer->conndata,
                                          pChatSink->pServer->login,
                                          pChatSink->pServer->password,
                                          bPassIsMangled ? "(mangled)" :
  "(unmangled)" );
  */
  pChatSink->bRequestConnectionWait = true;
  pChatSink->hresRequestConnectionError = 0;

  if (!SUCCEEDED(pChat->RequestConnection(pChatSink->pServer, 15,
                                          bPassIsMangled ? 0 : 1))) {
    //		debugprint( "RequestConnection call failed\n" );
    return CHAT_E_CON_ERROR;
  }

  const DWORD dwTimeStart = Get_Time_Ms();
  DWORD dwTimeNextPump = Get_Time_Ms() + PUMPSLEEPDURATION;
  Keyboard->Clear();  //	Set up for escape key checking.
  bool bCancel = false;
  while (pChatSink->bRequestConnectionWait &&
         Get_Time_Ms() - dwTimeStart < EMERGENCY_TIMEOUT) {
    while (Get_Time_Ms() < dwTimeNextPump) {
      ServiceRealTime();
      if (KeyboardClass::Down(KN_ESC)) {
        bCancel = true;
        break;
      }
    }
    if (bCancel) {
      break;
    }
    pChat->PumpMessages();
    dwTimeNextPump = Get_Time_Ms() + PUMPSLEEPDURATION;
    //        Sleep( PUMPSLEEPDURATION );	//	Can't do because we want
    //        to ServiceRealTime()
  }
  if (bCancel) {
    Keyboard->Clear();
    return USERCANCELLED;
  }
  if (pChatSink->bRequestConnectionWait) {
    return CHAT_E_CON_ERROR;
  }

  if (pChatSink->bConnected) {
    port::SafeCopy(szMyName, szName);
    port::SafeCopy(szMyRecord, szName);
    port::SafeCopy(szMyRecordAM, szName);
    return S_OK;
  }
  return pChatSink->hresRequestConnectionError;
}

//***********************************************************************************************
bool WolapiObject::bLoggedIn() const { return pChatSink->bConnected; }

//***********************************************************************************************
void WolapiObject::Logout() {
  //	Requests logout from wolapi. Doesn't return any error values, as what we
  // would do if it 	failed - force the user to stay connected?

  if (bSelfDestruct) {
    WWMessageBox().Process(TXT_WOL_ERRORLOGOUT, TXT_NONE);
  } else {
    WWMessageBox().Process(TXT_WOL_ATTEMPTLOGOUT, TXT_NONE);
  }

  //	debugprint( "RequestLogout()\n" );

  pChatSink->bRequestLogoutWait = true;

  if (!SUCCEEDED(pChat->RequestLogout())) {
    //		debugprint( "RequestLogout() call failed\n" );
  }

  const DWORD dwTimePatience =
      Get_Time_Ms();  //	After 5 seconds we run out of patience and bail.
  DWORD dwTimeNextPump = Get_Time_Ms() + PUMPSLEEPDURATION;
  while (pChatSink->bRequestLogoutWait &&
         Get_Time_Ms() - dwTimePatience < 5000) {
    while (Get_Time_Ms() < dwTimeNextPump) {
      ServiceRealTime();
    }
    pChat->PumpMessages();
    dwTimeNextPump = Get_Time_Ms() + PUMPSLEEPDURATION;
  }

  pChatSink->bConnected = false;
  *szMyName = 0;

  PlaySoundEffect(WOLSOUND_LOGOUT);
}

//***********************************************************************************************
bool WolapiObject::UpdateChannels(int iChannelType, CHANNELFILTER ChannelFilter,
                                  bool bAutoping) {
  //	This is now non-modal.
  //	Sends off a request for a new channels list.

  //	//	Returns false upon total failure.
  //	WWMessageBox().Process( TXT_WOL_WAIT, TXT_NONE );

  //	pChatSink->bRequestChannelListWait = true;
  pChatSink->ChannelFilter = ChannelFilter;

  //	debugprint( "RequestChannelList(), iChannelType = %i, filter = %i\n",
  // iChannelType, ChannelFilter );
  if (!SUCCEEDED(pChat->RequestChannelList(iChannelType, bAutoping ? 1 : 0))) {
    //		debugprint( "RequestChannelList() call failed\n" );
    return false;
  }
  /*
          DWORD dwTimeStart = Get_Time_Ms();
          DWORD dwTimeNextPump = Get_Time_Ms() + PUMPSLEEPDURATION;
          while( pChatSink->bRequestChannelListWait && Get_Time_Ms() -
     dwTimeStart < EMERGENCY_TIMEOUT )
          {
                  while( Get_Time_Ms() < dwTimeNextPump )
                          ServiceRealTime();
                  pChat->PumpMessages();
                  dwTimeNextPump = Get_Time_Ms() + PUMPSLEEPDURATION;
          }

          if( pChatSink->bRequestChannelListWait )
                  return false;
  */
  LastUpdateChannelCallLevel = CurrentLevel;

  return true;
}

//***********************************************************************************************
void WolapiObject::OnChannelList() {
  //	The chatsink calls this when its OnChannelList() is called, and it has
  // remade its internal channel list. 	The question here is: should we display
  // the values now in the chatsink? 	As UpdateChannels() is now non-modal,
  // there is the danger that we have moved away from the place in 	the
  // channel heirarchy where we originally called RequestChannelList().
  // To help ensure we're getting this where we expect to get it, we check the
  // value of CurrentLevel against 	what it was when we called
  // UpdateChannels().
  if (CurrentLevel == LastUpdateChannelCallLevel) {
    ListChannels();
  }
}

//***********************************************************************************************
void WolapiObject::ListChannels() {
  //	Show pChatSink's pChannelList in pILChannels.
  //	The extra data ptr hidden in each list item will hold a void pointer to
  // the channel described.
  //	debugprint( "ListChannels(), pChannelList = %i\n",
  // pChatSink->pChannelList );

  static WOL_LEVEL LevelLastListed = WOL_LEVEL_INVALID;

  int iListViewIndex = 0;

  //	If redrawing the same list as before, preserve the view position.
  if (LevelLastListed == CurrentLevel) {
    iListViewIndex = pILChannels->Get_View_Index();
  } else {
    LevelLastListed = CurrentLevel;
  }

  pILChannels->Clear();
  switch (CurrentLevel) {
    case WOL_LEVEL_GAMESOFTYPE:
    case WOL_LEVEL_LOBBIES:
      pILChannels->Add_Item(TXT_WOL_CHANNEL_BACK, CHANNELTYPE_GAMES, nullptr,
                            ICON_SHAPE, CHANNELTYPE_GAMES);
      break;
    case WOL_LEVEL_INLOBBY:
      pILChannels->Add_Item(TXT_WOL_CHANNEL_BACK, CHANNELTYPE_LOBBIES, nullptr,
                            ICON_SHAPE, CHANNELTYPE_LOBBIES);
      break;
    case WOL_LEVEL::WOL_LEVEL_TOP:
    case WOL_LEVEL::WOL_LEVEL_OFFICIALCHAT:
    case WOL_LEVEL::WOL_LEVEL_USERCHAT:
    case WOL_LEVEL::WOL_LEVEL_INCHATCHANNEL:
    case WOL_LEVEL::WOL_LEVEL_GAMES:
    case WOL_LEVEL::WOL_LEVEL_INGAMECHANNEL:
    case WOL_LEVEL::WOL_LEVEL_INVALID:
    default:
      pILChannels->Add_Item(TXT_WOL_CHANNEL_TOP, CHANNELTYPE_TOP, nullptr,
                            ICON_SHAPE, CHANNELTYPE_TOP);
      break;
  }

  Channel* pChannel = pChatSink->pChannelList;
  while (pChannel) {
    if (pChannel->type == 0) {
      //	Show chat channel.
      const int iLobby = iChannelLobbyNumber(WolText(pChannel->name));
      if (iLobby == -1) {
        //	Regular chat channel.
        const std::string show = absl::StrFormat(
            "%s\t%-3u", WolText(pChannel->name), pChannel->currentUsers);
        char szHelp[200];
        Format_Runtime_Text(szHelp, sizeof(szHelp), TXT_WOL_TTIP_CHANLIST_CHAT,
                            WolText(pChannel->name), pChannel->currentUsers);
        pILChannels->Add_Item(show.c_str(), szHelp,
                              IconPointer(base::At(DibIconInfos, kDibiconUser)),
                              ICON_DIB, CHANNELTYPE_CHATCHANNEL, pChannel);
      } else {
        //	Channel is a lobby.
        char szLobbyName[REASONABLELOBBYINTERPRETEDNAMELEN];
        InterpretLobbyNumber(szLobbyName, iLobby);
        const std::string show =
            absl::StrFormat("%s\t%-3u", szLobbyName, pChannel->currentUsers);
        char szHelp[200];
        Format_Runtime_Text(szHelp, sizeof(szHelp), TXT_WOL_TTIP_CHANLIST_LOBBY,
                            szLobbyName, pChannel->currentUsers);
        pILChannels->Add_Item(show.c_str(), szHelp, IconForGameType(-1),
                              ICON_DIB, CHANNELTYPE_LOBBYCHANNEL, pChannel);
      }
    } else {
      //	Show game channel.
      std::string show;
      char szHelp[200];
      void* pGameKindIcon = nullptr;
      if (pChannel->type == GAME_TYPE) {
        //	Get RedAlert GameKind.
        const auto GameKind = static_cast<CREATEGAMEINFO::GAMEKIND>(
            pChannel->reserved & 0xFF000000);
        switch (GameKind) {
          case CREATEGAMEINFO::RAGAME:
            pGameKindIcon = IconPointer(
                base::At(OldRAGameTypeInfos, 0));  //	Red Alert icon
            Format_Runtime_Text(szHelp, sizeof(szHelp),
                                TXT_WOL_TTIP_CHANLIST_RAGAME,
                                TXT_WOL_TTIP_REDALERT, pChannel->currentUsers,
                                pChannel->maxUsers);
            break;
          case CREATEGAMEINFO::CSGAME:
            pGameKindIcon =
                IconPointer(base::At(OldRAGameTypeInfos, 1));  //	CS icon
            Format_Runtime_Text(szHelp, sizeof(szHelp),
                                TXT_WOL_TTIP_CHANLIST_RAGAME,
                                TXT_WOL_TTIP_COUNTERSTRIKE,
                                pChannel->currentUsers, pChannel->maxUsers);
            break;
          case CREATEGAMEINFO::AMGAME:
            pGameKindIcon =
                IconPointer(base::At(OldRAGameTypeInfos, 2));  //	AM icon
            Format_Runtime_Text(szHelp, sizeof(szHelp),
                                TXT_WOL_TTIP_CHANLIST_RAGAME,
                                TXT_WOL_TTIP_AFTERMATH, pChannel->currentUsers,
                                pChannel->maxUsers);
            break;
          default:
            //					debugprint( "Illegal value for
            // GameKind channel reserved field: %s\n", (char*)pChannel->name );
            pGameKindIcon = nullptr;
            break;
        }
        show = absl::StrFormat("%s\t%u/%u", WolText(pChannel->name),
                               pChannel->currentUsers, pChannel->maxUsers);
      } else {
        pGameKindIcon = IconForGameType(pChannel->type);
        show = absl::StrFormat("%s\t%-2u", WolText(pChannel->name),
                               pChannel->currentUsers);
        Format_Runtime_Text(szHelp, sizeof(szHelp), TXT_WOL_TTIP_CHANLIST_GAME,
                            NameOfGameType(pChannel->type),
                            pChannel->currentUsers);
      }
      void* pPrivateIcon = nullptr;
      if (pChannel->flags & CHAN_MODE_KEY) {
        //	Game is private.
        pPrivateIcon = IconPointer(base::At(DibIconInfos, kDibiconPrivate));
        port::SafeAppend(szHelp, TXT_WOL_TTIP_PRIVATEGAME);
      }

      void* pTournamentIcon = nullptr;
      if (pChannel->tournament) {
        //	Game is tournament.
        pTournamentIcon =
            IconPointer(base::At(DibIconInfos, kDibiconTournament));
        port::SafeAppend(szHelp, TXT_WOL_TTIP_TOURNAMENTGAME);
      }

      int iLatencyUse = pChannel->latency;
      if (iLatencyUse == -1) {
        iLatencyUse = 0;
      }

      static const int iLatencyBarX = 227 - iLatencyIconWidth - 19;

      pILChannels->Add_Item(
          show.c_str(), szHelp, pGameKindIcon, ICON_DIB,
          CHANNELTYPE_GAMECHANNEL, pChannel, nullptr, pPrivateIcon, ICON_DIB,
          pTournamentIcon, ICON_DIB,
          IconPointer(base::At(DibIconInfos, kDibiconLatency)), ICON_DIB,
          iLatencyBarX, 0,
          static_cast<int>(static_cast<float>(iLatencyUse) *
                           fLatencyToIconWidth));
    }
    pChannel = pChannel->next;
  }
  if (iListViewIndex) {
    pILChannels->Set_View_Index(
        iListViewIndex);  //	Not perfect but should keep list pretty stable
                          // on updates.
  }
}

//***********************************************************************************************
HRESULT WolapiObject::ChannelJoin(const char* szChannelName,
                                  const char* szKey) {
  //	Used for CHAT channels (or lobbies) only. Channel type is set to 0.
  Channel ChannelTemp;
  base::FillBytes(base::ObjectBytes(ChannelTemp), 0, sizeof(ChannelTemp));
  port::SafeCopy(WolTextBuffer(ChannelTemp.name), szChannelName);
  port::SafeCopy(WolTextBuffer(ChannelTemp.key), szKey);
  return ChannelJoin(&ChannelTemp);
}

//***********************************************************************************************
// Not const: sends a chat request and waits for the reply.
// NOLINTNEXTLINE(readability-make-member-function-const)
HRESULT WolapiObject::ChannelJoin(Channel* pChannelToJoin) {
  //	Returns an HRESULT, the meaning of which is totally customized for my
  // own uses.

  WWMessageBox().Process(TXT_WOL_WAIT, TXT_NONE);

  pChatSink->bRequestChannelJoinWait = true;
  pChatSink->hresRequestJoinResult = 0;

  //	debugprint( "RequestChannelJoin(), %s\n", pChannelToJoin->name );
  const HRESULT hRes = pChat->RequestChannelJoin(pChannelToJoin);
  if (!SUCCEEDED(hRes)) {
    //		debugprint( "RequestChannelJoin() call failed, result %i ", hRes
    //);
    DebugChatDef(hRes);
    return E_FAIL;
  }
  pChatSink->bIgnoreChannelLists = true;  //	Turn off response to channel
                                          // lists.
  const DWORD dwTimeStart = Get_Time_Ms();
  DWORD dwTimeNextPump = Get_Time_Ms() + PUMPSLEEPDURATION;
  while (pChatSink->bRequestChannelJoinWait &&
         Get_Time_Ms() - dwTimeStart < EMERGENCY_TIMEOUT) {
    while (Get_Time_Ms() < dwTimeNextPump) {
      ServiceRealTime();
    }
    pChat->PumpMessages();
    dwTimeNextPump = Get_Time_Ms() + PUMPSLEEPDURATION;
  }
  pChatSink->bIgnoreChannelLists =
      false;  //	Turn on response to channel lists.

  if (pChatSink->bRequestChannelJoinWait) {
    return CHAT_E_TIMEOUT;
  }

  switch (pChatSink->hresRequestJoinResult) {
    case CHAT_E_CHANNELDOESNOTEXIST:
    case CHAT_E_BADCHANNELPASSWORD:
    case CHAT_E_BANNED:
    case CHAT_E_CHANNELFULL:
      return pChatSink->hresRequestJoinResult;
    default:
      break;
  }

  if (!pChatSink->bJoined) {
    return E_FAIL;
  }

  return S_OK;
}

//***********************************************************************************************
// Not const: sends a chat request and waits for the reply.
// NOLINTNEXTLINE(readability-make-member-function-const)
bool WolapiObject::ChannelLeave() {
  //	Returns false upon total failure.
  WWMessageBox().Process(TXT_WOL_WAIT, TXT_NONE);

  pChatSink->bRequestChannelLeaveWait = true;
  pChatSink->DeleteUserList();

  //	debugprint( "RequestChannelLeave()\n" );
  if (!SUCCEEDED(pChat->RequestChannelLeave())) {
    //		debugprint( "RequestChannelLeave() call failed\n" );
    return false;
  }
  pChatSink->bIgnoreChannelLists = true;  //	Turn off response to channel
                                          // lists.
  const DWORD dwTimeStart = Get_Time_Ms();
  DWORD dwTimeNextPump = Get_Time_Ms() + PUMPSLEEPDURATION;
  while (pChatSink->bRequestChannelLeaveWait &&
         Get_Time_Ms() - dwTimeStart < EMERGENCY_TIMEOUT) {
    while (Get_Time_Ms() < dwTimeNextPump) {
      ServiceRealTime();
    }
    pChat->PumpMessages();
    dwTimeNextPump = Get_Time_Ms() + PUMPSLEEPDURATION;
  }
  pChatSink->bIgnoreChannelLists = false;

  return !(pChatSink->bRequestChannelLeaveWait || pChatSink->bJoined);
}

//***********************************************************************************************
// typedef char CHANNELUSERNAME[ WOL_NAME_LEN_MAX ];
struct CHANNELUSERINFO {
  char szName[WOL_NAME_LEN_MAX];
  bool bFlagged;
  RemapControlType* pColorRemap;
  HousesType House;  //	Only used if game channel.
  bool bAccept;      //	Only used if game channel.
  char szExtra[50];  //	Only used if game channel.
};

//***********************************************************************************************
bool WolapiObject::ListChannelUsers() {
  //	Show pChatSink's pUserList in pILUsers or pILPlayers (depending on
  // CurrentLevel), after clearing it. 	The extra data ptr hidden in each list
  // item will hold a void pointer to the user described. 	For simplicity,
  // I destroy the old list and write a new one, even though possibly only one
  // item 	may have changed. 	In order for the multiselect flags in
  // the list to
  // persist, I record the names that are flagged 	before clearing the
  // list, then reset them (if found) in the new list. 	This is inefficient, but
  // should be fine in this non-time-critical situation. 	(I also save
  // item color here, and save all items. Now it's really inefficient.)
  // (Oh, boy. Now I've added the persistence of house info when this is a game
  // channel...) 	The idea was to avoid duplication of player data, and
  // not have any dependence on the integrity of the chatsink's 	players
  // list. Now it is a case of it working "well enough" to not require time to
  // elegantize it.

  //	Extra bonus, if useful: returns true if an operator (channel owner) is
  // found in the channel, false otherwise.

  bool bChannelOwnerFound = false;

  bool bInLobby = false;
  IconListClass* pListToUse = nullptr;
  if (CurrentLevel == WOL_LEVEL_INGAMECHANNEL) {
    bInLobby = false;
    pListToUse = pILPlayers;
  } else {
    bInLobby = (iChannelLobbyNumber(szChannelNameCurrent) != -1);
    pListToUse = pILUsers;
  }

  if (pListToUse &&  //	Fails in rare cases when list draw is triggered before
                     // it is fully set up.
      *szChannelNameCurrent)  //	No users to list if not in a channel.
  {
    //	If redrawing the same list as before, preserve the view position.
    static char szChannelLastListed[WOL_CHANNAME_LEN_MAX] = {0};
    // debugprint( "szChannelLastListed '%s', szChannelNameCurrent '%s'\n",
    // szChannelLastListed, szChannelNameCurrent );
    int iListViewIndex = 0;
    if (std::string_view(szChannelLastListed) == szChannelNameCurrent) {
      iListViewIndex = pListToUse->Get_View_Index();
    } else {
      port::SafeCopy(szChannelLastListed, szChannelNameCurrent);
    }

    // debugprint( "ListChannelUsers(), pUserList = %i\n", pChatSink->pUserList
    // ); 	Save users in current list.
    const int iCount = pListToUse->Count();
    std::vector<CHANNELUSERINFO> pUsersSaved;
    int iUsersSaved = 0;
    if (iCount) {
      pUsersSaved.resize(base::ToSize(iCount));
      for (int i = 0; i != iCount; i++) {
        PullPlayerName_Into_From(
            pUsersSaved.at(base::ToSize(iUsersSaved)).szName,
            pListToUse->Get_Item(i));
        pUsersSaved.at(base::ToSize(iUsersSaved)).bFlagged =
            pListToUse->bItemIsMultiSelected(i);
        pUsersSaved.at(base::ToSize(iUsersSaved)).pColorRemap =
            pListToUse->Get_Item_Color(i);
        //				debugprint( "  Saving color of %s as
        //%i.\n", pUsersSaved[base::ToSize( iUsersSaved )].szName,
        //pUsersSaved[base::ToSize( iUsersSaved
        //)].pColorRemap );
        if (CurrentLevel == WOL_LEVEL_INGAMECHANNEL) {
          pUsersSaved.at(base::ToSize(iUsersSaved)).House =
              PullPlayerHouse_From(pListToUse->Get_Item(i));
          pUsersSaved.at(base::ToSize(iUsersSaved)).bAccept =
              bItemMarkedAccepted(i);
          const char* szExtra = pListToUse->Get_Item_ExtraDataString(i);
          if (szExtra) {
            port::SafeCopy(pUsersSaved.at(base::ToSize(iUsersSaved)).szExtra,
                           szExtra);
          } else {
            *pUsersSaved.at(base::ToSize(iUsersSaved)).szExtra = 0;
          }
        }
        iUsersSaved++;
      }
    }
    //	Clear list and recreate with new users list.
    pListToUse->Clear();
    User* pUser = pChatSink->pUserList;
    int iUserCount = 0;
    while (pUser) {
      ++iUserCount;
      void* pIconStatus = nullptr;
      void* pIconSquelched = nullptr;
      if (pUser->flags & CHAT_USER_CHANNELOWNER) {
        pIconStatus = IconPointer(base::At(DibIconInfos, kDibiconOwner));
        bChannelOwnerFound = true;
      } else {
        if (CurrentLevel == WOL_LEVEL_INGAMECHANNEL) {
          pIconStatus = IconPointer(base::At(DibIconInfos, kDibiconNotaccept));
        } else {
          if (pUser->flags & CHAT_USER_VOICE) {
            pIconStatus = IconPointer(base::At(DibIconInfos, kDibiconVoice));
          } else {
            pIconStatus = IconPointer(base::At(DibIconInfos, kDibiconUser));
          }
        }
      }
      if (pUser->flags & CHAT_USER_SQUELCHED) {
        pIconSquelched = IconPointer(base::At(DibIconInfos, kDibiconSquelch));
      }

      if (CurrentLevel == WOL_LEVEL_INGAMECHANNEL || bInLobby) {
        const int iRank =
            pNetUtilSink->GetUserRank(WolText(pUser->name), bShowRankRA);
        char szNameToShow[WOL_NAME_LEN_MAX + 40];
        if (iRank) {
          //					debugprint("  Found %s has rank
          //%u\n", (char*)pUser->name, iRank );
          Format_Runtime_Text(szNameToShow, sizeof(szNameToShow),
                              TXT_WOL_USERRANK, WolText(pUser->name), iRank);
        } else {
          port::SafeCopy(szNameToShow, WolText(pUser->name));
        }

        static const int iLatencyBarX = 248 - iLatencyIconWidth - 5 - 16;

        //	If we have had a chance to request pings to the player, there'll
        // be some avg. results waiting for us.
        int iLatencyBarWidth = 0;
        int iLatency = 0;
        if (CurrentLevel == WOL_LEVEL_INGAMECHANNEL) {
          const uint32_t UserIP = pChatSink->GetUserIP(WolText(pUser->name));
          //					debugprint( "player %s ip
          // address %i\n", szNameToShow, UserIP );
          if (UserIP && pNetUtil->GetAvgPing(UserIP, &iLatency) == S_OK) {
            //						debugprint( "player %s
            // latency %i\n", szNameToShow, iLatency );
            if (iLatency == -1) {
              iLatency = 0;
            }
            iLatencyBarWidth = static_cast<int>(static_cast<float>(iLatency) *
                                                fLatencyToIconWidth);
          }
        }
        pListToUse->Add_Item(
            szNameToShow, nullptr, pIconStatus, ICON_DIB, nullptr, pUser,
            nullptr, pIconSquelched, ICON_DIB, nullptr, ICON_DIB,
            IconPointer(base::At(DibIconInfos, kDibiconLatency)), ICON_DIB,
            iLatencyBarX, 2, iLatencyBarWidth);
      } else {
        pListToUse->Add_Item(WolText(pUser->name), nullptr, pIconStatus,
                             ICON_DIB, nullptr, pUser, nullptr, pIconSquelched,
                             ICON_DIB);
      }

      pUser = pUser->next;
    }
    if (pStaticUsers) {
      //	Display number of users in channel.
      char szCount[100];
      Format_Runtime_Text(szCount, sizeof(szCount), TXT_WOL_USERLIST,
                          iUserCount);
      pStaticUsers->Set_Text(szCount);
    }
    //	Reset multiselectedness, color, and item text for a user. Slow.
    //	(What a bloody, bloody hack.)
    for (int iUser = 0; iUser != iUsersSaved; iUser++) {
      const int iFind = pListToUse->Find(
          pUsersSaved.at(base::ToSize(iUser))
              .szName);  //	Finds any item beginning with szName...
      if (iFind != -1) {
        if (CurrentLevel == WOL_LEVEL_INGAMECHANNEL) {
          if (pUsersSaved.at(base::ToSize(iUser)).House != HOUSE_NONE) {
            //	Append house text to item string, as we found a valid house name
            // after the name, above.
            char szItem[120];
            WritePlayerListItem(szItem, sizeof(szItem),
                                pUsersSaved.at(base::ToSize(iUser)).szName,
                                pUsersSaved.at(base::ToSize(iUser)).House);
            pListToUse->Set_Item(static_cast<unsigned int>(iFind), szItem);
          }
          //	Player was marked "accepted" before. If he has one now, it's
          // because he is the host. 	Else it was an accepted icon before, so
          // put one in again now. (a-hacking-we-will-go)
          if (pUsersSaved.at(base::ToSize(iUser)).bAccept &&
              (!bItemMarkedAccepted(iFind))) {
            MarkItemAccepted(iFind, true);
          }

          if (*pUsersSaved.at(base::ToSize(iUser)).szExtra) {
            pListToUse->Set_Item_ExtraDataString(
                iFind, pUsersSaved.at(base::ToSize(iUser)).szExtra);
          }
        }
        if (pUsersSaved.at(base::ToSize(iUser)).bFlagged) {
          pListToUse->MultiSelect(iFind, true);
        }
        //				debugprint( "  Restoring color of %s as
        //%i.\n", pUsersSaved[base::ToSize( iUser )].szName,
        //pUsersSaved[base::ToSize( iUser )].pColorRemap
        //);
        pListToUse->Set_Item_Color(
            iFind, pUsersSaved.at(base::ToSize(iUser)).pColorRemap);
      }
      //			else
      //				debugprint( "ListChannelUsers() -
      // Couldn't find %s!\n", pUsersSaved[base::ToSize( iUser )].szName );
    }
    if (iListViewIndex) {
      pListToUse->Set_View_Index(
          iListViewIndex);  //	Not perfect but should keep list pretty stable
                            // on updates.
    }
  }
  return bChannelOwnerFound;
}

//***********************************************************************************************
bool WolapiObject::bItemMarkedAccepted(int iIndex) {
  //	Returns true if the iIndex'th entry in pILPlayers has an icon pointer in
  // position 0 that 	is either the host icon or the accepted icon.
  const IconList_ItemExtras* pItemExtras = pILPlayers->Get_ItemExtras(iIndex);
  return (base::At(pItemExtras->pIcon, 0).image ==
              IconPointer(base::At(DibIconInfos, kDibiconOwner)) ||
          base::At(pItemExtras->pIcon, 0).image ==
              IconPointer(base::At(DibIconInfos, kDibiconAccept)));
}

//***********************************************************************************************
bool WolapiObject::MarkItemAccepted(int iIndex, bool bAccept) {
  pILPlayers->Flag_To_Redraw();
  if (bAccept) {
    return pILPlayers->Set_Icon(
        static_cast<unsigned int>(iIndex), 0,
        IconPointer(base::At(DibIconInfos, kDibiconAccept)), ICON_DIB);
  }  // return pILPlayers->Set_Icon( iIndex, 0, NULL, ICON_DIB );
  return pILPlayers->Set_Icon(
      static_cast<unsigned int>(iIndex), 0,
      IconPointer(base::At(DibIconInfos, kDibiconNotaccept)), ICON_DIB);
}

//***********************************************************************************************
bool WolapiObject::bItemMarkedReadyToGo(int iIndex) {
  //	Returns true if the iIndex'th entry in pILPlayers marks the player as
  //"ready to go". 	This is true if the player is marked as "ready" or "need
  // scenario".
  const char* szItem = pILPlayers->Get_Item_ExtraDataString(iIndex);
  if (!szItem) {
    return false;
  }
  //	debugprint( "szItem is %s\n", szItem );
  return (std::string_view(szItem) == "ready" ||
          std::string_view(szItem) == "need scenario");
}

//***********************************************************************************************
void WolapiObject::MarkItemReadyToGo(int iIndex, const char* szReadyState) {
  //	Set szReadyState to "ready", "need scenario", or NULL.
  //	First two cases are regarded as player being ready to go.
  pILPlayers->Flag_To_Redraw();
  pILPlayers->Set_Item_ExtraDataString(iIndex, szReadyState);
}

//***********************************************************************************************
bool WolapiObject::bItemMarkedNeedScenario(int iIndex) {
  //	Returns true if the iIndex'th entry in pILPlayers marks the player as
  // ready to go, but needing scenario download.
  const char* szItem = pILPlayers->Get_Item_ExtraDataString(iIndex);
  if (!szItem) {
    return false;
  }
  return (std::string_view(szItem) == "need scenario");
}

//***********************************************************************************************
void WolapiObject::PullPlayerName_Into_From(std::span<char> szDest,
                                            const char* szSource) {
  //	Sets szDest to the "player name" found in szSource.
  //	Called "player" name because this is mainly designed for use in game
  // channels. 	Player name appears first in item, separated by a space from
  // anything later.

  const auto space = std::string_view(szSource).find(' ');
  if (space == std::string_view::npos) {
    //	No space character. Use entire item.
    port::SafeCopy(szDest, szSource);
  } else {
    const auto iSpacePosition = space + 1;
    port::SafeCopy(szDest.first(std::min(iSpacePosition, szDest.size())),
                   szSource);
  }
  //	debugprint( "PullPlayerName_Into_From: '%s' from '%s', ok?\n", szDest,
  // szSource );
}

//***********************************************************************************************
HousesType WolapiObject::PullPlayerHouse_From(const char* szSource) {
  //	Pulls the house value out of a player list item in a game channel.
  //	House appears as the last word, and it's in <>.
  //	char* pChar = strrchr( szSource, ' ' );		//	Last space
  // character.		was failing on roy. uni cause of space 	if( !pChar )
  //		return HOUSE_NONE;
  //	++pChar;
  //	if( *pChar++ != '<' )		//	We know house has to be last, so
  // if not the case, no house in item. 		return HOUSE_NONE;
  const std::string_view source(szSource);
  const auto house_start = source.rfind('<');
  if (house_start == std::string_view::npos) {
    return HOUSE_NONE;
  }
  const auto house_name = source.substr(house_start + 1);
  const std::size_t iLen = house_name.size();  //	Remaining: "housename>"
  //	Copy remaining string, minus the trailing ">".
  char szHouse[30];
  port::SafeCopy(std::span(szHouse).first(std::min(iLen, sizeof(szHouse))),
                 house_name);
  //	debugprint( "PullPlayerHouse_From: '%s' from '%s', ok?\n", szHouse,
  // szSource ); 	pChar is now a valid house name.

  //	return HouseTypeClass::From_Name( szHouse );
  if constexpr (config::kIsEnglish) {
    // From_Name() knows the house as "USSR", but the game calls it "Russia".
    if (std::string_view(szHouse) == "Russia") {
      return HOUSE_USSR;
    }
    return HouseTypeClass::From_Name(szHouse);
  } else {
    for (HousesType house = HOUSE_USSR; house <= HOUSE_FRANCE; house++) {
      if (std::string_view(Text_String(
              HouseTypeClass::As_Reference(house).Full_Name())) == szHouse) {
        return house;
      }
    }
    return HOUSE_USSR;  // Should never happen.
  }
}

//***********************************************************************************************
void WolapiObject::WritePlayerListItem(std::span<char> szDest,
                                       std::size_t iSize, const char* szName,
                                       HousesType House) const {
  //	Sets szDest to the way a player list item appears in a game channel.
  char szHouse[50];
  port::SafeCopy(szHouse,
                 Text_String(HouseTypeClass::As_Reference(House).Full_Name()));

  const int iRank = pNetUtilSink->GetUserRank(
      szName, bShowRankRA);  //	Horrendous inefficiency here, when called for
                             // relisting players...
  if (iRank) {
    Format_Runtime_Text(szDest, iSize, TXT_WOL_USERRANKHOUSE, szName, iRank,
                        szHouse);
  } else {
    Format_Runtime_Text(szDest, iSize, TXT_WOL_USERHOUSE, szName, szHouse);
  }
  //	debugprint( "WritePlayerListItem: '%s', ok?\n", szDest );
}

//***********************************************************************************************
void WolapiObject::RequestPlayerPings() {
  //	Does a RequestPing for every other player listed in pILPlayers.
  for (int i = 0; i < pILPlayers->Count(); i++) {
    const User* pUser =
        static_cast<const User*>(pILPlayers->Get_Item_ExtraDataPtr(i));
    if (pUser && !(pUser->flags & CHAT_USER_MYSELF)) {
      const uint32_t UserIP = pChatSink->GetUserIP(WolText(pUser->name));
      if (UserIP) {
        int iUnused = 0;
        in_addr inaddrUser{};
        inaddrUser.s_addr = UserIP;
        const std::string szIP = port::Ipv4Text(inaddrUser);
        //				debugprint( "RequestPing of %s, ipaddr
        // of %i, aka %s\n", (char*)pUser->name, UserIP, szIP );
        pNetUtil->RequestPing(szIP.c_str(), 1000, &iUnused);
      }
    }
  }
}

//***********************************************************************************************
void WolapiObject::SendMessage(const char* szMessage, IconListClass& ILUsers,
                               bool bAction) {
  //	Send regular chat message.

  if (*szMessage == 0) {
    return;
  }

  if (std::string_view(szMessage).size() > 4 &&
      std::string_view(szMessage).starts_with("?ajw")) {
    const int i = tech::ParseInteger<int>(std::string_view(szMessage).substr(4))
                      .value_or(0);
    if (i >= static_cast<int>(VOX_ACCOMPLISHED) &&
        i <= static_cast<int>(VOX_LOAD1)) {
      Speak(static_cast<VoxType>(i));
    }
    return;
  }
  if (std::string_view(szMessage).size() > 4 &&
      std::string_view(szMessage).starts_with("#ajw")) {
    const int i = tech::ParseInteger<int>(std::string_view(szMessage).substr(4))
                      .value_or(0);
    if (i >= static_cast<int>(VOX_ACCOMPLISHED) &&
        i <= static_cast<int>(VOX_LOAD1)) {
      Speak(static_cast<VoxType>(i));
    }
  }

  //	Iterate through ILUsers looking for selected entries. Build up a users
  // list of selected 	items. If the list turns out to be blank, send message
  // publicly.
  User* pUserListSend = nullptr;
  User* pUserTail = nullptr;
  const int iCount = ILUsers.Count();
  int iPrivatePrintLen = 1;
  for (int i = 0; i != iCount; i++) {
    if (ILUsers.bItemIsMultiSelected(i)) {
      User* pUserNew = new User;
      *pUserNew = *static_cast<const User*>(ILUsers.Get_Item_ExtraDataPtr(i));
      //			debugprint( "Copied %s for sendmessage.\n",
      // pUserNew->name );
      pUserNew->next =
          nullptr;  //	(We don't want the value that was just copied!)
      if (!pUserTail) {
        //	First User in list.
        pUserListSend = pUserNew;
      } else {
        pUserTail->next = pUserNew;
      }
      pUserTail = pUserNew;
      //	Extra space and comma.
      iPrivatePrintLen +=
          static_cast<int>(std::string_view(WolText(pUserNew->name)).size()) +
          2;
    }
  }
  if (pUserListSend) {
    //	Send private message.
    if (!bAction) {
      pChat->RequestPrivateMessage(pUserListSend, szMessage);
    } else {
      pChat->RequestPrivateAction(pUserListSend, szMessage);
    }
    //	One buffer for either shape of the message, sized for the longest.
    const std::size_t iPrintSize =
        std::string_view(szMessage).size() + std::string_view(szMyName).size() +
        static_cast<std::size_t>(iPrivatePrintLen) + 140;
    std::vector<char> print_storage(iPrintSize);
    char* szPrint = print_storage.data();
    if (iPrivatePrintLen > 50) {
      //	Too many users specified to print out. Just say "multiple
      // users".
      if (!bAction) {
        absl::SNPrintF(szPrint, iPrintSize, "%s %s", TXT_WOL_PRIVATETOMULTIPLE,
                       szMessage);
      } else {
        absl::SNPrintF(szPrint, iPrintSize, "%s %s %s",
                       TXT_WOL_PRIVATETOMULTIPLE, szMyName, szMessage);
      }
    } else {
      // strcpy( szPrint, "<Private to " );
      absl::SNPrintF(szPrint, iPrintSize, "<%s ", TXT_WOL_PRIVATETO);
      User* pUserPrint = pUserListSend;
      while (pUserPrint) {
        port::SafeAppend(print_storage, WolText(pUserPrint->name));
        if (pUserPrint->next) {
          port::SafeAppend(print_storage, ", ");
        } else {
          port::SafeAppend(print_storage, ">: ");
        }
        pUserPrint = pUserPrint->next;
      }
      if (bAction) {
        port::SafeAppend(print_storage, szMyName);
        port::SafeAppend(print_storage, " ");
      }
      port::SafeAppend(print_storage, szMessage);
    }
    if (!bAction) {
      PrintMessage(szPrint, WOLCOLORREMAP_SELFSPEAKING);
    } else {
      PrintMessage(szPrint, WOLCOLORREMAP_ACTION);
      RAChatEventSink::ActionEggSound(szMessage);
    }

  } else {
    //	Send public message.
    if (!bAction) {
      //	Easter egg related.
      if (absl::EqualsIgnoreCase(szMessage, "/nousersounds")) {
        bEggSounds = false;
        return;
      }
      if (absl::EqualsIgnoreCase(
              szMessage, "/usersounds"))  //	Left as obvious text in the exe,
                                          //for someone to find...
                                          //:-)
      {
        bEggSounds = true;
        return;
      }
      if (absl::EqualsIgnoreCase(
              szMessage, "/8playergames"))  //	Left as obvious text in the exe,
                                            //for someone to find...
                                            //:-)
      {
        bEgg8Player = true;
        return;
      }
      const HRESULT hRes = pChat->RequestPublicMessage(szMessage);
      if (hRes != S_OK) {
        //				debugprint( " RequestPublicMessage()
        // failed with: " ); 				DebugChatDef( hRes );
      }
    } else {
      const HRESULT hRes = pChat->RequestPublicAction(szMessage);
      if (hRes != S_OK) {
        //				debugprint( " RequestPublicAction()
        // failed with: " ); 				DebugChatDef( hRes );
      }
    }
    if (!bAction) {
      PrintMessage(absl::StrFormat("%s: %s", szMyName, szMessage).c_str(),
                   WOLCOLORREMAP_SELFSPEAKING);
    } else {
      PrintMessage(absl::StrFormat("%s %s", szMyName, szMessage).c_str(),
                   WOLCOLORREMAP_ACTION);
      RAChatEventSink::ActionEggSound(szMessage);
    }
  }
}

//***********************************************************************************************
bool WolapiObject::ChannelCreate(
    const char* szChannelName, const char* szKey, bool bGame /* = false */,
    int iMaxPlayers /* = 0 */, bool bTournament /* = false */,
    int iLobby /* = 0 */, CREATEGAMEINFO::GAMEKIND GameKind /* = red alert */) {
  //	Create a channel.
  //	szKey is NULL if a public channel is to be created, else channel
  // password.

  //	Returns true if everything goes okay.

  if (pChatSink->bJoined) {
    //	This never happens. Here just in case.
    //		debugprint( "WolapiObject::ChannelCreate called when bJoined is
    // true!\n" );
    return false;
    //		Fatal( "WolapiObject::ChannelCreate called when bJoined is
    // true!" );
  }

  Channel ChannelNew;

  //	Prepare the struct.
  base::FillBytes(base::ObjectBytes(ChannelNew), 0, sizeof(ChannelNew));

  if (!bGame) {
    //	ChannelNew.type = 0;	0 for chat channel.
    port::SafeCopy(WolTextBuffer(ChannelNew.name), szChannelName);
  } else {
    ChannelNew.type = GAME_TYPE;
    ChannelNew.maxUsers = static_cast<unsigned int>(iMaxPlayers);
    ChannelNew.tournament = static_cast<unsigned int>(bTournament);
    //	Channel 'reserved' stores GameKind in the highest byte, and
    //	lobby number to return to in the lower three bytes.
    //	Note: If lobby number is -1 (no lobby to return to), it's encoded as
    // 0x00FFFFFF
    ChannelNew.reserved = (static_cast<uint32_t>(iLobby) & 0x00FFFFFFU) |
                          static_cast<uint32_t>(GameKind);
    port::SafeCopy(WolTextBuffer(ChannelNew.name), szChannelName);
  }

  //	debugprint( "RequestChannelCreate(), channel name: '%s'\n",
  // szChannelName );

  if (szKey) {
    port::SafeCopy(WolTextBuffer(ChannelNew.key), szKey);
  }

  WWMessageBox().Process(TXT_WOL_WAIT, TXT_NONE);

  pChatSink->bRequestChannelCreateWait = true;

  const HRESULT hRes = pChat->RequestChannelCreate(&ChannelNew);
  if (!SUCCEEDED(hRes)) {
    //		debugprint( "RequestChannelCreate() call failed:" );
    DebugChatDef(hRes);
    return false;
  }
  pChatSink->bIgnoreChannelLists = true;  //	Turn off response to channel
                                          // lists.
  const DWORD dwTimeStart = Get_Time_Ms();
  DWORD dwTimeNextPump = Get_Time_Ms() + PUMPSLEEPDURATION;
  while (pChatSink->bRequestChannelCreateWait &&
         Get_Time_Ms() - dwTimeStart < EMERGENCY_TIMEOUT) {
    while (Get_Time_Ms() < dwTimeNextPump) {
      ServiceRealTime();
    }
    pChat->PumpMessages();
    dwTimeNextPump = Get_Time_Ms() + PUMPSLEEPDURATION;
  }
  pChatSink->bIgnoreChannelLists = false;

  if (pChatSink->bRequestChannelCreateWait || !pChatSink->bJoined) {
    return false;  //	Timed out or callback got fail value.
  }

  if (bGame) {
    iLobbyReturnAfterGame = iLobby;
  }

  return true;
}

//***********************************************************************************************
void WolapiObject::DoFindPage() {
  //	User presses find/page button.

  //	Ask user for user desired.
  Fancy_Text_Print(TXT_NONE, 0, 0, nullptr, kTBlack,
                   kTpfText);  //	Required before String_Pixel_Width()
                               // call, for god's sake.
  auto* pFindPageDlg = new SimpleEditDlgClass(
      400, TXT_WOL_PAGELOCATE, TXT_WOL_USERNAMEPROMPT, WOL_NAME_LEN_MAX);
  pFindPageDlg->SetButtons(TXT_WOL_LOCATE, Text_String(TXT_CANCEL),
                           TXT_WOL_PAGE);
  bPump_In_Call_Back = true;
  const char* szNameDlgResult = pFindPageDlg->Show();
  bPump_In_Call_Back = false;

  if (std::string_view(szNameDlgResult) == Text_String(TXT_CANCEL) ||
      !*pFindPageDlg->szEdit) {
    delete pFindPageDlg;
    return;
  }

  if (std::string_view(szNameDlgResult) == TXT_WOL_LOCATE) {
    //	Locate user.
    const HRESULT hRes = Locate(pFindPageDlg->szEdit);
    switch (hRes) {
      case CHAT_S_FIND_NOTHERE:
        bPump_In_Call_Back = true;
        WWMessageBox().Process(TXT_WOL_FIND_NOTHERE);
        bPump_In_Call_Back = false;
        break;
      case CHAT_S_FIND_NOCHAN:
        bPump_In_Call_Back = true;
        WWMessageBox().Process(TXT_WOL_FIND_NOCHAN);
        bPump_In_Call_Back = false;
        break;
      case CHAT_S_FIND_OFF:
        bPump_In_Call_Back = true;
        WWMessageBox().Process(TXT_WOL_FIND_OFF);
        bPump_In_Call_Back = false;
        break;
      case CHAT_E_TIMEOUT:
        bPump_In_Call_Back = true;
        WWMessageBox().Process(TXT_WOL_TIMEOUT);
        bPump_In_Call_Back = false;
        break;
      case E_FAIL:
        GenericErrorMessage();
        break;
      case S_OK: {
        const char* szChannel = WolText(pChatSink->OnFindChannel.name);
        const int iLobby = iChannelLobbyNumber(szChannel);
        std::string szFound;
        if (iLobby != -1) {
          char szLobbyName[REASONABLELOBBYINTERPRETEDNAMELEN];
          InterpretLobbyNumber(szLobbyName, iLobby);
          szFound = port::FormatRuntime(TXT_WOL_FOUNDIN, szLobbyName);
        } else {
          szFound = port::FormatRuntime(TXT_WOL_FOUNDIN, szChannel);
        }
        bPump_In_Call_Back = true;
        WWMessageBox().Process(szFound.c_str());
        bPump_In_Call_Back = false;

        break;
      }
      default:
        break;
    }
  } else {
    //	Page user.
    //	Ask user for text to send.
    auto* pMessDlg =
        new SimpleEditDlgClass(600, TXT_WOL_PAGEMESSAGETITLE,
                               TXT_WOL_PAGEMESSAGEPROMPT, MAXCHATSENDLENGTH);
    bPump_In_Call_Back = true;
    if (std::string_view(pMessDlg->Show()) == Text_String(TXT_OK) &&
        *pMessDlg->szEdit) {
      switch (Page(pFindPageDlg->szEdit, pMessDlg->szEdit, true)) {
        case CHAT_S_PAGE_NOTHERE:
          WWMessageBox().Process(TXT_WOL_PAGE_NOTHERE);
          break;
        case CHAT_S_PAGE_OFF:
          WWMessageBox().Process(TXT_WOL_PAGE_OFF);
          break;
        case CHAT_E_TIMEOUT:
          WWMessageBox().Process(TXT_WOL_TIMEOUT);
          break;
        case E_FAIL:
          GenericErrorMessage();
          break;
        case S_OK:
          char szMessage[WOL_NAME_LEN_MAX + 30];
          Format_Runtime_Text(szMessage, sizeof(szMessage), TXT_WOL_WASPAGED,
                              pFindPageDlg->szEdit);
          PrintMessage(szMessage, WOLCOLORREMAP_LOCALMACHINEMESS);
          break;
        default:
          break;
      }
    }
    bPump_In_Call_Back = false;
  }

  delete pFindPageDlg;
}

//***********************************************************************************************
// Not const: sends a chat request and waits for the reply.
// NOLINTNEXTLINE(readability-make-member-function-const)
HRESULT WolapiObject::Locate(const char* szUser) {
  //	Returns HRESULT with possibly customized meanings.

  const std::string szMessage = port::FormatRuntime(TXT_WOL_LOCATING, szUser);
  WWMessageBox().Process(szMessage.c_str(), TXT_NONE);

  pChatSink->bRequestFindWait = true;

  User userFind;
  port::SafeCopy(WolTextBuffer(userFind.name), szUser);

  //	debugprint( "RequestFind()\n" );
  if (!SUCCEEDED(pChat->RequestFind(&userFind))) {
    //		debugprint( "RequestFind() call failed\n" );
    return 0;
  }
  const DWORD dwTimeStart = Get_Time_Ms();
  DWORD dwTimeNextPump = Get_Time_Ms() + PUMPSLEEPDURATION;
  while (pChatSink->bRequestFindWait &&
         Get_Time_Ms() - dwTimeStart < EMERGENCY_TIMEOUT) {
    while (Get_Time_Ms() < dwTimeNextPump) {
      ServiceRealTime();
    }
    pChat->PumpMessages();
    //		debugprint( ">Find pump\n" );
    dwTimeNextPump = Get_Time_Ms() + PUMPSLEEPDURATION;
  }

  if (pChatSink->bRequestFindWait) {
    return CHAT_E_TIMEOUT;
  }

  return pChatSink->hresRequestFindResult;
}

//***********************************************************************************************
// Not const: sends a chat request and waits for the reply.
// NOLINTNEXTLINE(readability-make-member-function-const)
HRESULT WolapiObject::Page(const char* szUser, const char* szSend,
                           bool bWaitForResult) {
  //	Returns HRESULT with possibly customized meanings.

  if (bWaitForResult) {
    const std::string szMessage = port::FormatRuntime(TXT_WOL_PAGING, szUser);
    WWMessageBox().Process(szMessage.c_str(), TXT_NONE);
  }

  pChatSink->bRequestPageWait = true;

  User userFind;
  port::SafeCopy(WolTextBuffer(userFind.name), szUser);

  //	debugprint( "RequestPage()\n" );
  if (!SUCCEEDED(pChat->RequestPage(&userFind, szSend))) {
    //		debugprint( "RequestPage() call failed\n" );
    return 0;
  }
  if (!bWaitForResult) {
    return 0;
  }
  const DWORD dwTimeStart = Get_Time_Ms();
  DWORD dwTimeNextPump = Get_Time_Ms() + PUMPSLEEPDURATION;
  while (pChatSink->bRequestPageWait &&
         Get_Time_Ms() - dwTimeStart < EMERGENCY_TIMEOUT) {
    while (Get_Time_Ms() < dwTimeNextPump) {
      ServiceRealTime();
    }
    pChat->PumpMessages();
    //		debugprint( ">Page pump\n" );
    dwTimeNextPump = Get_Time_Ms() + PUMPSLEEPDURATION;
  }

  if (pChatSink->bRequestPageWait) {
    return CHAT_E_TIMEOUT;
  }

  return pChatSink->hresRequestPageResult;
}

//***********************************************************************************************
void WolapiObject::DoKick(IconListClass* pILUsersOrPlayers, bool bAndBan) {
  //	Kick selected users.

  if (CurrentLevel != WOL_LEVEL_INCHATCHANNEL &&
      CurrentLevel != WOL_LEVEL_INLOBBY &&
      CurrentLevel != WOL_LEVEL_INGAMECHANNEL) {
    PrintMessage(TXT_WOL_YOURENOTINCHANNEL, WOLCOLORREMAP_LOCALMACHINEMESS);
    PlaySoundEffect(WOLSOUND_ERROR);
  } else if (!bChannelOwner) {
    PrintMessage(TXT_WOL_ONLYOWNERCANKICK, WOLCOLORREMAP_LOCALMACHINEMESS);
    PlaySoundEffect(WOLSOUND_ERROR);
  } else {
    int iFound = 0;
    for (int i = 0; i < pILUsersOrPlayers->Count(); i++) {
      if (pILUsersOrPlayers->bItemIsMultiSelected(i)) {
        User* pUser =
            static_cast<User*>(pILUsersOrPlayers->Get_Item_ExtraDataPtr(i));
        if (pUser && std::string_view(WolText(pUser->name)) !=
                         szMyName)  //	Don't kick yourself.
        {
          Kick(pUser);
          if (bAndBan) {
            Ban(pUser);
          }
          iFound++;
          if (iFound < 5) {
            PlaySoundEffect(static_cast<VocType>(static_cast<int>(VOC_SCREAM1) +
                                              Sim_Random_Pick(0, 8)));
          }
        }
      }
    }
    if (!iFound) {
      PrintMessage(TXT_WOL_NOONETOKICK, WOLCOLORREMAP_LOCALMACHINEMESS);
      PlaySoundEffect(WOLSOUND_ERROR);
    }
  }
}

//***********************************************************************************************
// Not const: sends a chat request.
// NOLINTNEXTLINE(readability-make-member-function-const)
bool WolapiObject::Kick(User* pUserToKick) {
  //	Returns false if something terrible happens.
  //	debugprint( "RequestUserKick()\n" );
  //		debugprint( "RequestUserKick() call failed\n" ) on failure.
  return SUCCEEDED(pChat->RequestUserKick(pUserToKick));
}

//***********************************************************************************************
// Not const: sends a chat request.
// NOLINTNEXTLINE(readability-make-member-function-const)
bool WolapiObject::Ban(User* pUserToKick) {
  //	Returns false if something terrible happens.
  //	debugprint( "RequestChannelBan()\n" );
  //		debugprint( "RequestChannelBan() call failed\n" ) on failure.
  return SUCCEEDED(pChat->RequestChannelBan(WolText(pUserToKick->name), 1));
}

//***********************************************************************************************
void WolapiObject::DoSquelch(IconListClass* pILUsersOrPlayers) {
  //	Squelch/unsquelch selected users.
  bool bFound = false;
  for (int i = 0; i < pILUsersOrPlayers->Count(); i++) {
    if (pILUsersOrPlayers->bItemIsMultiSelected(i)) {
      User* pUser =
          static_cast<User*>(pILUsersOrPlayers->Get_Item_ExtraDataPtr(i));
      if (pUser) {
        if (std::string_view(WolText(pUser->name)) !=
            szMyName)  //	Don't squelch yourself.
        {
          Squelch(pUser);
          //					char szMess[ 150 ];
          //					if( Squelch( pUser ) )
          //						sprintf( szMess,
          // TXT_WOL_USERISSQUELCHED, (char*)pUser->name );
          // else 						sprintf( szMess,
          // TXT_WOL_USERISNOTSQUELCHED, (char*)pUser->name );
          // WOL_PrintMessage( chatlist, szMess, WOLCOLORREMAP_LOCALMACHINEMESS
          // );

          bFound = true;
          pILUsersOrPlayers->Flag_To_Redraw();
        } else {
          PrintMessage(TXT_WOL_CANTSQUELCHSELF, WOLCOLORREMAP_LOCALMACHINEMESS);
        }
      }
    }
  }
  if (bFound) {
    PlaySoundEffect(VOC_SQUISH);
    ListChannelUsers();  //	Refresh displayed user list.
  }
}

//***********************************************************************************************
// Not const: sends a chat request.
// NOLINTNEXTLINE(readability-make-member-function-const)
bool WolapiObject::Squelch(User* pUserToSquelch) {
  //	Returns true if user is now squelched, false if not squelched.
  //	Sets User pointer flags value.
  //	debugprint( "Squelch:: pUser is %i, flags is %i\n", pUserToSquelch,
  // pUserToSquelch->flags );

  if (pUserToSquelch->flags & CHAT_USER_SQUELCHED) {
    pChat->SetSquelch(pUserToSquelch, 0);
    pUserToSquelch->flags &= ~static_cast<unsigned int>(CHAT_USER_SQUELCHED);
    return false;
  }
  pChat->SetSquelch(pUserToSquelch, 1);
  pUserToSquelch->flags |= CHAT_USER_SQUELCHED;
  return true;
}

//***********************************************************************************************
void WolapiObject::DoOptions() {
  //	Show options dialog.
  bPump_In_Call_Back = true;
  WOL_Options_Dialog(this, false);
  bPump_In_Call_Back = false;
  //	Set trigger for an immediate channel list update, in case local lobby
  // games filter was changed.
  dwTimeNextChannelUpdate = Get_Time_Ms();
}

//***********************************************************************************************
bool WolapiObject::DoLadder() {
  bPump_In_Call_Back = true;
  if (WWMessageBox().Process(TXT_WOL_LADDERSHELL, TXT_YES, TXT_NO) == 0) {
    bPump_In_Call_Back = false;
    return SpawnBrowser(
        "http://www.westwood.com/westwoodonline/tournaments/redalert/"
        "index.html");
  }

  bPump_In_Call_Back = false;
  return false;
}

//***********************************************************************************************
bool WolapiObject::DoHelp() {
  bPump_In_Call_Back = true;
  if (WWMessageBox().Process(TXT_WOL_HELPSHELL, TXT_YES, TXT_NO) == 0) {
    bPump_In_Call_Back = false;
    const char* szURL = nullptr;
    if (pChat->GetHelpURL(&szURL) == S_OK) {
      return SpawnBrowser(szURL);
    }
    GenericErrorMessage();
  }

  bPump_In_Call_Back = false;
  return false;
}

//***********************************************************************************************
bool WolapiObject::DoWebRegistration() {
  //	Get the executable name from the registry.
  HKEY hKey = nullptr;
  if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Westwood\\Register", 0,
                   KEY_READ, &hKey) != ERROR_SUCCESS) {
    GenericErrorMessage();
    return false;
  }
  char szPath[port::kMaxPath + 1];
  DWORD dwBufSize = port::kMaxPath;
  if (RegQueryValueEx(hKey, "InstallPath", nullptr, nullptr,
                      port::BytesOf(szPath), &dwBufSize) != ERROR_SUCCESS) {
    GenericErrorMessage();
    return false;
  }
  RegCloseKey(hKey);
  //	debugprint( "Registration app is '%s'\n", szPath );

  bPump_In_Call_Back = true;
  if (WWMessageBox().Process(TXT_WOL_WEBREGISTRATIONSHELL, TXT_YES, TXT_NO) ==
      0) {
    bPump_In_Call_Back = false;
    ::ShellExecute(nullptr, "open", szPath, nullptr, ".", SW_SHOW);
    return true;
  }

  bPump_In_Call_Back = false;
  return false;
}

//***********************************************************************************************
bool WolapiObject::DoGameAdvertising(const Channel* pChannel) {
  const char* szURL = URLForGameType(pChannel->type);
  if (!szURL) {
    GenericErrorMessage();
    return false;
  }

  char szQuestion[512];
  Format_Runtime_Text(szQuestion, sizeof(szQuestion), TXT_WOL_GAMEADVERTSHELL,
                      NameOfGameType(pChannel->type));
  bPump_In_Call_Back = true;
  if (WWMessageBox().Process(szQuestion, TXT_YES, TXT_NO) == 0) {
    bPump_In_Call_Back = false;
    return SpawnBrowser(szURL);
  }

  bPump_In_Call_Back = false;
  return false;
}

//***********************************************************************************************
bool WolapiObject::SpawnBrowser(const char* szURL) {
  //	Attempts to launch user's web browser, and monitors it, waiting for user
  // to close it, at which 	point we bring focus back to the game.

  //	Loosely based on Dune2000 example.

  bool bSuccess = false;
  STARTUPINFO si{};
  PROCESS_INFORMATION pi{};

  si.cb = sizeof(si);

  if (*szWebBrowser) {
    char szCommandLine[port::kMaxPath + 300];
    absl::SNPrintF(szCommandLine, sizeof(szCommandLine), "\"%s\" %s",
                   szWebBrowser, szURL);
    //		debugprint( "About to CreateProcess: '%s'\n", szCommandLine );
    Hide_Mouse();
    BlackPalette.Set(kFadePaletteFast, ServiceRealTime);
    //		::ShowWindow( MainWindow, SW_SHOWMINIMIZED );
    SeenBuff.Clear();
    if (::CreateProcess(nullptr,
                        szCommandLine,  //	Command line.
                        nullptr,        //	Process handle not inheritable.
                        nullptr,        //	Thread handle not inheritable.
                        FALSE,          //	Set handle inheritance to false.
                        0,              //	No creation flags.
                        nullptr,        //	Use parent�s environment block.
                        nullptr,        //	Use parent�s starting directory.
                        &si,            //	Pointer to STARTUPINFO structure.
                        &pi) &&
        pi.hProcess)  //	Pointer to PROCESS_INFORMATION structure.

    {
      //				debugprint( "CreateProcess: '%s'\n",
      // szCommandLine );
      bSuccess = true;
      ::WaitForInputIdle(pi.hProcess, 5000);
      bPump_In_Call_Back = true;
      for (;;) {
        DWORD dwActive = 0;
        ServiceRealTime();
        port::SleepMs(200);
        ::GetExitCodeProcess(pi.hProcess, &dwActive);
        if (dwActive != STILL_ACTIVE || cancel_current_msgbox) {
          //	Either user closed the browser app, or game is starting and we
          // should return focus to game.
          cancel_current_msgbox = false;
          Restore_Game_Window();
          break;
        }
        if (Game_Window_Has_Focus()) {
          Restore_Game_Window();  //	In case it was topmost but minimized.
          break;
        }
      }
      bPump_In_Call_Back = false;
      GamePalette.Set(kFadePaletteFast, ServiceRealTime);
      Show_Mouse();
    }
  }

  if (!bSuccess) {
    //	This was the old way - does not pop you back into game when finished...
    if (::ShellExecute(nullptr, nullptr, szURL, nullptr, ".", SW_SHOW) ==
        nullptr) {
      //			debugprint( "ShellExecute\n" );
      //	ShellExecute failed as well. Just print a message instead.
      GamePalette.Set();
      Restore_Game_Window();
      char szError[300];
      Format_Runtime_Text(szError, sizeof(szError), TXT_WOL_CANTLAUNCHBROWSER,
                          szURL);
      Show_Mouse();
      WWMessageBox().Process(szError);
      return false;
    }
    //	(We return immediately after launching in this case.)
    GamePalette.Set();
    Show_Mouse();
  }
  return true;
}

//***********************************************************************************************
void WolapiObject::ChannelListTitle(const char* szTitle) {
  port::SafeCopy(szChannelListTitle, szTitle);
  bChannelListTitleUpdated = true;
}

//***********************************************************************************************
bool WolapiObject::EnterLevel_Top() {
  //	<Showing the top level choices.>

  //	debugprint( "*** EnterLevel_Top\n" );
  //	(Might as well hardcode the channels tree.)

  ChannelListTitle(TXT_WOL_TOPLEVELTITLE);
  pILChannels->Clear();
  // void* pTopIcon = IconPointer(DibIconInfos[ kDibiconAccept ]);
  void* pTopIcon = IconForGameType(0);
  pILChannels->Add_Item(TXT_WOL_OFFICIALCHAT, CHANNELTYPE_OFFICIALCHAT,
                        pTopIcon, ICON_DIB, CHANNELTYPE_OFFICIALCHAT);
  pILChannels->Add_Item(TXT_WOL_USERCHAT, CHANNELTYPE_USERCHAT, pTopIcon,
                        ICON_DIB, CHANNELTYPE_USERCHAT);
  pILChannels->Add_Item(TXT_WOL_GAMECHANNELS, CHANNELTYPE_GAMES, pTopIcon,
                        ICON_DIB, CHANNELTYPE_GAMES);

  //	Set wol buttons enabled/disabled.
  pShpBtnLeave->Disable();
  pShpBtnRefresh->Disable();
  pShpBtnSquelch->Disable();
  pShpBtnBan->Disable();
  pShpBtnKick->Disable();

  CurrentLevel = WOL_LEVEL_TOP;

  return true;
}

//***********************************************************************************************
bool WolapiObject::EnterLevel_OfficialChat() {
  //	<Showing available official chat channels.>

  //	debugprint( "*** EnterLevel_OfficialChat\n" );
  //	(Might as well hardcode the channels tree.)

  CurrentLevel = WOL_LEVEL_OFFICIALCHAT;
  if (!UpdateChannels(0, CHANNELFILTER_OFFICIAL, false)) {
    GenericErrorMessage();
    return false;
  }

  ChannelListTitle(TXT_WOL_OFFICIALCHAT);
  pILChannels->Clear();
  pILChannels->Add_Item(TXT_WOL_CHANNELLISTLOADING, CHANNELTYPE_LOADING,
                        nullptr, ICON_SHAPE, CHANNELTYPE_LOADING);
  dwTimeNextChannelUpdate = Get_Time_Ms();  //	Set trigger for an
                                            // immediate channel list update.

  //	Set wol buttons enabled/disabled.
  pShpBtnLeave->Disable();
  pShpBtnRefresh->Enable();
  pShpBtnSquelch->Disable();
  pShpBtnBan->Disable();
  pShpBtnKick->Disable();

  return true;
}

//***********************************************************************************************
bool WolapiObject::EnterLevel_UserChat() {
  //	<Showing available user chat channels.>

  //	debugprint( "*** EnterLevel_UserChat\n" );
  //	(Might as well hardcode the channels tree.)

  CurrentLevel = WOL_LEVEL_USERCHAT;
  if (!UpdateChannels(0, CHANNELFILTER_UNOFFICIAL, false)) {
    GenericErrorMessage();
    return false;
  }

  ChannelListTitle(TXT_WOL_USERCHAT);
  pILChannels->Clear();
  pILChannels->Add_Item(TXT_WOL_CHANNELLISTLOADING, CHANNELTYPE_LOADING,
                        nullptr, ICON_SHAPE, CHANNELTYPE_LOADING);
  dwTimeNextChannelUpdate = Get_Time_Ms();  //	Set trigger for an
                                            // immediate channel list update.

  //	Set wol buttons enabled/disabled.
  pShpBtnLeave->Disable();
  pShpBtnRefresh->Enable();
  pShpBtnSquelch->Disable();
  pShpBtnBan->Disable();
  pShpBtnKick->Disable();

  return true;
}

//***********************************************************************************************
bool WolapiObject::EnterLevel_Games() {
  //	<Showing each westwood game type.>

  //	debugprint( "*** EnterLevel_Games\n" );
  //	(Might as well hardcode the channels tree.)

  CurrentLevel = WOL_LEVEL_GAMES;

  ChannelListTitle(TXT_WOL_GAMECHANNELS);
  pILChannels->Clear();
  pILChannels->Add_Item(TXT_WOL_CHANNEL_TOP, CHANNELTYPE_TOP, nullptr,
                        ICON_SHAPE, CHANNELTYPE_TOP);

  //	Create entry for our lobbies at the top.
  bool bFound = false;
  //	(There are actually 2 additional game types at the end of GameTypeInfos
  //- for ws icon and wwonline icon.)
  for (unsigned int i = 0; i + 2 < nGameTypeInfos; i++) {
    if (GameTypeInfos.at(base::ToSize(i)).iGameType == GAME_TYPE) {
      // pILChannels->Add_Item( GameTypeInfos[base::ToSize( i )].szName,
      // CHANNELTYPE_LOBBIES, IconPointer(GameTypeInfos[base::ToSize( i )]),
      // ICON_DIB, CHANNELTYPE_LOBBIES );
      pILChannels->Add_Item(TXT_WOL_REDALERTLOBBIES, CHANNELTYPE_LOBBIES,
                            IconPointer(GameTypeInfos.at(base::ToSize(i))),
                            ICON_DIB, CHANNELTYPE_LOBBIES);
      bFound = true;
      break;
    }
  }
  if (!bFound) {
    //	In the production version, this should never happen, as there should
    // always be a gametypeinfo created that matches 	our game type. It
    // depends on the recentness of the WOR file accompanying the wolapi.dll.
    pILChannels->Add_Item(TXT_WOL_REDALERTLOBBIES, CHANNELTYPE_LOBBIES,
                          IconPointer(base::At(OldRAGameTypeInfos, 0)),
                          ICON_DIB, CHANNELTYPE_LOBBIES);
  }

  //	A pointer to the GameTypeInfos entry is stored in the item for
  // convenience later.
  for (unsigned int i = 0; i + 2 < nGameTypeInfos; i++) {
    const int iType = GameTypeInfos.at(base::ToSize(i)).iGameType;
    if (iType != GAME_TYPE)  //	Else it is our game - skip it here since we put
                             // it at the top.
    {
      if (iType != 2 && iType != 3 &&
          iType != 4)  //	Hack needed for the time being, to prevent the
                       // old ra games from being seen.
      {
        char szHelp[200];
        Format_Runtime_Text(szHelp, sizeof(szHelp),
                            TXT_WOL_TTIP_CHANNELTYPE_GAMESOFTYPE,
                            GameTypeInfos.at(base::ToSize(i)).szName);
        pILChannels->Add_Item(GameTypeInfos.at(base::ToSize(i)).szName, szHelp,
                              IconPointer(GameTypeInfos.at(base::ToSize(i))),
                              ICON_DIB, CHANNELTYPE_GAMESOFTYPE,
                              &GameTypeInfos.at(base::ToSize(i)));
      }
    }
  }

  //	Set wol buttons enabled/disabled.
  pShpBtnLeave->Disable();
  pShpBtnRefresh->Disable();
  pShpBtnSquelch->Disable();
  pShpBtnBan->Disable();
  pShpBtnKick->Disable();

  return true;
}

//***********************************************************************************************
bool WolapiObject::EnterLevel_GamesOfType(WOL_GAMETYPEINFO* pGameTypeInfo) {
  //	<Showing current game channels of a specific type - not our own game
  // type.>

  //	debugprint( "*** EnterLevel_GamesOfType: pGameTypeInfo->szName %s,
  // iGameType %i, URL %s\n", pGameTypeInfo->szName, pGameTypeInfo->iGameType,
  // pGameTypeInfo->szURL );

  CurrentLevel = WOL_LEVEL_GAMESOFTYPE;
  if (!UpdateChannels(pGameTypeInfo->iGameType, CHANNELFILTER_NO, true)) {
    GenericErrorMessage();
    return false;
  }

  ChannelListTitle(pGameTypeInfo->szName);
  pILChannels->Clear();
  pILChannels->Add_Item(TXT_WOL_CHANNELLISTLOADING, CHANNELTYPE_LOADING,
                        nullptr, ICON_SHAPE, CHANNELTYPE_LOADING);
  dwTimeNextChannelUpdate = Get_Time_Ms();  //	Set trigger for an
                                            // immediate channel list update.

  //	Set wol buttons enabled/disabled.
  pShpBtnLeave->Disable();
  pShpBtnRefresh->Enable();
  pShpBtnSquelch->Disable();
  pShpBtnBan->Disable();
  pShpBtnKick->Disable();

  return true;
}

//***********************************************************************************************
bool WolapiObject::EnterLevel_Lobbies() {
  //	<Showing available lobbies.>

  //	debugprint( "*** EnterLevel_Lobbies\n" );

  CurrentLevel = WOL_LEVEL_LOBBIES;
  if (!UpdateChannels(0, CHANNELFILTER_LOBBIES, false)) {
    GenericErrorMessage();
    return false;
  }

  ChannelListTitle(TXT_WOL_REDALERTLOBBIES);
  pILChannels->Clear();
  pILChannels->Add_Item(TXT_WOL_CHANNELLISTLOADING, CHANNELTYPE_LOADING,
                        nullptr, ICON_SHAPE, CHANNELTYPE_LOADING);
  dwTimeNextChannelUpdate = Get_Time_Ms();  //	Set trigger for an
                                            // immediate channel list update.

  //	Set wol buttons enabled/disabled.
  pShpBtnLeave->Disable();
  pShpBtnRefresh->Enable();
  pShpBtnSquelch->Disable();
  pShpBtnBan->Disable();
  pShpBtnKick->Disable();

  return true;
}

//***********************************************************************************************
bool WolapiObject::OnEnteringChatChannel(const char* szChannelName,
                                         bool bICreatedChannel, int iLobby) {
  //	Called when a chat channel (or lobby) has been successfully joined.
  //	debugprint( "*** OnEnteringChatChannel '%s'\n", szChannelName );

  //	//	Block until we have a userlist.		- Not necessary - always
  // comes immediately following OnJoin. 	if( !UserList() )
  // return false;

  //	Request ladders if this is a lobby.
  if (iLobby != -1) {
    RequestLadders(nullptr);
  }

  //	Set channels list.
  pILChannels->Clear();
  //	Add a "return" choice at the top of the channel list, based on where we
  // want to go 'back' to...
  if (iLobby == -1) {
    switch (CurrentLevel) {
      case WOL_LEVEL_OFFICIALCHAT:
        pILChannels->Add_Item(TXT_WOL_CHANNEL_BACK, CHANNELTYPE_OFFICIALCHAT,
                              nullptr, ICON_SHAPE, CHANNELTYPE_OFFICIALCHAT);
        break;
      case WOL_LEVEL_USERCHAT:
      case WOL_LEVEL::WOL_LEVEL_TOP:
      case WOL_LEVEL::WOL_LEVEL_INCHATCHANNEL:
      case WOL_LEVEL::WOL_LEVEL_GAMES:
      case WOL_LEVEL::WOL_LEVEL_GAMESOFTYPE:
      case WOL_LEVEL::WOL_LEVEL_INGAMECHANNEL:
      case WOL_LEVEL::WOL_LEVEL_LOBBIES:
      case WOL_LEVEL::WOL_LEVEL_INLOBBY:
      case WOL_LEVEL::WOL_LEVEL_INVALID:
      default:
        //	If entering a channel from anywhere else, user must have created
        // the channel. 	Make "back" take them to user channels list.
        //			if( bICreatedChannel )		//	ajw just
        // verifying
        pILChannels->Add_Item(TXT_WOL_CHANNEL_BACK, CHANNELTYPE_USERCHAT,
                              nullptr, ICON_SHAPE, CHANNELTYPE_USERCHAT);
        /*			else
                                {
        //				debugprint( "Case that should not occur
        in OnEnteringChatChannel. CurrentLevel %i\n", CurrentLevel );
                                        pILChannels->Add_Item( "ERROR in
        OnEnteringChatChannel", NULL, NULL, ICON_SHAPE, CHANNELTYPE_TOP );
                                }
        */
        break;
    }
  } else {
    pILChannels->Add_Item(TXT_WOL_CHANNEL_BACK, CHANNELTYPE_LOBBIES, nullptr,
                          ICON_SHAPE, CHANNELTYPE_LOBBIES);
  }

  std::string szMess;
  if (iLobby == -1) {
    CurrentLevel = WOL_LEVEL_INCHATCHANNEL;
    szMess = port::FormatRuntime(TXT_WOL_YOUJOINED, szChannelName);
    ChannelListTitle(szChannelName);
  } else {
    CurrentLevel = WOL_LEVEL_INLOBBY;
    char szLobbyName[REASONABLELOBBYINTERPRETEDNAMELEN];
    InterpretLobbyNumber(szLobbyName, iLobby);
    szMess = port::FormatRuntime(TXT_WOL_YOUJOINEDLOBBY, szLobbyName);
    ChannelListTitle(szLobbyName);
    iLobbyLast = iLobby;
    dwTimeNextChannelUpdate = Get_Time_Ms();  //	Set trigger for an
                                              // immediate channel list update.
  }

  port::SafeCopy(szChannelNameCurrent, szChannelName);

  bChannelOwner = bICreatedChannel;

  //	Set users list.
  ListChannelUsers();

  PrintMessage(szMess.c_str(), WOLCOLORREMAP_LOCALMACHINEMESS);

  PlaySoundEffect(WOLSOUND_ENTERCHAN);

  //	Set wol buttons enabled/disabled.
  pShpBtnLeave->Enable();
  if (CurrentLevel == WOL_LEVEL_INLOBBY) {
    pShpBtnRefresh->Enable();
  } else {
    pShpBtnRefresh->Disable();
  }
  pShpBtnSquelch->Enable();
  if (bChannelOwner) {
    pShpBtnBan->Enable();
    pShpBtnKick->Enable();
  } else {
    pShpBtnBan->Disable();
    pShpBtnKick->Disable();
  }

  return true;
}

//***********************************************************************************************
void WolapiObject::OnExitingChatChannel() {
  //	Called when we successfully ExitChannel, or we get kicked out. (Lobbies
  // included.)

  //	Clear users list.
  pILUsers->Clear();
  if (pStaticUsers) {
    pStaticUsers->Set_Text(TXT_WOL_NOUSERLIST);
  }

  //	debugprint( "*** OnExitingChatChannel() - szChannelNameCurrent '%s',
  // CurrentLevel %i\n", szChannelNameCurrent, CurrentLevel );
  const int iLobby = iChannelLobbyNumber(szChannelNameCurrent);
  std::string szMess;
  if (iLobby == -1) {
    szMess = port::FormatRuntime(TXT_WOL_YOULEFT, szChannelNameCurrent);
  } else {
    //	Channel is a lobby.
    char szLobbyName[REASONABLELOBBYINTERPRETEDNAMELEN];
    InterpretLobbyNumber(szLobbyName, iLobby);
    szMess = port::FormatRuntime(TXT_WOL_YOULEFTLOBBY, szLobbyName);
  }
  PrintMessage(szMess.c_str(), WOLCOLORREMAP_LOCALMACHINEMESS);

  *szChannelNameCurrent = 0;
  CurrentLevel = WOL_LEVEL_INVALID;

  PlaySoundEffect(WOLSOUND_EXITCHAN);
}

//***********************************************************************************************
bool WolapiObject::ExitChatChannelForGameChannel() {
  //	We are about to try and join/create a game channel, and are currently in
  // a chat channel.

  //	Save this channel name, so we can come back to it if game channel
  // join/create fails.
  port::SafeCopy(szChannelReturnOnGameEnterFail, szChannelNameCurrent);

  if (!ChannelLeave()) {
    GenericErrorMessage();
    return false;
  }
  return true;
}

//***********************************************************************************************
bool WolapiObject::OnEnteringGameChannel(const char* szChannelName,
                                         bool bICreatedChannel,
                                         const CREATEGAMEINFO& CreateGameInfo) {
  //	Called when a game channel has been successfully joined, while still in
  // chat dialog, 	before game dialog has been created. 	CreateGameInfo
  // is copied to GameInfoCurrent, so that we know what kind of a game we're in
  // during setup.

  //	debugprint( "*** OnEnteringGameChannel() - %s\n", szChannelName );

  CurrentLevel = WOL_LEVEL_INGAMECHANNEL;
  port::SafeCopy(szChannelNameCurrent, szChannelName);

  bChannelOwner = bICreatedChannel;
  //	GameKindCurrent = GameKind;
  GameInfoCurrent = CreateGameInfo;
  port::SafeCopy(GameInfoCurrent.szPassword, CreateGameInfo.szPassword);

  //	Remove shared buttons from wolchat's command list.
  pShpBtnDiscon->Zap();
  pShpBtnLeave->Zap();
  pShpBtnRefresh->Zap();
  pShpBtnSquelch->Zap();
  pShpBtnBan->Zap();
  pShpBtnKick->Zap();
  pShpBtnFindpage->Zap();
  pShpBtnOptions->Zap();
  pShpBtnLadder->Zap();
  pShpBtnHelp->Zap();

  //	Set wol buttons enabled/disabled.
  pShpBtnLeave->Enable();
  pShpBtnRefresh->Disable();
  pShpBtnSquelch->Enable();
  if (bChannelOwner) {
    pShpBtnBan->Enable();
    pShpBtnKick->Enable();
  } else {
    pShpBtnBan->Disable();
    pShpBtnKick->Disable();
  }

  if (CreateGameInfo.GameKind == CREATEGAMEINFO::AMGAME) {
    if (bShowRankRA) {
      //	Switch to "show AM rankings" mode.
      bShowRankRA = false;
      bMyRecordUpdated = true;
      bShowRankUpdated = true;
    }
  } else {
    if (!bShowRankRA) {
      //	Switch to "show RA rankings" mode.
      bShowRankRA = true;
      bMyRecordUpdated = true;
      bShowRankUpdated = true;
    }
  }

  return true;
}

//***********************************************************************************************
bool WolapiObject::OnEnteringGameSetup() {
  //	Called when entering the game setup screen. Controls are initialized.
  // OnEnteringGameChannel 	has just been called earlier.

  //	Returns false only if we find there is not host - he must have
  // simultaneously left.

  //	//	Block until we have a userlist.		- Not necessary - always
  // comes immediately following OnJoin. 	if( !UserList() )
  // return false;

  //	Request ladders.
  RequestLadders(nullptr);

  //	Request IP addresses.
  RequestIPs(nullptr);

  //	Set users list.
  if (!ListChannelUsers()) {
    //	No host was found currently in channel!
    return false;
  }

  if (!pGSupDlg->bHost) {
    char szHostName[WOL_NAME_LEN_MAX];
    HostNameFromGameChannelName(szHostName, szChannelNameCurrent);
    const std::string szMess =
        port::FormatRuntime(TXT_WOL_YOUJOINEDGAME, szHostName);
    PrintMessage(szMess.c_str(), WOLCOLORREMAP_LOCALMACHINEMESS);

  } else {
    PrintMessage(TXT_WOL_YOUCREATEDGAME, WOLCOLORREMAP_LOCALMACHINEMESS);
  }

  return true;
}

//***********************************************************************************************
void WolapiObject::OnFailedToEnterGameChannel() {
  if (*szChannelReturnOnGameEnterFail == 0) {
    return;
  }

  //	This is called when we fail to join/create a game channel.
  *szChannelNameCurrent = 0;

  //	Because we don't save the channel key as well, assume the usual lobby
  // password. If we fail, we'll return to top level.
  const HRESULT hRes =
      ChannelJoin(szChannelReturnOnGameEnterFail, LOBBYPASSWORD);
  if (hRes == S_OK) {
    OnEnteringChatChannel(szChannelReturnOnGameEnterFail, false,
                          iChannelLobbyNumber(szChannelReturnOnGameEnterFail));
  } else {
    //	ChannelJoin returned fail value.
    //	(Now only applies if you could ever enter a game channel from a
    // non-lobby.) 	There is the possibility that the channel we were in
    // disappeared in the instant between leaving it and 	failing to join
    // the game channel. <sigh> Or, the channel has a password, that we didn't
    // record. In either 	case, go back to the top level.
    GenericErrorMessage();
    EnterLevel_Top();
  }
}

//***********************************************************************************************
void WolapiObject::OnExitingGameChannel() {
  //	This is called after we leave a game channel, while still in the game
  // setup dialog.

  //	Remove shared buttons from wolgsup's command list.
  pShpBtnDiscon->Zap();
  pShpBtnLeave->Zap();
  pShpBtnRefresh->Zap();
  pShpBtnSquelch->Zap();
  pShpBtnBan->Zap();
  pShpBtnKick->Zap();
  pShpBtnFindpage->Zap();
  pShpBtnOptions->Zap();
  pShpBtnLadder->Zap();
  pShpBtnHelp->Zap();

  CurrentLevel = WOL_LEVEL_INVALID;
  *szChannelNameCurrent = 0;
}

//***********************************************************************************************
void WolapiObject::RejoinLobbyAfterGame() {
  //	Called to rejoin lobby after EITHER a game, or the game setup dialog.
  // debugprint( "RejoinLobbyAfterGame, iLobbyReturnAfterGame is %i\n",
  // iLobbyReturnAfterGame );

  if (iLobbyReturnAfterGame == -1) {
    //	Will never happen presumably, if games are always entered via a lobby
    // chat channel. 	We will naturally reenter the top level.
  } else {
    char szChannelToJoin[WOL_CHANNAME_LEN_MAX];
    // absl::SNPrintF(szChannelToJoin, sizeof(szChannelToJoin), "Lob_%i_%i",
    // GAME_TYPE, iLobbyReturnAfterGame
    // );
    absl::SNPrintF(szChannelToJoin, sizeof(szChannelToJoin), "%s%i", LOB_PREFIX,
                   iLobbyReturnAfterGame);
    // debugprint( "RejoinLobbyAfterGame, channel is %s\n", szChannelToJoin );

    const HRESULT hRes = ChannelJoin(szChannelToJoin, LOBBYPASSWORD);
    if (hRes == S_OK) {
      // OnEnteringChatChannel( szChannelToJoin, false );		Done
      // automatically now in wol_chat.
    } else {
      //	Something went wrong when trying to rejoin the lobby we were in.
      //	We'll go back to the top level instead, which happens
      // automatically if we do this...
      iLobbyReturnAfterGame = -1;
    }
  }
}

//***********************************************************************************************
bool WolapiObject::RequestLadders(const char* szName) {
  //	If szName is NULL, calls RequestLadderList() until all ladder structs
  // for all users in pChatSink's current 	list have been asked for. Does
  // not wait for results - these come in asynchronously. The previous list is
  // erased before new results come in. 	If szName is valid, asks for
  // specific name only. Result is appended to current ladder list. 	This
  // function does not block.

  if (szName && *szName) {
    //		debugprint( "RequestLadderList( %s )\n", szName );
    if (!SUCCEEDED(pNetUtil->RequestLadderList(szLadderServerHost,
                                               iLadderServerPort, szName,
                                               LADDER_CODE_RA, -1, 0, 0))) {
      //			debugprint( "RequestLadderList() call failed\n"
      //);
      return false;
    }
    if (!SUCCEEDED(pNetUtil->RequestLadderList(szLadderServerHost,
                                               iLadderServerPort, szName,
                                               LADDER_CODE_AM, -1, 0, 0))) {
      //			debugprint( "RequestLadderList() call failed\n"
      //);
      return false;
    }
    return true;
  }

  char szNames[(WOL_NAME_LEN_MAX + 1) *
               30];  //	Neal says max is actually 25
                     // names requested at once. Do 24...

  pNetUtilSink->DeleteLadderList();

  //	Do not request more than this number of times, to prevent overloads to
  // ladder server. 	If we have that many people in the channel, forget about
  // doing ladders for all of them. 	Probably this will never come into play
  //(except while testing), because lobbies will be limited in # of users.
  int iCallLimit = 4;

  User* pUser = pChatSink->pUserList;
  while (pUser) {
    //	Reset names string.
    *szNames = 0;
    //	Get 24 users from list and add names to string.
    for (int i = 0; i != 24; ++i) {
      port::SafeAppend(szNames, WolText(pUser->name));
      port::SafeAppend(szNames, ":");
      pUser = pUser->next;
      if (!pUser) {
        break;
      }
    }
    //	Remove last colon.
    base::At(szNames, std::string_view(szNames).size() - 1) = 0;

    //		debugprint( "RequestLadderList( %s )\n", szNames );
    if (!SUCCEEDED(pNetUtil->RequestLadderList(szLadderServerHost,
                                               iLadderServerPort, szNames,
                                               LADDER_CODE_RA, -1, 0, 0))) {
      //			debugprint( "RequestLadderList() call failed\n"
      //);
      return false;
    }
    if (!SUCCEEDED(pNetUtil->RequestLadderList(szLadderServerHost,
                                               iLadderServerPort, szNames,
                                               LADDER_CODE_AM, -1, 0, 0))) {
      //			debugprint( "RequestLadderList() call failed\n"
      //);
      return false;
    }
    if (--iCallLimit == 0) {
      return false;
    }
  }
  return true;
}

//***********************************************************************************************
// Not const: sends a chat request.
// NOLINTNEXTLINE(readability-make-member-function-const)
bool WolapiObject::RequestIPs(const char* szName) {
  //	If szName is NULL, calls RequestUserIP() until IPs for all users in
  // pChatSink's current 	list have been asked for. Does not wait for
  // results - these come in asynchronously. The previous list is 	erased
  // before new results come in. 	If szName is valid, asks for specific
  // name only. Result is appended to current IP list. 	This function does not
  // block.

  if (szName && *szName) {
    User user;
    port::SafeCopy(WolTextBuffer(user.name), szName);
    //		debugprint( "RequestUserIP( %s )\n", szName );
    //		debugprint( "RequestUserIP() call failed\n" ) on failure.
    return SUCCEEDED(pChat->RequestUserIP(&user));
  }

  //	Do all users in current chatsink list.

  pChatSink
      ->DeleteUserIPList();  //	Clear old user IPs. (To keep searches fast, if
                             // we go in and out of game channels a lot.)

  User* pUser = pChatSink->pUserList;
  while (pUser) {
    if ((!(pUser->flags & CHAT_USER_MYSELF)) &&
        (!SUCCEEDED(pChat->RequestUserIP(pUser)))) {
      //				debugprint( "RequestUserIP() call
      // failed\n" );
      return false;
    }

    pUser = pUser->next;
  }
  return true;
}

//***********************************************************************************************
void WolapiObject::SaveChat() {
  //	Basically, a big hack to avoiding restructuring things so that the
  // dialogs are persistent 	objects. Save the contents of the chat list in
  // the chat dialog so that we can refresh it 	after returning from the game
  // setup dialog (if necessary). 	This turns out to be the easiest and
  // most straightforward way to implement this.
  pChatSaveLast = pChatSaveList = nullptr;

  for (int i = 0; i != pILChat->Count(); i++) {
    auto* pChatSaveNew = new CHATSAVE;
    const char* szItem = pILChat->Get_Item(i);
    if (std::string_view(szItem).size() < SAVECHATWIDTH) {
      port::SafeCopy(pChatSaveNew->szText, szItem);
    }
    const IconList_ItemExtras* pItemExtras = pILChat->Get_ItemExtras(i);
    pChatSaveNew->ItemExtras.pColorRemap = pItemExtras->pColorRemap;
    pChatSaveNew->next = nullptr;

    if (pChatSaveLast) {
      pChatSaveLast->next = pChatSaveNew;
    } else {
      pChatSaveList = pChatSaveNew;
    }
    pChatSaveLast = pChatSaveNew;
  }
}

//***********************************************************************************************
void WolapiObject::RestoreChat() {
  //	See SaveChat()...
  CHATSAVE* pChatSave = pChatSaveList;
  while (pChatSave) {
    PrintMessage(pChatSave->szText, pChatSave->ItemExtras.pColorRemap);
    pChatSave = pChatSave->next;
  }
}

//***********************************************************************************************
void WolapiObject::AddHostLeftMessageToSavedChat(const char* szName) {
  auto* pChatSaveNew = new CHATSAVE;
  Format_Runtime_Text(pChatSaveNew->szText, sizeof(pChatSaveNew->szText),
                      TXT_WOL_HOSTLEFTGAME, szName);
  pChatSaveNew->ItemExtras.pColorRemap =
      &ColorRemaps.at(WOLCOLORREMAP_LOCALMACHINEMESS);
  pChatSaveNew->next = nullptr;
  if (pChatSaveLast) {
    pChatSaveLast->next = pChatSaveNew;
  } else {
    pChatSaveList = pChatSaveNew;
  }
  pChatSaveLast = pChatSaveNew;
}

//***********************************************************************************************
void WolapiObject::AddMessageToSavedChat(const char* szMessage) {
  auto* pChatSaveNew = new CHATSAVE;
  port::SafeCopy(pChatSaveNew->szText, szMessage);
  pChatSaveNew->ItemExtras.pColorRemap =
      &ColorRemaps.at(WOLCOLORREMAP_LOCALMACHINEMESS);
  pChatSaveNew->next = nullptr;
  if (pChatSaveLast) {
    pChatSaveLast->next = pChatSaveNew;
  } else {
    pChatSaveList = pChatSaveNew;
  }
  pChatSaveLast = pChatSaveNew;
}

//***********************************************************************************************
void WolapiObject::DeleteSavedChat() {
  //	See SaveChat()...
  while (pChatSaveList) {
    CHATSAVE* pChatSaveNext = pChatSaveList->next;
    delete pChatSaveList;
    pChatSaveList = pChatSaveNext;
  }
}

//***********************************************************************************************
void WolapiObject::GenericErrorMessage() {
  //	Displays generic "something bad happened" error message.
  bPump_In_Call_Back = true;
  WWMessageBox().Process(TXT_WOL_ERRORMESSAGE);
  bPump_In_Call_Back = false;
}

//***********************************************************************************************
bool WolapiObject::GetNameOfBeginningLobby(std::span<char> szNameToSet) {
  //	Checks for game lobbies, sets szNameToSet to the channel name that the
  // new user should enter and returns true if succeeds.
  if (!GetLobbyChannels()) {
    return false;
  }

  //	Chatsink should now have a list of lobbies.
  int iCount = 0;
  Channel* pChannel = pChatSink->pChannelList;
  if (!pChannel) {
    //	List is empty.
    return false;
  }

  //	Return the name of the first lobby with less than 50 users.
  while (pChannel) {
    if (pChannel->currentUsers < 50) {
      port::SafeCopy(szNameToSet, WolText(pChannel->name));
      return true;
    }
    ++iCount;
    pChannel = pChannel->next;
  }

  //	All lobbies have 50 or more users. So just choose a random one.
  const int iChoice = Sim_Random_Pick(0, iCount - 1);
  pChannel = pChatSink->pChannelList;
  // Also stop at the tail, in case the list shrank since it was counted.
  for (int i = 0; i != iChoice && pChannel->next != nullptr; i++) {
    pChannel = pChannel->next;
  }

  port::SafeCopy(szNameToSet, WolText(pChannel->name));

  return true;
}

//***********************************************************************************************
// Not const: sends a chat request and waits for the reply.
// NOLINTNEXTLINE(readability-make-member-function-const)
bool WolapiObject::GetLobbyChannels() {
  //	Modal version of UpdateChannels, for fetching lobby names.

  //	//	Returns false upon total failure.	ajxxx do same for other
  // calls 	WWMessageBox().Process( TXT_WOL_WAIT, TXT_NONE );

  pChatSink->bRequestChannelListForLobbiesWait = true;
  pChatSink->ChannelFilter = CHANNELFILTER_LOBBIES;

  //	debugprint( "RequestChannelList() for lobbies\n" );
  if (!SUCCEEDED(pChat->RequestChannelList(0, 0))) {
    //		debugprint( "RequestChannelList() call failed\n" );
    return false;
  }

  const DWORD dwTimeStart = Get_Time_Ms();
  DWORD dwTimeNextPump = Get_Time_Ms() + PUMPSLEEPDURATION;
  while (pChatSink->bRequestChannelListForLobbiesWait &&
         Get_Time_Ms() - dwTimeStart < EMERGENCY_TIMEOUT) {
    while (Get_Time_Ms() < dwTimeNextPump) {
      ServiceRealTime();
    }
    pChat->PumpMessages();
    dwTimeNextPump = Get_Time_Ms() + PUMPSLEEPDURATION;
  }

  return !pChatSink->bRequestChannelListForLobbiesWait;
}

//***********************************************************************************************
const char* WolapiObject::pGameHostName() {
  //	Returns a POINTER (careful - temporary!) to the name of the creator of
  // the game channel we're in, or null. 	Uses players' list as its means
  // of reference.
  if (pILPlayers) {
    for (int i = 0; i != pILPlayers->Count(); i++) {
      const User* pUser =
          static_cast<const User*>(pILPlayers->Get_Item_ExtraDataPtr(i));
      if (pUser && pUser->flags & CHAT_USER_CHANNELOWNER) {
        return WolText(pUser->name);
      }
    }
  }
  return nullptr;
}

//***********************************************************************************************
User* WolapiObject::pGameHost() {
  //	Returns a POINTER (careful - temporary!) to the creator of the game
  // channel we're in, or null. 	Uses players' list as its means of
  // reference.
  if (pILPlayers) {
    for (int i = 0; i != pILPlayers->Count(); i++) {
      User* pUser = static_cast<User*>(pILPlayers->Get_Item_ExtraDataPtr(i));
      if (pUser && pUser->flags & CHAT_USER_CHANNELOWNER) {
        return pUser;
      }
    }
  }
  return nullptr;
}

//***********************************************************************************************
// Not const: sends a chat request.
// NOLINTNEXTLINE(readability-make-member-function-const)
bool WolapiObject::SendGameOpt(const char* szSend, User* pUserPriv) {
  //	Used during game setup to send public or private game options string.
  //	If pUserPriv is NULL, message is public, else private to pUserPriv.
  if (!pUserPriv) {
    //		debugprint( "Send public game opt: '%s'\n", szSend );
    if (!SUCCEEDED(pChat->RequestPublicGameOptions(szSend))) {
      //			debugprint( "RequestPublicGameOptions() call
      // failed\n" );
      return false;
    }
  } else {
    //		debugprint( "Send private game opt to %s: '%s'\n",
    //(char*)pUserPriv->name, szSend );
    if (!SUCCEEDED(pChat->RequestPrivateGameOptions(pUserPriv, szSend))) {
      //			debugprint( "RequestPrivateGameOptions() call
      // failed\n" );
      return false;
    }
  }
  return true;
}

//***********************************************************************************************
// Not const: sends a chat request and waits for the reply.
// NOLINTNEXTLINE(readability-make-member-function-const)
bool WolapiObject::RequestGameStart() {
  //	Host is starting a game.

  /*
          //	Block any users that join the channel in the next microsecond
     from becoming involved, and
          //	block any users that leave from being recognized as having left.
          //what if someone leaves?
          //	This is done to preserve the integrity of the ChatSink's user
     list pWO->pChatSink->bIgnoreJoin = true;
  */
  pChatSink->bRequestGameStartWait = true;

  //	debugprint( "RequestGameStart()\n" );
  if (!SUCCEEDED(pChat->RequestGameStart(pChatSink->pUserList))) {
    //		debugprint( "RequestGameStart() call failed\n" );
    return false;
  }

  const DWORD dwTimeStart = Get_Time_Ms();
  DWORD dwTimeNextPump = Get_Time_Ms() + PUMPSLEEPDURATION;
  while (pChatSink->bRequestGameStartWait &&
         Get_Time_Ms() - dwTimeStart < EMERGENCY_TIMEOUT) {
    while (Get_Time_Ms() < dwTimeNextPump) {
      ServiceRealTime();
    }
    pChat->PumpMessages();
    dwTimeNextPump = Get_Time_Ms() + PUMPSLEEPDURATION;
  }

  if (pChatSink->bRequestGameStartWait) {
    //		debugprint( "WolapiObject::RequestGameStart returning false\n"
    //);
    pChatSink->bRequestGameStartWait = false;
    return false;
  }

  //	debugprint( "WolapiObject::RequestGameStart returning true\n" );
  return true;
}

//***********************************************************************************************
// Not const: sends a chat request.
// NOLINTNEXTLINE(readability-make-member-function-const)
bool WolapiObject::SendGo(const char* szSend) {
  //	Send a "GO" message to all players included in the list that came back
  // from OnGameStart. 	(Don't just broadcast it. We don't want to include any
  // users that may have joined the channel 	in the last microsecond.)
  //	debugprint( "SendGo()\n" );

  User* pUser = pChatSink->pGameUserList;

  while (pUser) {
    //		if( !( pUser->flags & CHAT_USER_MYSELF ) )
    // Method changed. I now wait for go message to bounce back to me.
    //		{
    //			debugprint( "Send private game opt to %s: '%s'\n",
    //(char*)pUser->name, szSend );
    if (!SUCCEEDED(pChat->RequestPrivateGameOptions(pUser, szSend))) {
      //				debugprint( "RequestPrivateGameOptions()
      // call failed\n" );
      return false;
    }
    //		}
    pUser = pUser->next;
  }
  return true;
}

//***********************************************************************************************
void WolapiObject::Init_DisconnectPinging() {
  //	Sets us up to begin "disconnect pinging" - the pinging that occurs when
  // connection is broken 	during a tournament game. The idea is to try and
  // figure out who is responsible for the connection 	going down. We do this
  // by repeatedly pinging the opponent and the game results server. The number
  // of successful pings is sent in the game results package.
  iDisconnectPingCurrent = 0;
  for (int i = 0; i != DISCONNECT_PING_COUNT; ++i) {
    base::At(DisconnectPingResult_Server, i) = PING_UNSTARTED;
    base::At(DisconnectPingResult_Opponent, i) = PING_UNSTARTED;
  }
  bDisconnectPingingCompleted = false;
  bDoingDisconnectPinging = true;
}

//***********************************************************************************************
bool WolapiObject::Pump_DisconnectPinging() {
  //	Called repeatedly and continuously when it seems a tournament game
  // connection with the opponent 	has been broken. Does PumpMessages() and
  // requests new pings when previous results have been received. 	Returns
  // true when the required number of pings have been completed.

  if (Get_Time_Ms() > dwTimeNextWolapiPump) {
    pChat->PumpMessages();
    pNetUtil->PumpMessages();
    dwTimeNextWolapiPump = Get_Time_Ms() + WOLAPIPUMPWAIT;
  }

  switch (base::At(DisconnectPingResult_Server, iDisconnectPingCurrent)) {
    case PING_UNSTARTED: {
      //	Pings have yet to be requested.
      //	Ping game results server.
      int iUnused = 0;
      if (*szGameResServerHost1) {
        //			debugprint( "RequestPing ( gameres server )\n"
        //);
        if (pNetUtil->RequestPing(szGameResServerHost1, 1000, &iUnused) !=
            S_OK) {
          //				debugprint( "RequestPing() ( gameres
          // server ) failed\n" );
          base::At(DisconnectPingResult_Server, iDisconnectPingCurrent) =
              PING_BAD;
        }
        base::At(DisconnectPingResult_Server, iDisconnectPingCurrent) =
            PING_WAITING;
      } else {
        //	We never got an address for the gameresults server. Fake fail
        // result.
        base::At(DisconnectPingResult_Server, iDisconnectPingCurrent) =
            PING_BAD;
      }

      //	Ping opponent.
      in_addr inaddr{};
      inaddr.s_addr = TournamentOpponentIP;
      const std::string szIP = port::Ipv4Text(inaddr);
      //		debugprint( "RequestPing ( opponent )\n" );
      if (pNetUtil->RequestPing(szIP.c_str(), 1000, &iUnused) != S_OK) {
        //			debugprint( "RequestPing() ( opponent )
        // failed\n" );
        base::At(DisconnectPingResult_Opponent, iDisconnectPingCurrent) =
            PING_BAD;
      } else {
        base::At(DisconnectPingResult_Opponent, iDisconnectPingCurrent) =
            PING_WAITING;
      }
      break;
    }
    case PING_WAITING:
      //	Ping results still pending. (Callback will set vars when results
      // arrive.)
      break;
    case DISCONNECT_PING_STATUS::PING_GOOD:
    case DISCONNECT_PING_STATUS::PING_BAD:
    default:
      //	Ping result for server is in.
      if (base::At(DisconnectPingResult_Opponent, iDisconnectPingCurrent) ==
          PING_WAITING) {
        break;
      }
      //	Both results are in. Begin new ping, or end disconnect pinging.
      iDisconnectPingCurrent++;
      if (iDisconnectPingCurrent == DISCONNECT_PING_COUNT) {
        bDisconnectPingingCompleted = true;
        bDoingDisconnectPinging = false;
        return true;
      }
      break;
  }
  return false;
}

//***********************************************************************************************
void WolapiObject::DisconnectPingResultsString(char* szStringToSet) {
  int iGoodServerPings = 0;
  int iGoodPlayerPings = 0;
  for (int i = 0; i < DISCONNECT_PING_COUNT; ++i) {
    if (base::At(DisconnectPingResult_Server, i) == PING_GOOD) {
      ++iGoodServerPings;
    }
    if (base::At(DisconnectPingResult_Opponent, i) == PING_GOOD) {
      ++iGoodPlayerPings;
    }
  }

  // The caller's buffer holds "x/y a/b" with single-digit counts.
  constexpr size_t kResultSize = 8;
  absl::SNPrintF(szStringToSet, kResultSize, "%1i/%1i %1i/%1i",
                 iGoodServerPings, DISCONNECT_PING_COUNT, iGoodPlayerPings,
                 DISCONNECT_PING_COUNT);
}

//***********************************************************************************************
void WolapiObject::SetOptionDefaults() {
  //	Get stored defaults for options.
  HKEY hKey = nullptr;
  if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, Game_Registry_Key(), 0, KEY_READ,
                   &hKey) == ERROR_SUCCESS) {
    DWORD dwValue = 0;
    DWORD dwBufSize = sizeof(DWORD);
    if (RegQueryValueEx(hKey, "WOLAPI Find Enabled", nullptr, nullptr,
                        port::BytesOf(dwValue), &dwBufSize) != ERROR_SUCCESS) {
      bFindEnabled = true;
    } else {
      bFindEnabled = dwValue != 0;
    }
    if (RegQueryValueEx(hKey, "WOLAPI Page Enabled", nullptr, nullptr,
                        port::BytesOf(dwValue), &dwBufSize) != ERROR_SUCCESS) {
      bPageEnabled = true;
    } else {
      bPageEnabled = dwValue != 0;
    }
    if (RegQueryValueEx(hKey, "WOLAPI Lang Filter", nullptr, nullptr,
                        port::BytesOf(dwValue), &dwBufSize) != ERROR_SUCCESS) {
      bLangFilter = true;
    } else {
      bLangFilter = dwValue != 0;
    }
    if (RegQueryValueEx(hKey, "WOLAPI Show All Games", nullptr, nullptr,
                        port::BytesOf(dwValue), &dwBufSize) != ERROR_SUCCESS) {
      bAllGamesShown = true;
    } else {
      bAllGamesShown = dwValue != 0;
    }

    RegCloseKey(hKey);
  }
  pChat->SetFindPage(bFindEnabled ? 1 : 0, bPageEnabled ? 1 : 0);
  pChat->SetLangFilter(bLangFilter ? 1 : 0);
}

//***********************************************************************************************
void WolapiObject::SetOptions(bool bEnableFind, bool bEnablePage,
                              bool bLangFilterOn, bool bShowAllGames) {
  //	Set options and remember them in registry.

  bFindEnabled = bEnableFind;
  bPageEnabled = bEnablePage;
  bLangFilter = bLangFilterOn;
  bAllGamesShown = bShowAllGames;

  HKEY hKey = nullptr;
  if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, Game_Registry_Key(), 0, KEY_WRITE,
                   &hKey) == ERROR_SUCCESS) {
    DWORD dwValue = bFindEnabled ? 1 : 0;
    RegSetValueEx(hKey, "WOLAPI Find Enabled", 0, REG_DWORD,
                  port::BytesOf(dwValue), sizeof(dwValue));
    dwValue = bPageEnabled ? 1 : 0;
    RegSetValueEx(hKey, "WOLAPI Page Enabled", 0, REG_DWORD,
                  port::BytesOf(dwValue), sizeof(dwValue));
    dwValue = bLangFilter ? 1 : 0;
    RegSetValueEx(hKey, "WOLAPI Lang Filter", 0, REG_DWORD,
                  port::BytesOf(dwValue), sizeof(dwValue));
    dwValue = bAllGamesShown ? 1 : 0;
    RegSetValueEx(hKey, "WOLAPI Show All Games", 0, REG_DWORD,
                  port::BytesOf(dwValue), sizeof(dwValue));

    RegCloseKey(hKey);
  }
  pChat->SetFindPage(bFindEnabled ? 1 : 0, bPageEnabled ? 1 : 0);
  pChat->SetLangFilter(bLangFilter ? 1 : 0);
}

//***********************************************************************************************
std::array<dib::Color, dib::kPaletteSize> CurrentScreenPalette() {
  // Was: ask DirectDraw for the primary surface's palette, pull its 256
  // entries out and hand them to CreatePalette so GDI could match against
  // them. The game keeps those same 256 colours in
  // PaletteClass::CurrentPalette, so there is nobody left to ask.
  //
  // RGBClass stores the VGA's six bits per channel, which is the space
  // dib::RemapToPalette matches in, so the values go across unscaled.
  std::array<dib::Color, dib::kPaletteSize> Palette = {};
  for (int i = 0; i != dib::kPaletteSize; i++) {
    const RGBClass& Entry = PaletteClass::CurrentPalette.at(i);
    dib::Color& color = Palette.at(base::ToSize(i));
    color.red = static_cast<std::uint8_t>(Entry.Red_Component());
    color.green = static_cast<std::uint8_t>(Entry.Green_Component());
    color.blue = static_cast<std::uint8_t>(Entry.Blue_Component());
  }
  return Palette;
}

//***********************************************************************************************
void HostNameFromGameChannelName(std::span<char> szNameToSet,
                                 const char* szChannelName) {
  const std::string_view channel(szChannelName);
  port::SafeCopy(szNameToSet, channel.substr(0, channel.find('\'')));
}
