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

/* this ALWAYS GENERATED file contains the definitions for the interfaces */

/* File created by MIDL compiler version 3.01.75 */
/* at Wed Jul 29 16:25:34 1998
 */
/* Compiler settings for WOLAPI.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )

// This file is MIDL 3.01.75 output, edited only to remove what cannot build
// here. Gone are the C-language vtable structs, the COBJMACROS wrappers and
// the RPC proxy/stub declarations -- all three exist to marshal these
// interfaces across a process boundary, which nothing in this game does. What
// is left is the C++ half: ten pure-virtual interface declarations and the
// IIDs that name them.
//
// It leans on the Win32 spellings (HRESULT, IUnknown, interface, LPCSTR)
// rather than on windows.h. Off Windows those come from port/win32; on
// Windows that header defers to the real SDK.
//
// Six of the ten interfaces are used: IChat, IChatEvent, IDownload,
// IDownloadEvent, INetUtil and INetUtilEvent. IRTPatcher, IRTPatcherEvent,
// IChat2 and IChat2Event are named nowhere else in the tree -- the game never
// took up the patcher or the second chat revision. They cost nothing to
// declare, so they stay for fidelity to the IDL, but nothing needs porting on
// their account.
//
// Keep it looking generated. If wolapi.dll's IDL ever resurfaces, the diff
// against fresh MIDL output is what tells you whether this is still faithful.

#ifndef CNC_RED_ALERT_RA_WOLAPI_WOLAPI_H_
#define CNC_RED_ALERT_RA_WOLAPI_WOLAPI_H_

#include <ctime>  // for time_t, which IChatEvent::OnServerBannedYou uses.

#include "port/win32/win32_com.h"
#include "port/win32/win32_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward Declarations */

#ifndef __IRTPatcher_FWD_DEFINED__
#define __IRTPatcher_FWD_DEFINED__
typedef interface IRTPatcher IRTPatcher;
#endif /* __IRTPatcher_FWD_DEFINED__ */

#ifndef __IRTPatcherEvent_FWD_DEFINED__
#define __IRTPatcherEvent_FWD_DEFINED__
typedef interface IRTPatcherEvent IRTPatcherEvent;
#endif /* __IRTPatcherEvent_FWD_DEFINED__ */

#ifndef __IChat_FWD_DEFINED__
#define __IChat_FWD_DEFINED__
typedef interface IChat IChat;
#endif /* __IChat_FWD_DEFINED__ */

#ifndef __IChatEvent_FWD_DEFINED__
#define __IChatEvent_FWD_DEFINED__
typedef interface IChatEvent IChatEvent;
#endif /* __IChatEvent_FWD_DEFINED__ */

#ifndef __IDownload_FWD_DEFINED__
#define __IDownload_FWD_DEFINED__
typedef interface IDownload IDownload;
#endif /* __IDownload_FWD_DEFINED__ */

#ifndef __IDownloadEvent_FWD_DEFINED__
#define __IDownloadEvent_FWD_DEFINED__
typedef interface IDownloadEvent IDownloadEvent;
#endif /* __IDownloadEvent_FWD_DEFINED__ */

#ifndef __INetUtil_FWD_DEFINED__
#define __INetUtil_FWD_DEFINED__
typedef interface INetUtil INetUtil;
#endif /* __INetUtil_FWD_DEFINED__ */

#ifndef __INetUtilEvent_FWD_DEFINED__
#define __INetUtilEvent_FWD_DEFINED__
typedef interface INetUtilEvent INetUtilEvent;
#endif /* __INetUtilEvent_FWD_DEFINED__ */

#ifndef __IChat2_FWD_DEFINED__
#define __IChat2_FWD_DEFINED__
typedef interface IChat2 IChat2;
#endif /* __IChat2_FWD_DEFINED__ */

#ifndef __IChat2Event_FWD_DEFINED__
#define __IChat2Event_FWD_DEFINED__
typedef interface IChat2Event IChat2Event;
#endif /* __IChat2Event_FWD_DEFINED__ */

#ifndef __RTPatcher_FWD_DEFINED__
#define __RTPatcher_FWD_DEFINED__

#ifdef __cplusplus
typedef class RTPatcher RTPatcher;
#else
typedef struct RTPatcher RTPatcher;
#endif /* __cplusplus */

