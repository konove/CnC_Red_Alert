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

/* $Header: /counterstrike/SESSION.H 4     3/10/97 6:23p Steve_tall $ */
/***************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 *                                                                         *
 *                 Project Name : Command & Conquer                        *
 *                                                                         *
 *                    File Name : SESSION.H                                *
 *                                                                         *
 *                   Programmer : Bill R. Randolph                         *
 *                                                                         *
 *                   Start Date : 11/30/95                                 *
 *                                                                         *
 *                  Last Update : November 30, 1995 [BRR]                  *
 *                                                                         *
 * The purpose of this class is to contain those variables & routines *
 * specifically related to a multiplayer game.
 **
 *                                                                         *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
#ifndef CNC_RED_ALERT_RA_NET_SESSION_H_
#define CNC_RED_ALERT_RA_NET_SESSION_H_

#include <array>
#include <concepts>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <utility>

#include "absl/base/attributes.h"
#include "absl/log/check.h"
#include "base/enum_array.h"
#include "base/installed.h"
#include "base/payload.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/net/connect.h"
#include "engine/net/ipxaddr.h"
#include "engine/platform/platform.h"
#include "engine/stream/byte_sink.h"
#include "engine/stream/byte_source.h"
#include "engine/stream/byte_stream.h"
#include "game/core/owned_vector.h"
#include "ra/core/defines.h"
#include "ra/core/special_flags.h"
#include "ra/core/target.h"
#include "ra/net/event.h"
#include "ra/objects/object.h"
#include "ra/startup/version.h"
#include "ra/ui/msglist.h"

class ArchiveReader;
class ArchiveWriter;

namespace ra {

class CellClass;

//---------------------------------------------------------------------------
// Defines
//---------------------------------------------------------------------------
//...........................................................................
// Various limiting values
//...........................................................................
#define MPLAYER_BUILD_LEVEL_MAX 10  // max build level in multiplay
#define MAX_MPLAYER_COLORS 8        // max # of colors

//...........................................................................
// Max sizes of packets we want to send
// The IPX packet's size is IPX's max size (546), rounded down to accommodate
// the max number of events possible.
//...........................................................................
#define MAX_IPX_PACKET_SIZE                                             \
  (((546 - sizeof(engine::net::CommHeaderType)) / sizeof(EventClass)) * \
   sizeof(EventClass))
#define MAX_SERIAL_PACKET_SIZE 256

//...........................................................................
// Max length of player names fields; attempt to use the constant for the
// HouseClass, if it's been defined; otherwise, define it myself.
//...........................................................................
#ifdef HOUSE_NAME_MAX
#define MPLAYER_NAME_MAX HOUSE_NAME_MAX
#else
#define MPLAYER_NAME_MAX 12  // max length of a player's name
#endif

//...........................................................................
// Values to control the multiplayer score screen
//...........................................................................
#define MAX_MULTI_NAMES 8  // max # names (rows) on the score screen
#define MAX_MULTI_GAMES 4  // max # games (columns) on the score screen

//...........................................................................
// Min value for MaxAhead, for both net & modem; only applies for
// COMM_PROTOCOL_MULTI_E_COMP.
//...........................................................................
#define MODEM_MIN_MAX_AHEAD 5
#define NETWORK_MIN_MAX_AHEAD 2

//...........................................................................
// Send period (in frames) for COMM_PROTOCOL_MULTI_E_COMP and above
//...........................................................................
#define DEFAULT_FRAME_SEND_RATE 3

//...........................................................................
// Modem-specific constants
//...........................................................................
#define PORTBUF_MAX 64  // dialog field sizes
#define IRQBUF_MAX 3
#define BAUDBUF_MAX 7
#define INITSTRBUF_MAX 41
#define CWAITSTRBUF_MAX 16
#define CREDITSBUF_MAX 5
#define PACKET_TIMING_TIMEOUT 40  // ticks b/w sending a timing packet
// Max length of modem name in list box.
#define MODEM_NAME_MAX (PORTBUF_MAX - 1)

// One modem initialization string, nul-terminated.
using InitString = std::array<char, INITSTRBUF_MAX>;

//---------------------------------------------------------------------------
// Enums
//---------------------------------------------------------------------------
//...........................................................................
// Types of games; used to tell which protocol we're using
//...........................................................................
enum class GameType {
  GAME_NORMAL,      // not multiplayer
  GAME_MODEM,       // modem game
  GAME_NULL_MODEM,  // NULL-modem
  GAME_IPX,         // IPX Network game
  GAME_INTERNET,    // Internet H2H
  GAME_SKIRMISH     // 1 plr vs. AI's
};
using enum GameType;

//...........................................................................
// Various Modem-specific enums
//...........................................................................
enum class DetectPortType { PORT_VALID = 0, PORT_INVALID, PORT_IRQ_INUSE };
using enum DetectPortType;

enum class DialStatusType {
  DIAL_CONNECTED = 0,
  DIAL_NO_CARRIER,
  DIAL_BUSY,
  DIAL_ERROR,
  DIAL_NO_DIAL_TONE,
  DIAL_CANCELED
};
using enum DialStatusType;

enum class DialMethodType { DIAL_TOUCH_TONE = 0, DIAL_PULSE, DIAL_METHODS };
using enum DialMethodType;

inline constexpr int kCallWaitTone1 = 0;
inline constexpr int kCallWaitTone2 = kCallWaitTone1 + 1;
inline constexpr int kCallWaitPulse = kCallWaitTone2 + 1;
inline constexpr int kCallWaitCustom = kCallWaitPulse + 1;
inline constexpr int kCallWaitStringsNum = kCallWaitCustom + 1;

enum class ModemGameType {
  MODEM_NULL_HOST = 0,
  MODEM_NULL_JOIN,
  MODEM_DIALER,
  MODEM_ANSWERER
};
using enum ModemGameType;

//...........................................................................
// Commands sent over the serial Global Channel
//...........................................................................
enum class SerialCommandType : uint16_t {
  SERIAL_CONNECT = 100,       // Are you there?  Hello?  McFly?
  SERIAL_GAME_OPTIONS = 101,  // Hey, dudes, here's some new game options
  SERIAL_SIGN_OFF = 102,  // Bogus, dudes, my boss is coming; I'm outta here!
  SERIAL_GO = 103,        // OK, dudes, jump into the game loop!
  SERIAL_MESSAGE = 104,   // Here's a message
  SERIAL_TIMING = 105,    // timimg packet
  SERIAL_SCORE_SCREEN = 106,  // player at score screen
  SERIAL_LOADGAME = 107,      // Start the game, loading a saved game first
  SERIAL_LAST_COMMAND = 108,  // last command
  SERIAL_REQ_SCENARIO =
      1000,  // Reqest that host sends the scenario file to the other players.
  SERIAL_FILE_INFO =
      1001,  // Info about the file that is going to be transferred
  SERIAL_FILE_CHUNK = 1002,   // A chunk of scenario
  SERIAL_READY_TO_GO = 1003,  // Sent in response to a 'GO' command
  SERIAL_NO_SCENARIO =
      1004  // Scenario isnt available on remote machine so we cant play
};
using enum SerialCommandType;

//...........................................................................
// Commands sent over the network Global Channel
//...........................................................................
enum class NetCommandType : uint16_t {
  NET_QUERY_GAME = 0,     // Hey, what games are out there?
  NET_ANSWER_GAME = 1,    // Yo, Here's my game's name!
  NET_QUERY_PLAYER = 2,   // Hey, what players are in this game?
  NET_ANSWER_PLAYER = 3,  // Yo, I'm in that game!
  NET_CHAT_ANNOUNCE = 4,  // I'm at the chat screen
  NET_CHAT_REQUEST = 5,   // Respond with a CHAT_ANNOUNCE, please.
  NET_QUERY_JOIN = 6,     // Hey guys, can I play too?
  NET_CONFIRM_JOIN = 7,   // Well, OK, if you really want to.
  NET_REJECT_JOIN = 8,    // No, you can't join; sorry, dude.
  NET_GAME_OPTIONS = 9,   // Hey, dudes, here's some new game options
  NET_SIGN_OFF = 10,      // Bogus, dudes, my boss is coming; I'm outta here!
  NET_GO = 11,            // OK, jump into the game loop!
  NET_MESSAGE = 12,       // Here's a message
  NET_PING = 13,          // I'm pinging you to take a time measurement
  NET_LOADGAME = 14,      // start a game by loading a saved game
  NET_REQ_SCENARIO =
      1000,  // Reqest that host sends the scenario file to the other players.
  NET_FILE_INFO = 1001,   // Info about the file that is going to be transferred
  NET_FILE_CHUNK = 1002,  // A chunk of scenario
  NET_READY_TO_GO = 1003,  // Sent in response to a 'GO' command
  NET_NO_SCENARIO =
      1004  // Scenario isnt available on remote machine so we cant play
};
using enum NetCommandType;

//---------------------------------------------------------------------------
// Structures
//---------------------------------------------------------------------------
//...........................................................................
// An entry on the score screen is defined by this structure
//...........................................................................
struct MPlayerScoreType {
  char Name[MPLAYER_NAME_MAX];
  int Wins;
  int Kills[MAX_MULTI_GAMES];
  PlayerColorType Color;
};

//...........................................................................
// Settings for the serial port
//...........................................................................
struct SerialSettingsType {
  int Port;
  int IRQ;
  int Baud;
  DialMethodType DialMethod;
  int InitStringIndex;
  int CallWaitStringIndex;
  char CallWaitString[CWAITSTRBUF_MAX];
  bool Compression;
  bool ErrorCorrection;
  bool HardwareFlowControl;
  char ModemName[MODEM_NAME_MAX];
};

// ??
class PhoneEntryClass {
 public:
  SerialSettingsType Settings;

  static const int PHONE_MAX_NUM = 20;
  char Number[PHONE_MAX_NUM + 1];

  static const int PHONE_MAX_NAME = 30;
  char Name[PHONE_MAX_NAME];
};
//

// A name and an address on the network: what every list node has.
struct NodeName {
  char Name[MPLAYER_NAME_MAX]{};  // player or game name
  engine::net::IPXAddressClass Address;
};

// A game announced on the network, in Session::Games.
struct GameNode : NodeName {
  unsigned char IsOpen = 0;  // is the game open?
  int64_t LastTime = 0;      // last time we heard from it
};

// A player in this game, in Session::Players. The only node a save holds.
struct PlayerNode : NodeName {
  // Saved-game support; defined in saveglobals.cc.
  template <class Archive>
  void Serialize(Archive& ar);

  HousesType House{};       // "ActLike" House of this player
  PlayerColorType Color{};  // Color of this player
  HousesType ID{};          // Actual House of this player
  int ProcessTime = 0;      // Length of time to process players main loop
};

// A player in the chat lobby, in Session::Chat.
struct ChatNode : NodeName {
  int64_t LastTime = 0;          // last time we heard from this guy
  unsigned char LastChance = 0;  // we're about to remove him from the list
  PlayerColorType Color{};       // chat player's color
};

//...........................................................................
// Packet sent over the serial Global Channel
//...........................................................................
class SerialPacketType {
 public:
  // The payload arms, by what Command says the packet is. Each names its
  // commands in Carries(); data<T>() and UpdateData<T>() check that Command
  // is one of them. The arms share the payload's bytes, as the union they
  // replace did.
  struct ScenarioInfoData {
    static constexpr bool Carries(SerialCommandType command) {
      // Every serial command but MESSAGE; SIGN_OFF, TIMING and
      // SCORE_SCREEN use only Color, the sender's ID.
      return command != SERIAL_MESSAGE;
    }
    HousesType House;                   // player's House
    PlayerColorType Color;              // player's color or SIGNOFF ID
    uint32_t MinVersion;                // min version this game supports
    uint32_t MaxVersion;                // max version this game supports
    char Scenario[kDescripMax];         // Scenario name
    int Credits;                        // player's credits
    unsigned int IsBases : 1;           // 1 = bases are allowed
    unsigned int IsTiberium : 1;        // 1 = tiberium is allowed
    unsigned int IsGoodies : 1;         // 1 = goodies are allowed
    unsigned int IsGhosties : 1;        // 1 = ghosts are allowed
    unsigned int OfficialScenario : 1;  //	Is this scenario an official
                                        // Westwood one?
    int CheatCheck;                     // Unique ID of "rules.ini" file.
    unsigned char BuildLevel;           // buildable level
    unsigned char UnitCount;            // max # units
    unsigned char AIPlayers;            // # of AI players allowed
    int Seed;                           // random number seed
    SpecialFlags Special;               // command-line options
    unsigned int GameSpeed;             // Game Speed
    int32_t ResponseTime;               // packet response time
    unsigned int FileLength;  // Length of scenario file to expect from host.
    // Name of the scenario file to expect from the host: 12 characters of
    // 8.3 name plus the terminator.
    char ShortFileName[13];
    char FileDigest[32];  // Digest of scenario file to expect from host
                          //	ajw - This is not necessarily null-terminated.
  };
  struct MessageData {
    static constexpr bool Carries(SerialCommandType command) {
      return command == SERIAL_MESSAGE;
    }
    char Message[MAX_MESSAGE_LENGTH];  // inter-player message
    PlayerColorType Color;             // player's color or SIGNOFF ID
  };

  // Returns a copy of payload arm T. Command must be one T carries.
  template <class T>
  [[nodiscard]] T data() const {
    DCHECK(T::Carries(Command));
    return data_.Get<T>();
  }
  // Copies arm T out, lets `edit` change it and copies it back; the payload
  // bytes past T keep what they held. Set Command first: it must be one T
  // carries.
  template <class T, std::invocable<T&> Edit>
  void UpdateData(Edit&& edit) {
    DCHECK(T::Carries(Command));
    data_.Update<T>(std::forward<Edit>(edit));
  }

  SerialCommandType Command = SERIAL_LAST_COMMAND;  // none until set
  char Name[MPLAYER_NAME_MAX]{};                    // Player or Game Name
  unsigned char ID = 0;  // unique ID of sender of message

 private:
  base::Payload<ScenarioInfoData, MessageData> data_;
};

// Both packets go on the wire with sizeof(), so every peer must agree on these
// numbers. Change one only when you mean to change the protocol.
//
// ShortFileName above used to be [12] in one build and [13] in the other,
// which made these two sizes depend on WOLAPI_INTEGRATION. It is [13]
// everywhere now -- 12 characters of 8.3 name plus the terminator, which is
// what the writers were always assuming -- so there is one protocol rather
// than two, and these assertions are what will notice if that changes again.
static_assert(sizeof(SerialPacketType) == 160);

//...........................................................................
// Other packet sent over the serial global channel (for file transfers)
//...........................................................................
#define MAX_SEND_FILE_PACKET_SIZE (MAX_SERIAL_PACKET_SIZE - 64)
struct RemoteFileTransferType {
  SerialCommandType
      Command;  // Enum defined above. Should be a file transfer enum.
  uint16_t BlockNumber;  // Index position of this file chunk in the file
  uint16_t BlockLength;  // Length of data in the RawData buffer
  unsigned char RawData[MAX_SEND_FILE_PACKET_SIZE];
};

//...........................................................................
// Packet sent over the network Global Channel
//...........................................................................
class GlobalPacketType {
 public:
  // The payload arms, by what Command says the packet is; see
  // SerialPacketType.
  struct GameInfoData {
    static constexpr bool Carries(NetCommandType command) {
      return command == NET_ANSWER_GAME;
    }
    unsigned int IsOpen : 1;  // 1 = game is open for joining
  };
  struct PlayerInfoData {
    static constexpr bool Carries(NetCommandType command) {
      return command == NET_QUERY_JOIN || command == NET_ANSWER_PLAYER ||
             command == NET_CONFIRM_JOIN;
    }
    HousesType House;       // player's House
    PlayerColorType Color;  // player's color
    uint32_t NameCRC;       // CRC of player's game's name
    uint32_t MinVersion;    // game's min supported version
    uint32_t MaxVersion;    // game's max supported version
    int CheatCheck;         // Unique ID of "rules.ini" file.
  };
  struct ScenarioInfoData {
    static constexpr bool Carries(NetCommandType command) {
      return command == NET_GAME_OPTIONS || command == NET_FILE_INFO;
    }
    char Scenario[kDescripMax];    // Scenario Name
    int Credits;                   // player's credits
    uint8_t IsBases : 1;           // 1 = bases are allowed
    uint8_t IsTiberium : 1;        // 1 = tiberium is allowed
    uint8_t IsGoodies : 1;         // 1 = goodies are allowed
    uint8_t IsGhosties : 1;        // 1 = ghosts are allowed
    uint8_t OfficialScenario : 1;  // Is this scenario an official Westwood one?
    unsigned char BuildLevel;      // buildable level
    unsigned char UnitCount;       // max # units
    unsigned char AIPlayers;       // # of AI players allowed
    int Seed;                      // random number seed
    SpecialFlags Special;          // command-line options
    unsigned int GameSpeed;        // Game Speed
    uint32_t Version;              // version # common to all players
    unsigned int FileLength;  // Length of scenario file to expect from host.
    // Name of the scenario file to expect from the host: 12 characters of
    // 8.3 name plus the terminator.
    char ShortFileName[13];
    char FileDigest[32];  // Digest of scenario file to expect from host
                          //	ajw - This is not necessarily null-terminated.
  };
  struct MessageData {
    static constexpr bool Carries(NetCommandType command) {
      return command == NET_MESSAGE;
    }
    char Buf[MAX_MESSAGE_LENGTH];  // inter-user message
    PlayerColorType Color;         // color of sender of message
    uint32_t NameCRC;              // CRC of sender's Game Name
  };
  struct ResponseTimeData {
    static constexpr bool Carries(NetCommandType command) {
      return command == NET_GO || command == NET_LOADGAME;
    }
    int OneWay;  // one-way response time
  };
  struct RejectData {
    static constexpr bool Carries(NetCommandType command) {
      return command == NET_REJECT_JOIN;
    }
    int Why;  // why were we rejected from the game?
  };
  struct ChatData {
    static constexpr bool Carries(NetCommandType command) {
      return command == NET_CHAT_ANNOUNCE;
    }
    uint32_t ID;            // unique ID for this chat node
    PlayerColorType Color;  // my color
  };

  // Returns a copy of payload arm T. Command must be one T carries.
  template <class T>
  [[nodiscard]] T data() const {
    DCHECK(T::Carries(Command));
    return data_.Get<T>();
  }
  // Copies arm T out, lets `edit` change it and copies it back; the payload
  // bytes past T keep what they held. Set Command first: it must be one T
  // carries.
  template <class T, std::invocable<T&> Edit>
  void UpdateData(Edit&& edit) {
    DCHECK(T::Carries(Command));
    data_.Update<T>(std::forward<Edit>(edit));
  }

  NetCommandType Command{};       // One of the enum's defined above
  char Name[MPLAYER_NAME_MAX]{};  // Player or Game Name

 private:
  base::Payload<GameInfoData, PlayerInfoData, ScenarioInfoData, MessageData,
                ResponseTimeData, RejectData, ChatData>
      data_;
};

static_assert(sizeof(GlobalPacketType) == 144);

/*
**	This is the identifier for a multiplayer mission. This can be used to
**	identify the filename of the mission as well as display the mission in a
**	mission selection list.
*/
class MultiMission {
 public:
  explicit MultiMission(const char* filename = nullptr,
               const char* description = nullptr, const char* digest = nullptr,
               bool official = true, bool expansion = false);

