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

//	rawolapi.h - WOLAPI sinks declarations.
//	ajw 07/10/98

//	Based somewhat on Neal's Borlandized version, "chatapi.h".

#ifndef CNC_RED_ALERT_RA_RAWOLAPI_H_
#define CNC_RED_ALERT_RA_RAWOLAPI_H_

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <ctime>

// ajw wrapped these in a WOL namespace to keep wolapi's `struct Server` away
// from Red Alert's global of that name, then had to take it out again --
// "Can't use namespaces in Watcom 10.5 it seems". The global has since been
// renamed IsNetworkHost, so there is nothing left to collide with.
#include "port/win32/win32_com.h"
#include "port/win32/win32_types.h"
#include "ra/config.h"
#include "ra/defines.h"
#include "ra/wolapi/chatdefs.h"
#include "ra/wolapi/downloaddefs.h"
#include "ra/wolapi/ftpdefs.h"
#include "ra/wolapi/wolapi.h"

//***********************************************************************************************
//	For debugging chat defined hresults...
//	Writes the name of `hRes` into `szDesc`, truncating rather than
//	overflowing. About 150 characters is enough for any of them.
void ChatDefAsText(char* szDesc, std::size_t iSize, HRESULT hRes);
void DebugChatDef(HRESULT hRes);

int iChannelLobbyNumber(const unsigned char* szChannelName);
#define REASONABLELOBBYINTERPRETEDNAMELEN 50
void InterpretLobbyNumber(char* szLobbyNameToSet, int iLobby);

class WolapiObject;

#define MAXCHATSENDLENGTH \
  71  //	Mainly aesthetic, and because of the length of edit line.

enum CHANNELFILTER {
  CHANNELFILTER_NO,
  CHANNELFILTER_OFFICIAL,
  CHANNELFILTER_UNOFFICIAL,
  CHANNELFILTER_LOBBIES,
  CHANNELFILTER_LOCALLOBBYGAMES,
};

#define WOLCOLORREMAP_ACTION PCOLOR_GREY
#define WOLCOLORREMAP_SELFSPEAKING PCOLOR_RED
#define WOLCOLORREMAP_LOCALMACHINEMESS \
  PCOLOR_REALLY_BLUE  //	Color of system messages that originate locally.
#define WOLCOLORREMAP_PAGE PCOLOR_GOLD
#define WOLCOLORREMAP_KICKORBAN PCOLOR_GREEN  // LTBLUE
#define WOLCOLORREMAP_PUBLICMESSAGE PCOLOR_NONE
#define WOLCOLORREMAP_PRIVATEMESSAGE PCOLOR_ORANGE

#define WOLSOUND_ERROR VOC_SYS_ERROR
#define WOLSOUND_LOGIN VOC_RADAR_ON
#define WOLSOUND_LOGOUT VOC_RADAR_OFF
#define WOLSOUND_ENTERCHAN VOC_PLAYER_JOINED
#define WOLSOUND_EXITCHAN VOC_PLAYER_LEFT
#define WOLSOUND_ONPAGE VOC_INCOMING_MESSAGE
#define WOLSOUND_KICKORBAN VOC_TANYA_KISS
#define WOLSOUND_ENTERGAME VOC_INVULNERABLE
#define WOLSOUND_EXITGAME VOC_DOOR

enum DISCONNECT_PING_STATUS {
  PING_UNSTARTED,
  PING_WAITING,
  PING_GOOD,
  PING_BAD,
};
#define DISCONNECT_PING_COUNT 5