#endif /* __RTPatcher_FWD_DEFINED__ */

#ifndef __Chat_FWD_DEFINED__
#define __Chat_FWD_DEFINED__

#ifdef __cplusplus
typedef class Chat Chat;
#else
typedef struct Chat Chat;
#endif /* __cplusplus */

#endif /* __Chat_FWD_DEFINED__ */

#ifndef __Download_FWD_DEFINED__
#define __Download_FWD_DEFINED__

#ifdef __cplusplus
typedef class Download Download;
#else
typedef struct Download Download;
#endif /* __cplusplus */

#endif /* __Download_FWD_DEFINED__ */

#ifndef __NetUtil_FWD_DEFINED__
#define __NetUtil_FWD_DEFINED__

#ifdef __cplusplus
typedef class NetUtil NetUtil;
#else
typedef struct NetUtil NetUtil;
#endif /* __cplusplus */

#endif /* __NetUtil_FWD_DEFINED__ */

#ifndef __Chat2_FWD_DEFINED__
#define __Chat2_FWD_DEFINED__

#ifdef __cplusplus
typedef class Chat2 Chat2;
#else
typedef struct Chat2 Chat2;
#endif /* __cplusplus */

#endif /* __Chat2_FWD_DEFINED__ */

#ifndef __IRTPatcher_INTERFACE_DEFINED__
#define __IRTPatcher_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRTPatcher
 * at Wed Jul 29 16:25:34 1998
 * using MIDL 3.01.75
 ****************************************/
/* [object][unique][helpstring][uuid] */

EXTERN_C const IID IID_IRTPatcher;

interface DECLSPEC_UUID("925CDEDE-71B9-11D1-B1C5-006097176556") IRTPatcher
    : public IUnknown {
 public:
  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ApplyPatch(
      /* [string][in] */ LPCSTR destpath,
      /* [string][in] */ LPCSTR filename) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE PumpMessages() = 0;
};

#endif /* __IRTPatcher_INTERFACE_DEFINED__ */

#ifndef __IRTPatcherEvent_INTERFACE_DEFINED__
#define __IRTPatcherEvent_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRTPatcherEvent
 * at Wed Jul 29 16:25:34 1998
 * using MIDL 3.01.75
 ****************************************/
/* [object][unique][helpstring][uuid] */

EXTERN_C const IID IID_IRTPatcherEvent;

interface DECLSPEC_UUID("925CDEE3-71B9-11D1-B1C5-006097176556") IRTPatcherEvent
    : public IUnknown {
 public:
  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnProgress(
      /* [in] */ LPCSTR filename,
      /* [in] */ int progress) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnTermination(
      /* [in] */ BOOL success) = 0;
};

#endif /* __IRTPatcherEvent_INTERFACE_DEFINED__ */

#ifndef __IChat_INTERFACE_DEFINED__
#define __IChat_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IChat
 * at Wed Jul 29 16:25:34 1998
 * using MIDL 3.01.75
 ****************************************/
/* [object][unique][helpstring][uuid] */

struct Ladder {
  unsigned int sku;
  unsigned int team_no;
  unsigned int wins;
  unsigned int losses;
  unsigned int points;
  unsigned int kills;
  unsigned int rank;
  unsigned int rung;
  unsigned int disconnects;
  unsigned int team_rung;
  unsigned int provisional;
  unsigned int last_game_date;
  unsigned int win_streak;
  unsigned int reserved1;
  unsigned int reserved2;
  struct Ladder __RPC_FAR* next;
  unsigned char login_name[40];
};
typedef int GroupID;