  void Set_Description(const char* description);
  void Set_Filename(const char* filename);
  void Set_Digest(const char* digest);
  void Set_Official(bool official);
  void Set_Expansion(bool expansion);
  [[nodiscard]] const char* Description() const ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return ScenarioDescription;
  }
  [[nodiscard]] const char* Get_Filename() const ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return Filename;
  }
  [[nodiscard]] const char* Get_Digest() const ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return Digest;
  }
  [[nodiscard]] std::span<const char> Get_Digest_Bytes() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return Digest;
  }
  [[nodiscard]] bool Get_Official() const { return IsOfficial; }
  [[nodiscard]] bool Get_Expansion() const {
    return IsExpansion;
  }  //	Implied "IsCounterstrike". No longer used. -ajw

 private:
  char ScenarioDescription[kDescripMax]{};
  char Filename[engine::platform::kMaxFname + engine::platform::kMaxExt]{};
  char Digest[32]{};
  bool IsOfficial = false;
  bool IsExpansion = false;
};

struct GameOptionsType {
  int ScenarioIndex;  // Used on host machine only as index into scenario list
  int Bases;
  int Credits;
  int Tiberium;
  int Goodies;
  int Ghosts;
  int UnitCount;
  int AIPlayers;                          // # of AI players allowed to be built
  char ScenarioDescription[kDescripMax];  // Used on client machines only
};