//***********************************************************************************************
class RAChatEventSink
    :  /////public CComObjectRoot, /////public IConnectionPoint,
       public IChatEvent {
 public:
  RAChatEventSink(WolapiObject* pOwner);
  ~RAChatEventSink() override;

  //    BEGIN_COM_MAP(RAChatEventSink)
  //	  COM_INTERFACE_ENTRY(IChatEvent)
  //    END_COM_MAP()

  // IUnknown
  STDMETHOD(QueryInterface)(const IID& iid, void** ppv) override;
  STDMETHOD_(ULONG, AddRef)() override;
  STDMETHOD_(ULONG, Release)() override;

  // IChatEvent. The tree's wolapi.h is a later revision of the IDL than this
  // sink was written against: OnBusy, OnIdle, OnChannelModify, OnGroupList and
  // a one-argument OnServerError were declared here and overrode nothing.
  STDMETHOD(OnServerList)(HRESULT res, Server* servers) override;
  STDMETHOD(OnLogout)(HRESULT r, User* user) override;
  STDMETHOD(OnPageSend)(HRESULT r) override;
  STDMETHOD(OnPaged)(HRESULT r, User*, LPCSTR) override;
  STDMETHOD(OnFind)(HRESULT r, Channel*) override;
  STDMETHOD(OnConnection)(HRESULT r, LPCSTR motd) override;
  STDMETHOD(OnChannelCreate)(HRESULT r, Channel* channel) override;
  STDMETHOD(OnChannelJoin)(HRESULT r, Channel* channel, User* user) override;
  STDMETHOD(OnChannelLeave)(HRESULT r, Channel* channel, User* user) override;
  STDMETHOD(OnChannelTopic)(HRESULT r, Channel* channel, LPCSTR topic) override;
  STDMETHOD(OnPublicMessage)
  (HRESULT r, Channel* channel, User* user, LPCSTR text) override;
  STDMETHOD(OnPrivateMessage)(HRESULT r, User* user, LPCSTR text) override;
  STDMETHOD(OnSystemMessage)(HRESULT r, LPCSTR) override;
  STDMETHOD(OnNetStatus)(HRESULT r) override;
  STDMETHOD(OnChannelList)(HRESULT r, Channel* channels) override;
  STDMETHOD(OnUserList)(HRESULT r, Channel* channel, User* users) override;
  STDMETHOD(OnUpdateList)(HRESULT res, Update*) override;
  STDMETHOD(OnMessageOfTheDay)(HRESULT res, LPCSTR) override;
  STDMETHOD(OnPrivateAction)(HRESULT r, User*, LPCSTR) override;
  STDMETHOD(OnPublicAction)(HRESULT r, Channel*, User*, LPCSTR) override;
  STDMETHOD(OnPrivateGameOptions)(HRESULT r, User*, LPCSTR) override;
  STDMETHOD(OnPublicGameOptions)(HRESULT r, Channel*, User*, LPCSTR) override;
  STDMETHOD(OnGameStart)(HRESULT r, Channel*, User*, int) override;
  STDMETHOD(OnUserKick)(HRESULT r, Channel*, User*, User*) override;
  STDMETHOD(OnUserIP)(HRESULT r, User*) override;
  STDMETHOD(OnServerError)(HRESULT res, LPCSTR ircmsg) override;
  STDMETHOD(OnServerBannedYou)(HRESULT r, time_t bannedTill) override;
  STDMETHOD(OnUserFlags)
  (HRESULT r, LPCSTR name, unsigned int flags, unsigned int mask) override;
  STDMETHOD(OnChannelBan)(HRESULT r, LPCSTR name, int banned) override;

  unsigned long GetPlayerGameIP(const char* szPlayerName) const;
  void DeleteUserList();  //	Deletes from heap all users pointed to through
                          // pUserList.
  void DeleteUserIPList();
  unsigned long GetUserIP(const char* szName) const;

  void ActionEggSound(const char* szMessage);

  //	These vars are rather hackish. Basically, they are set before a callback
  // is expected to be fired, and 	then checked immediately afterwards. The
  // rest of the time, their values are meaningless. 	The idea is to force
  // wolapi act in a modal way. In many places I "block" until a callback
  // response to a 	wolapi request has been received.
  bool bRequestServerListWait;
  bool bRequestConnectionWait;
  bool bRequestLogoutWait;
  //	bool	bRequestChannelListWait;
  bool bRequestChannelJoinWait;
  bool bRequestChannelLeaveWait;
  bool bRequestUserListWait;
  bool bRequestChannelCreateWait;
  bool bRequestFindWait;
  bool bRequestPageWait;

  bool bRequestChannelListForLobbiesWait;

  bool bIgnoreChannelLists;  //	Used to temporarily turn off response to channel
                             // lists, when we are in the midst 	of some
                             // processing that depends on pChannelList
                             // remaining constant.

  bool bRequestGameStartWait;

  Server* pServer;  //	Server to connect to, acquired from OnServerList.
  bool bConnected;  //	True when user is logged in to chat server.
  bool bJoined;     //	True when user has joined a channel.

  Channel* pChannelList;        //	First element of channel list, or null.
  CHANNELFILTER ChannelFilter;  //	Affects what channels are included in
                                // channel list when built.

  User* pUserList;  //	First element of user list, or null.
  User* pUserTail;  //	Last element of user list, or null.

  char* szMotd;                        //	Message of the day.
  HRESULT hresRequestConnectionError;  //	Used to pass error hresult.

  HRESULT hresRequestFindResult;  //	Used to pass hresult.
  Channel OnFindChannel;

  HRESULT hresRequestPageResult;  //	Used to pass hresult.

  HRESULT hresRequestJoinResult;  //	Used to pass hresult.

  bool bGotKickedTrigger;  //	Special flag meaning do some more processing
                           // after callback has exited.

  User* pGameUserList;  //	First element of start game user list, or null.
  int iGameID;          //	WW Online game id received from OnGameStart.
                        //	Is also a flag indicating "OnGameStart() called,
                        // TriggerGameStart() not yet called".

  User* pUserIPList;  //	List that holds user IP's, used for pinging in
                      // game channel.
  User* pUserIPListTail;

 protected:
  WolapiObject* pOwner;  //	Link back to the object that contains me.

  void DeleteChannelList();  //	Deletes from heap all channels pointed to
                             // through pChannelList.
  bool DownloadUpdates(Update* pUpdateList, int iUpdates);
  bool bSpecialMessage(const char* szMessage);
  void InsertUserSorted(User* pUserNew);

 private:
  std::atomic<int> m_cRef;  // Reference count.
};