struct Server {
  int gametype;
  int chattype;
  int timezone;
  float longitude;
  float lattitude;
  struct Server __RPC_FAR* next;
  unsigned char name[71];
  unsigned char connlabel[5];
  unsigned char conndata[128];
  unsigned char login[10];
  unsigned char password[10];
};
struct Channel {
  int type;
  unsigned int minUsers;
  unsigned int maxUsers;
  unsigned int currentUsers;
  unsigned int official;
  unsigned int tournament;
  unsigned int ingame;
  unsigned int flags;
  unsigned long reserved;
  unsigned long ipaddr;
  int latency;
  int hidden;
  struct Channel __RPC_FAR* next;
  unsigned char name[17];
  unsigned char topic[81];
  unsigned char location[65];
  unsigned char key[9];
  unsigned char exInfo[41];
};
struct User {
  unsigned int flags;
  GroupID group;
  unsigned long reserved;
  unsigned long reserved2;
  unsigned long reserved3;
  unsigned long reserved4;
  unsigned long ipaddr;
  unsigned long squad_icon;
  struct User __RPC_FAR* next;
  unsigned char name[10];
  unsigned char squadname[41];
};
struct Group {
  GroupID ident;
  int type;
  unsigned int members;
  struct Group __RPC_FAR* next;
  unsigned char name[65];
};
struct Update {
  unsigned long SKU;
  unsigned long version;
  int required;
  struct Update __RPC_FAR* next;
  unsigned char server[65];
  unsigned char patchpath[256];
  unsigned char patchfile[33];
  unsigned char login[33];
  unsigned char password[65];
  unsigned char localpath[256];
};
typedef struct Server Server;

typedef struct Channel Channel;

typedef struct User User;

typedef struct Group Group;

typedef struct Update Update;

typedef struct Ladder Ladder;

EXTERN_C const IID IID_IChat;

interface DECLSPEC_UUID("4DD3BAF4-7579-11D1-B1C6-006097176556") IChat
    : public IUnknown {
 public:
  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE PumpMessages() = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestServerList(
      /* [in] */ unsigned long SKU,
      /* [in] */ unsigned long current_version,
      /* [in] */ LPCSTR loginname,
      /* [in] */ LPCSTR password,
      /* [in] */ int timeout) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestConnection(
      /* [in] */ Server __RPC_FAR * server,
      /* [in] */ int timeout, int domangle) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestChannelList(
      /* [in] */ int channelType,
      /* [in] */ int autoping) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestChannelCreate(
      /* [in] */ Channel __RPC_FAR * channel) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestChannelJoin(
      /* [in] */ Channel __RPC_FAR * channel) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE
  RequestChannelLeave() = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestUserList() = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestPublicMessage(
      /* [in] */ LPCSTR message) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestPrivateMessage(
      /* [in] */ User __RPC_FAR * users,
      /* [in] */ LPCSTR message) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestLogout() = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE
  RequestPrivateGameOptions(
      /* [in] */ User __RPC_FAR * users,
      /* [in] */ LPCSTR options) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestPublicGameOptions(
      /* [in] */ LPCSTR options) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestPublicAction(
      /* [in] */ LPCSTR action) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestPrivateAction(
      /* [in] */ User __RPC_FAR * users,
      /* [in] */ LPCSTR action) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestGameStart(
      /* [in] */ User __RPC_FAR * users) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestChannelTopic(
      /* [in] */ LPCSTR topic) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetVersion(
      /* [in] */ unsigned long __RPC_FAR* version) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestUserKick(
      /* [in] */ User __RPC_FAR * user) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestUserIP(
      /* [in] */ User __RPC_FAR * user) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetGametypeInfo(
      unsigned int gtype, int icon_size,
      unsigned char __RPC_FAR * __RPC_FAR * bitmap, int __RPC_FAR* bmp_bytes,
      LPCSTR __RPC_FAR* name, LPCSTR __RPC_FAR* URL) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestFind(
      User __RPC_FAR * user) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestPage(
      User __RPC_FAR * user, LPCSTR message) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetFindPage(
      int findOn, int pageOn) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetSquelch(
      User __RPC_FAR * user, int squelch) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetSquelch(
      User __RPC_FAR * user) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetChannelFilter(
      int channelType) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestGameEnd() = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetLangFilter(
      int onoff) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestChannelBan(
      LPCSTR name, int ban) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetGametypeList(
      LPCSTR __RPC_FAR * list) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetHelpURL(
      LPCSTR __RPC_FAR * url) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetProductSKU(
      unsigned long SKU) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetNick(
      int num, LPCSTR __RPC_FAR* nick, LPCSTR __RPC_FAR* pass) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetNick(
      int num, LPCSTR nick, LPCSTR pass, int domangle) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetLobbyCount(
      int __RPC_FAR* count) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestRawMessage(
      LPCSTR ircmsg) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetAttributeValue(
      LPCSTR attrib, LPCSTR __RPC_FAR * value) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetAttributeValue(
      LPCSTR attrib, LPCSTR value) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetChannelExInfo(
      LPCSTR info) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE StopAutoping() = 0;
};