//---------------------------------------------------------------------------
// Class Definition
//---------------------------------------------------------------------------
class SessionClass {
  //------------------------------------------------------------------------
  // Public interface
  //------------------------------------------------------------------------
 public:
  //.....................................................................
  // Constructor/Destructor
  //.....................................................................
  SessionClass();
  ~SessionClass();
  SessionClass(const SessionClass&) = delete;
  SessionClass& operator=(const SessionClass&) = delete;
  SessionClass(SessionClass&&) = delete;
  SessionClass& operator=(SessionClass&&) = delete;

  //.....................................................................
  // Initialization
  //.....................................................................
  void One_Time();
  void Init();

  //.....................................................................
  // Reads/writes to the INI file
  //.....................................................................
  void Read_MultiPlayer_Settings();
  void Write_MultiPlayer_Settings();
  void Read_Scenario_Descriptions();
  void Free_Scenario_Descriptions();

  //.....................................................................
  // Utility functions
  //.....................................................................
  int Create_Connections();
  static bool Am_I_Master();
  static uint32_t Compute_Unique_ID();

  //.....................................................................
  // File I/O
  //.....................................................................
  // Session state shared by saved games and recordings. Connected peers stay local.
  template <class Archive>
  void Serialize(Archive& ar);
  // Recording-only player roster; network load keeps the connected roster.
  template <class Archive>
  void SerializePlayers(Archive& ar);