//***********************************************************************************************
class RADownloadEventSink :
    ///////////	public CComObjectRoot,
    public IDownloadEvent {
 public:
  RADownloadEventSink();
  ~RADownloadEventSink() override = default;

  //  BEGIN_COM_MAP(RADownloadEventSink)
  //    COM_INTERFACE_ENTRY(IDownloadEvent)
  //  END_COM_MAP()

  // IUnknown
  STDMETHOD(QueryInterface)(const IID& iid, void** ppv) override;
  STDMETHOD_(ULONG, AddRef)() override;
  STDMETHOD_(ULONG, Release)() override;

  // IDownloadEvent
  STDMETHOD(OnEnd)() override;
  STDMETHOD(OnError)(int error) override;
  STDMETHOD(OnProgressUpdate)
  (int bytesread, int totalsize, int timetaken, int timeleft) override;
  STDMETHOD(OnStatusUpdate)(int status) override;
  STDMETHOD(OnQueryResume)() override;

  bool bFlagEnd;
  bool bFlagError;
  bool bFlagProgressUpdate;
  bool bFlagStatusUpdate;
  bool bFlagQueryResume;
  int iBytesRead;
  int iTotalSize;
  int iTimeTaken;
  int iTimeLeft;
  int iStatus;
  bool bResumed;

 private:
  std::atomic<int> m_cRef;  // Reference count.
};

//***********************************************************************************************
class RANetUtilEventSink :
    //    public CComObjectRoot,
    public INetUtilEvent {
 public:
  RANetUtilEventSink(WolapiObject* pOwner);
  ~RANetUtilEventSink() override;

  // BEGIN_COM_MAP(CNetUtilEventSink)
  //	COM_INTERFACE_ENTRY(INetUtilEvent)
  // END_COM_MAP()

  // IUnknown
  STDMETHOD(QueryInterface)(const IID& iid, void** ppv) override;
  STDMETHOD_(ULONG, AddRef)() override;
  STDMETHOD_(ULONG, Release)() override;

  // INetUtilEvent

  STDMETHOD(OnGameresSent)(HRESULT res) override;
  STDMETHOD(OnLadderList)
  (HRESULT res, Ladder* list, int totalCount, long timeStamp, int keyRung)
      override;
  STDMETHOD(OnPing)(HRESULT res, int time, unsigned long ip,
                    int handle) override;

  void DeleteLadderList();  //	Deletes from heap all users pointed to through
                            // pUserList.
  unsigned int GetUserRank(const char* szName, bool bRankRA);

  Ladder* pLadderList;    //	First element of Ladder list, or null.
  Ladder* pLadderTail;    //	Last element of Ladder list, or null.
  Ladder* pLadderListAM;  //	First element of Aftermath Ladder list, or null.
  Ladder* pLadderTailAM;  //	Last element of Aftermath Ladder list, or null.

 protected:
  WolapiObject* pOwner;  //	Link back to the object that contains me.

 private:
  std::atomic<int> m_cRef;  // Reference count.
};

//***********************************************************************************************

// SKU reported to WOLAPI for the purpose of finding patches: one per
// language release.
inline constexpr int kGameSku =
    config::kIsEnglish ? 0x1500 : config::kIsGerman ? 0x1502 : 0x1503;

inline constexpr int kGameVersion = 0x00030003;
#define GAME_TYPE 21
#define LOB_PREFIX "Lob_21_"

//	Sent to gameres server in order to receive Red Alert or Aftermath ladder
// rankings. (Sent in RequestLadderList.)
#define LADDER_CODE_RA 1005
#define LADDER_CODE_AM 500

#endif  // CNC_RED_ALERT_RA_RAWOLAPI_H_