#endif /* __IChat_INTERFACE_DEFINED__ */

#ifndef __IChatEvent_INTERFACE_DEFINED__
#define __IChatEvent_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IChatEvent
 * at Wed Jul 29 16:25:34 1998
 * using MIDL 3.01.75
 ****************************************/
/* [object][unique][helpstring][uuid] */

EXTERN_C const IID IID_IChatEvent;

interface DECLSPEC_UUID("4DD3BAF6-7579-11D1-B1C6-006097176556") IChatEvent
    : public IUnknown {
 public:
  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnServerList(
      /* [in] */ HRESULT res,
      /* [in] */ Server __RPC_FAR * servers) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnUpdateList(
      /* [in] */ HRESULT res,
      /* [in] */ Update __RPC_FAR * updates) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnServerError(
      /* [in] */ HRESULT res,
      /* [in] */ LPCSTR ircmsg) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnConnection(
      /* [in] */ HRESULT res,
      /* [in] */ LPCSTR motd) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnMessageOfTheDay(
      /* [in] */ HRESULT res,
      /* [in] */ LPCSTR motd) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnChannelList(
      /* [in] */ HRESULT res,
      /* [in] */ Channel __RPC_FAR * channels) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnChannelCreate(
      /* [in] */ HRESULT res,
      /* [in] */ Channel __RPC_FAR * channel) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnChannelJoin(
      /* [in] */ HRESULT res,
      /* [in] */ Channel __RPC_FAR * channel,
      /* [in] */ User __RPC_FAR * user) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnChannelLeave(
      /* [in] */ HRESULT res,
      /* [in] */ Channel __RPC_FAR * channel,
      /* [in] */ User __RPC_FAR * user) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnChannelTopic(
      /* [in] */ HRESULT res,
      /* [in] */ Channel __RPC_FAR * channel,
      /* [in] */ LPCSTR topic) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnPrivateAction(
      /* [in] */ HRESULT res,
      /* [in] */ User __RPC_FAR * user,
      /* [in] */ LPCSTR action) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnPublicAction(
      /* [in] */ HRESULT res,
      /* [in] */ Channel __RPC_FAR * channel, User __RPC_FAR * user,
      /* [in] */ LPCSTR action) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnUserList(
      /* [in] */ HRESULT res,
      /* [in] */ Channel __RPC_FAR * channel,
      /* [in] */ User __RPC_FAR * users) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnPublicMessage(
      /* [in] */ HRESULT res,
      /* [in] */ Channel __RPC_FAR * channel,
      /* [in] */ User __RPC_FAR * user,
      /* [in] */ LPCSTR message) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnPrivateMessage(
      /* [in] */ HRESULT res,
      /* [in] */ User __RPC_FAR * user,
      /* [in] */ LPCSTR message) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnSystemMessage(
      /* [in] */ HRESULT res,
      /* [in] */ LPCSTR message) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnNetStatus(
      /* [in] */ HRESULT res) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnLogout(
      /* [in] */ HRESULT status,
      /* [in] */ User __RPC_FAR * user) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnPrivateGameOptions(
      /* [in] */ HRESULT res,
      /* [in] */ User __RPC_FAR * user,
      /* [in] */ LPCSTR options) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnPublicGameOptions(
      /* [in] */ HRESULT res,
      /* [in] */ Channel __RPC_FAR * channel,
      /* [in] */ User __RPC_FAR * user,
      /* [in] */ LPCSTR options) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnGameStart(
      /* [in] */ HRESULT res,
      /* [in] */ Channel __RPC_FAR * channel,
      /* [in] */ User __RPC_FAR * users,
      /* [in] */ int gameid) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnUserKick(
      /* [in] */ HRESULT res,
      /* [in] */ Channel __RPC_FAR * channel,
      /* [in] */ User __RPC_FAR * kicked,
      /* [in] */ User __RPC_FAR * kicker) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnUserIP(
      /* [in] */ HRESULT res,
      /* [in] */ User __RPC_FAR * user) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnFind(
      HRESULT res, Channel __RPC_FAR * chan) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnPageSend(
      HRESULT res) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnPaged(
      HRESULT res, User __RPC_FAR * user, LPCSTR message) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnServerBannedYou(
      HRESULT res, time_t bannedTill) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnUserFlags(
      HRESULT res, LPCSTR name, unsigned int flags, unsigned int mask) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnChannelBan(
      HRESULT res, LPCSTR name, int banned) = 0;
};