  int Save(ByteSink& file);
  bool Load(ByteSource& file);

  //---------------------------------------------------------------------
  // Public Data
  //---------------------------------------------------------------------
  //.....................................................................
  // The type of session being played
  //.....................................................................
  GameType Type{GAME_NORMAL};

  //.....................................................................
  // The current communications protocol
  //.....................................................................
  CommProtocolType CommProtocol{DEFAULT_COMM_PROTOCOL};

  //.....................................................................
  // Whether this multiplayer game runs under the Aftermath rules, which
  // decides whether RULES.INI is followed by AFTRMATH.INI and whether the
  // expansion's units can be built. Not saved: the host tells every guest
  // when the game is set up.
  //.....................................................................
  bool IsAftermath{false};

  //.....................................................................
  // Game options
  //.....................................................................
  GameOptionsType Options{};

  //.....................................................................
  // Unique workstation ID, for detecting my own packets
  //.....................................................................
  int UniqueID{0};

  //.....................................................................
  // Player's local options
  //.....................................................................
  char Handle[MPLAYER_NAME_MAX]{};  // player name
  PlayerColorType PrefColor;      // preferred color index
  PlayerColorType ColorIdx;       // actual color index
  HousesType House{HOUSE_GOOD};   // GDI / NOD
  bool ObiWan{false};             // 1 = player can see all
  int Solo{0};                    // 1 = player can play alone