#endif /* __IChatEvent_INTERFACE_DEFINED__ */

#ifndef __IDownload_INTERFACE_DEFINED__
#define __IDownload_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IDownload
 * at Wed Jul 29 16:25:34 1998
 * using MIDL 3.01.75
 ****************************************/
/* [unique][helpstring][dual][uuid][object] */

EXTERN_C const IID IID_IDownload;

interface DECLSPEC_UUID("0BF5FCEB-9F03-11D1-9DC7-006097C54321") IDownload
    : public IUnknown {
 public:
  virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DownloadFile(
      LPCSTR server, LPCSTR login, LPCSTR password, LPCSTR file,
      LPCSTR localfile, LPCSTR regkey) = 0;

  virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Abort() = 0;

  virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE PumpMessages() = 0;
};

#endif /* __IDownload_INTERFACE_DEFINED__ */

#ifndef __IDownloadEvent_INTERFACE_DEFINED__
#define __IDownloadEvent_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IDownloadEvent
 * at Wed Jul 29 16:25:34 1998
 * using MIDL 3.01.75
 ****************************************/
/* [object][unique][helpstring][uuid] */

EXTERN_C const IID IID_IDownloadEvent;

interface DECLSPEC_UUID("6869E99D-9FB4-11D1-9DC8-006097C54321") IDownloadEvent
    : public IUnknown {
 public:
  virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE OnEnd() = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnError(int error) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnProgressUpdate(
      int bytesread, int totalsize, int timetaken, int timeleft) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnQueryResume() = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnStatusUpdate(
      int status) = 0;
};

#endif /* __IDownloadEvent_INTERFACE_DEFINED__ */

#ifndef __INetUtil_INTERFACE_DEFINED__
#define __INetUtil_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: INetUtil
 * at Wed Jul 29 16:25:34 1998
 * using MIDL 3.01.75
 ****************************************/
/* [object][unique][helpstring][uuid] */

EXTERN_C const IID IID_INetUtil;

interface DECLSPEC_UUID("B832B0AA-A7D3-11D1-97C3-00609706FA0C") INetUtil
    : public IUnknown {
 public:
  virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RequestGameresSend(
      LPCSTR host, int port, unsigned char __RPC_FAR* data, int length) = 0;

  virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RequestLadderSearch(
      LPCSTR host, int port, LPCSTR key, unsigned long SKU, int team, int cond,
      int sort, int number, int leading) = 0;

  virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RequestLadderList(
      LPCSTR host, int port, LPCSTR keys, unsigned long SKU, int team, int cond,
      int sort) = 0;

  virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RequestPing(
      LPCSTR host, int timeout, int __RPC_FAR* handle) = 0;

  virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE PumpMessages() = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetAvgPing(
      unsigned long ip, int __RPC_FAR* avg) = 0;
};

#endif /* __INetUtil_INTERFACE_DEFINED__ */

#ifndef __INetUtilEvent_INTERFACE_DEFINED__
#define __INetUtilEvent_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: INetUtilEvent
 * at Wed Jul 29 16:25:34 1998
 * using MIDL 3.01.75
 ****************************************/
/* [object][unique][helpstring][uuid] */

EXTERN_C const IID IID_INetUtilEvent;

interface DECLSPEC_UUID("B832B0AC-A7D3-11D1-97C3-00609706FA0C") INetUtilEvent
    : public IUnknown {
 public:
  virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE OnPing(
      HRESULT res, int time, unsigned long ip, int handle) = 0;

  virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE OnLadderList(
      HRESULT res,
      /* [in] */ Ladder __RPC_FAR * list, int totalCount, long timeStamp,
      int keyRung) = 0;

  virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE OnGameresSent(
      HRESULT res) = 0;
};

#endif /* __INetUtilEvent_INTERFACE_DEFINED__ */

#ifndef __IChat2_INTERFACE_DEFINED__
#define __IChat2_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IChat2
 * at Wed Jul 29 16:25:34 1998
 * using MIDL 3.01.75
 ****************************************/
/* [object][unique][helpstring][uuid] */

typedef unsigned long GID;

enum GTYPE_ { SERVER = 0, CHANNEL = 1, CLIENT = 2 };
typedef enum GTYPE_ GTYPE;

enum CHAN_CTYPE_ { ALLEXIT = 0, CREATOREXIT = 1, CLOSEC = 2 };
typedef enum CHAN_CTYPE_ CHAN_CTYPE;

EXTERN_C const IID IID_IChat2;

interface DECLSPEC_UUID("8B938190-EF3F-11D1-9808-00609706FA0C") IChat2
    : public IUnknown {
 public:
  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE PumpMessages() = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestConnection(
      Server __RPC_FAR * server, int timeout) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestMessage(
      GID who, LPCSTR message) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetTypeFromGID(
      GID id, GTYPE __RPC_FAR * type) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestChannelList() = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestChannelJoin(
      LPCSTR name) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestChannelLeave(
      Channel __RPC_FAR * chan) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestUserList(
      Channel __RPC_FAR * chan) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestLogout() = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestChannelCreate(
      Channel __RPC_FAR * chan) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestRawCmd(
      LPCSTR cmd) = 0;
};

#endif /* __IChat2_INTERFACE_DEFINED__ */

#ifndef __IChat2Event_INTERFACE_DEFINED__
#define __IChat2Event_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IChat2Event
 * at Wed Jul 29 16:25:34 1998
 * using MIDL 3.01.75
 ****************************************/
/* [object][unique][helpstring][uuid] */

EXTERN_C const IID IID_IChat2Event;

interface DECLSPEC_UUID("8B938192-EF3F-11D1-9808-00609706FA0C") IChat2Event
    : public IUnknown {
 public:
  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnNetStatus(
      HRESULT res) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnMessage(
      HRESULT res, User __RPC_FAR * user, LPCSTR message) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnChannelList(
      HRESULT res, Channel __RPC_FAR * list) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnChannelJoin(
      HRESULT res, Channel __RPC_FAR * chan, User __RPC_FAR * user) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnLogin(HRESULT res) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnUserList(
      HRESULT res, Channel __RPC_FAR * chan, User __RPC_FAR * users) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnChannelLeave(
      HRESULT res, Channel __RPC_FAR * chan, User __RPC_FAR * user) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnChannelCreate(
      HRESULT res, Channel __RPC_FAR * chan) = 0;

  virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnUnknownLine(
      HRESULT res, LPCSTR line) = 0;
};

#endif /* __IChat2Event_INTERFACE_DEFINED__ */

#ifndef __WOLAPILib_LIBRARY_DEFINED__
#define __WOLAPILib_LIBRARY_DEFINED__

/****************************************
 * Generated header for library: WOLAPILib
 * at Wed Jul 29 16:25:34 1998
 * using MIDL 3.01.75
 ****************************************/
/* [helpstring][version][uuid] */

EXTERN_C const IID LIBID_WOLAPILib;

#ifdef __cplusplus
EXTERN_C const CLSID CLSID_RTPatcher;

class DECLSPEC_UUID("925CDEDF-71B9-11D1-B1C5-006097176556") RTPatcher;
#endif

#ifdef __cplusplus
EXTERN_C const CLSID CLSID_Chat;

class DECLSPEC_UUID("4DD3BAF5-7579-11D1-B1C6-006097176556") Chat;
#endif

#ifdef __cplusplus
EXTERN_C const CLSID CLSID_Download;

class DECLSPEC_UUID("BF6EA206-9E55-11D1-9DC6-006097C54321") Download;
#endif

#ifdef __cplusplus
EXTERN_C const CLSID CLSID_NetUtil;

class DECLSPEC_UUID("B832B0AB-A7D3-11D1-97C3-00609706FA0C") NetUtil;
#endif

#ifdef __cplusplus
EXTERN_C const CLSID CLSID_Chat2;

class DECLSPEC_UUID("8B938191-EF3F-11D1-9808-00609706FA0C") Chat2;
#endif
#endif /* __WOLAPILib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif  // CNC_RED_ALERT_RA_WOLAPI_WOLAPI_H_