  //.....................................................................
  // Max allowable # of players & actual # of (human) players
  //.....................................................................
  int MaxPlayers{8};
  int NumPlayers{0};

  //.....................................................................
  // Frame-sync'ing timing variables
  // 'MaxAhead' is the number of frames ahead of this one to execute
  // a given packet.  It's set by the RESPONSE_TIME event.
  // 'FrameSendRate' is the # frames between data packets
  //.....................................................................
  int MaxAhead{5};
  int64_t FrameSendRate{DEFAULT_FRAME_SEND_RATE};

  int DesiredFrameRate = 0;

  int64_t ProcessTimer = 0;
  int64_t ProcessTicks = 0;
  int ProcessFrames = 0;

  //.....................................................................
  // This flag is set when we've loaded a multiplayer game.
  //.....................................................................
  bool LoadGame{false};

  //.....................................................................
  // This flag is set when the modem game saves the game due to a lost
  // connection.
  //.....................................................................
  bool EmergencySave{false};

  //.....................................................................
  // List of scenarios & their file numbers
  //.....................................................................
  game::OwnedVector<MultiMission> Scenarios;
  //		DynamicVectorClass <char *> Scenarios;
  //		DynamicVectorClass <int> Filenum;

  char ScenarioFileName[engine::platform::kMaxFname +
                        engine::platform::kMaxExt +
                        1]{};  // File name of scenario to load

  char ScenarioDigest[32]{};  // Digest of scenario to load
  unsigned int ScenarioFileLength = 0;
  bool ScenarioIsOfficial = false;

  char ScenarioRequests[20]{};  // Which players requested scenario files
  int RequestCount = 0;
  engine::net::IPXAddressClass HostAddress;

  //.....................................................................
  // This is the multiplayer messaging system
  //.....................................................................
  MessageListClass Messages;
  engine::net::IPXAddressClass MessageAddress;
  char LastMessage[MAX_MESSAGE_LENGTH]{};
  bool WWChat : 1 {false};  // 1 = go into special WW Chat mode

  //.....................................................................
  // This is the multiplayer scorekeeping system
  //.....................................................................
  MPlayerScoreType Score[MAX_MULTI_NAMES]{};
  int GamesPlayed = 0;  // # games played this run
  int NumScores = 0;  // # active entries in MPlayerScore
  int Winner = 0;   // index of winner of last game
  int CurGame = 0;  // index of current game being played

  //.....................................................................
  // Static arrays
  //.....................................................................
  static char Descriptions[100][40];
  static int CountMin[2];
  static int CountMax[2];

  //.....................................................................
  // For Recording & Playing back a file
  //.....................................................................
  // The name of the file a recording is written to or played back from.
  [[nodiscard]] const std::string& record_file_name() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return record_file_name_;
  }
  // The open stream over record_file_name(): non-null while a game is being
  // recorded or played back, null otherwise.
  std::unique_ptr<ByteStream>& record_stream() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return record_stream_;
  }
  bool Record : 1 {false};  // set via command line
  bool Play : 1 {false};
  bool Attract : 1 {false};

  //.....................................................................
  // IPX-specific variables
  //.....................................................................
  int IsBridge{0};                            // 1 = we're crossing a bridge
  engine::net::IPXAddressClass BridgeNet;     // address of bridge
  bool NetStealth{false};                     // makes us invisible
  bool NetProtect{true};                      // keeps others from messaging us
  bool NetOpen{false};                        // 1 = game is open for joining
  char GameName[MPLAYER_NAME_MAX]{};          // game's name
  GlobalPacketType GPacket{};                 // global packet
  int GPacketlen = 0;                         // global packet length
  engine::net::IPXAddressClass GAddress;      // address of sender
  uint16_t GProductID{0};                     // product ID of sender
  char MetaPacket[MAX_IPX_PACKET_SIZE]{};     // packet building buffer
  int MetaSize;                               // size of MetaPacket
  game::OwnedVector<GameNode> Games;          // list of games
  game::OwnedVector<PlayerNode> Players;      // list of players
  game::OwnedVector<ChatNode> Chat;           // list of chat nodes

  //.....................................................................
  // Modem-specific variables
  //.....................................................................
  bool ModemService : 1 {true};         // 1 = service modem in ServiceRealTime
  int CurPhoneIdx{0};                   // phone listing index
  SerialSettingsType SerialDefaults{};  // default serial settings
  ModemGameType ModemType{MODEM_NULL_HOST};  // caller or answerer?

  game::OwnedVector<PhoneEntryClass> PhoneBook;
  game::OwnedVector<InitString> InitStrings;
  static base::EnumArray<DialMethodType, const char*,
                         static_cast<int>(DIAL_METHODS)>
      DialMethodCheck;
  static const char* CallWaitStrings[kCallWaitStringsNum];

  //.....................................................................
  // For finding Sync Bugs
  //.....................................................................
  int32_t TrapPrintCRC{0};  // Frame # to print CRC state file

 private:
  std::string record_file_name_ = "RECORD.BIN";
  std::unique_ptr<ByteStream> record_stream_;
};

extern template void SessionClass::SerializePlayers<ArchiveWriter>(ArchiveWriter&);
extern template void SessionClass::SerializePlayers<ArchiveReader>(ArchiveReader&);

extern template void PlayerNode::Serialize<ArchiveWriter>(ArchiveWriter&);
extern template void PlayerNode::Serialize<ArchiveReader>(ArchiveReader&);
extern template void SessionClass::Serialize<ArchiveWriter>(ArchiveWriter&);
extern template void SessionClass::Serialize<ArchiveReader>(ArchiveReader&);

bool Is_Mission_126x126(const char* file_name);

// Returns the SessionClass that Game installed. CHECK-fails outside a
// Game's lifetime unless a test installed its own.
inline SessionClass& TheSession() {
  return base::Installed<SessionClass>::Get();
}

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_NET_SESSION_H_

/*************************** end of session.h ******************************/
