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

/* $Header: /counterstrike/GLOBALS.CPP 2     3/10/97 6:22p Steve_tall $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : GLOBALS.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : September 10, 1993 *
 *                                                                                             *
 *                  Last Update : September 10, 1993   [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "ra/globals.h"

#include <algorithm>
#include <cstdint>
#include <vector>

#include "base/enum_array.h"
#include "ra/base.h"
#include "ra/carry.h"
#include "ra/ccptr.h"
#include "ra/compat.h"
#include "ra/connect.h"
#include "ra/credits.h"
#include "ra/defines.h"
#include "ra/event.h"
#include "ra/externs.h"
#include "ra/goptions.h"
#include "ra/house.h"
#include "ra/ipxgconn.h"
#include "ra/ipxmgr.h"
#include "ra/jshell.h"
#include "ra/logic.h"
#include "ra/mapedit.h"
#include "ra/nullmgr.h"
#include "ra/object.h"
#include "ra/queue.h"
#include "ra/scenario.h"
#include "ra/score.h"
#include "ra/session.h"
#include "ra/special.h"
#include "ra/theme.h"
#include "ra/trigger.h"
#include "ra/vector_dynamic.h"
#include "ra/version.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/wwstd.h"
#include "tech/audio_mixer.h"
#include "tech/ftimer.h"
#include "tech/pk.h"
#include "tech/random.h"
#include "winvq/vqa32/vqaplay.h"



/*
**	These are the instantiate static heap pointers for the various
**	CCPtr class objects that are allowed to exist. If the linker generates
**	an error about a missing heap pointer, then this indicates that CCPtr
*objects *	for that type are not allowed. For every case of a TFixedIHeap
*manager of *	game objects, then a CCPtr can be instantiated for it.
*/


/* These variables are used to keep track of the slowest speed of a team */
MPHType TeamMaxSpeed[10];
SpeedType TeamSpeed[10];
bool FormMove;
SpeedType FormSpeed;
MPHType FormMaxSpeed;

/*
** Global flag for the life of Tanya.  If this flag is set, she is
** no longer available.
*/
bool IsTanyaDead;
bool SaveTanya;

bool AntsEnabled = false;

int NewINIFormat = 0;

bool TimeQuake;

bool PendingTimeQuake;
TARGET TimeQuakeCenter;

WWMouseClass* WWMouse = nullptr;
bool InMovie = false;  // Are we currently playing a VQ movie?

/***************************************************************************
**	This is true if the game is the currently in focus windows app
**
*/
bool GameInFocus = false;

/***************************************************************************
**	Encryption keys.
*/
PKey FastKey;

/***************************************************************************
**	This is where the name overrides for the units will reside.
*/
const char* NameOverride[25];
int NameIDOverride[25];

/***************************************************************************
**	These are the mission control structures. They hold the information
*about *	how the missions should behave in the system.
*/

/***************************************************************************
**	This holds the rules database. The rules database won't change during
*the *	program's run, but may need to be referenced intermitently.
*/

/***************************************************************************
**	General rules that control the game.
*/

/***************************************************************************
** All keyboard input is routed through the object pointed to by this
**	keyboard class pointer.
*/
KeyboardClass* Keyboard;

// Source of random numbers for events that must NOT affect game logic, such as
// cosmetic animations and one-machine-only effects. Because it never influences
// the simulation, it does not need to stay in sync across networked machines.
// For sync-critical randomness use Scen.sync_rng_.
RandomClass local_rng;

/***************************************************************************
**	This is a list of all selected objects (for this map). The support
*functions *	are used to control access to this list. Do not modify it
*directly.
*/
DynamicVectorClass<ObjectClass*> CurrentObject;

/***************************************************************************
**	This is the game version.
*/
VersionClass VerNum;

/***************************************************************************
**	This is the VQ animation controller structure. It is filled in by
*reading *	the PLAYER.INI and overridden through program control.
*/
VQAConfig AnimControl;

bool BreakoutAllowed = true;  // "true" if aborting of movies is allowed.

/***************************************************************************
**	These are the movie names to use for mission briefing, winning, and
*losing *	sequences. They are read from the INI file.
*/
ScenarioClass Scen;

/***************************************************************************
**	This records if the score (music) file is present. If not, then much of
**	the streaming score system can be disabled.
*/
bool ScoresPresent;

/***************************************************************************
**	This flag will control whether there is a response from game units.
**	By carefully controlling this global, multiple responses are suppressed
**	when a large group of infantry is given the movement order.
*/
bool AllowVoice = true;

/***************************************************************************
**	This is the current frame number. This number is guaranteed to count
**	upward at the rate of one per game logic process. The target rate is 15
**	per second. This value is saved and restored with the saved game.
*/

/***************************************************************************
**	These globals are constantly monitored to determine if the player
**	has won or lost. They get set according to the trigger events associated
**	with the scenario.
*/
bool PlayerWins;
bool PlayerLoses;
bool PlayerRestarts;

/***************************************************************************
**	This is the options control class. The options control such things as
**	game speed, visual controls, and other user settings.
*/
GameOptionsClass Options;

/***************************************************************************
**	Logic processing is controlled by this element. It handles both graphic
**	and AI logic.
*/
LogicClass Logic;

// The sound device and its four channels. Defined ahead of Theme, which
// plays through it.
AudioMixer Audio;

/***************************************************************************
**	This handles the background music.
*/
ThemeClass Theme;

/***************************************************************************
**	This is the main control class for the map.
*/
MapEditClass Map;

/**************************************************************************
**	The running game score is handled by this class (and member functions).
*/
ScoreClass Score;

/***************************************************************************
**	The running credit display is controlled by this class (and member
**	functions.
*/
static CreditClass CreditDisplay;

/**************************************************************************
** This class records the special command override options that C&C
**	supports.
*/
SpecialClass Special;

/***************************************************************************
**	This is the scenario data for the currently loaded scenario.
** These variables should all be set together.
*/
HousesType Whom;  // Initial command line house choice.
int ScenarioInit;

/***************************************************************************
** This value tells the sidebar what items it's allowed to add.  The
** lower the value, the simpler the sidebar will be. This value is the
**	displayed value for tech level in the multiplay dialogs. It remaps to
**	the in-game rules.ini tech levels.
*/
int BuildLevel = 10;  // Buildable level (1 = simplest)

/***************************************************************************
**	The game plays as long as this var is true.
*/
bool GameActive;

/***************************************************************************
**	This is a scratch variable that is used to when a reference is needed to
**	a long, but the value wasn't supplied to a function. This is used
**	specifically for the default reference value. As such, it is not stable.
*/
int32_t LParam;

/***************************************************************************
** The currently-selected cell for the Scenario Editor
*/
CELL CurrentCell = 0;

/***************************************************************************
**	This is the house that the human player is currently playing.
*/
HouseClass* PlayerPtr;

/***************************************************************************
**	These are the event queues. One is for holding events until they are
*ready to be *	sent to the remote computer for processing. The other list is
*for incoming events *	that need to be executed when the correct frame has been
*reached.
*/
QueueClass<EventClass, kMaxEvents> OutList;
QueueClass<EventClass, kMaxEvents * 64> DoList;

/***************************************************************************
**	These are arrays/lists of trigger pointers for each cell & the houses.
*/
base::EnumArray<HousesType, DynamicVectorClass<TriggerClass*>> HouseTriggers;
DynamicVectorClass<TriggerClass*> MapTriggers;
int MapTriggerID;
DynamicVectorClass<TriggerClass*> LogicTriggers;
int LogicTriggerID;

/***************************************************************************
**	This is the list of BuildingTypes that define the AI's base.
*/
BaseClass Base;

/***************************************************************************
**	This is the list of carry over objects. These objects are part of the
**	pseudo saved game that might be carried along with the current saved
**	game.
*/
std::vector<CarryoverClass> Carryover;

/***************************************************************************
** This value is computed every time a new scenario is loaded; it's a
** CRC of the INI and binary map files.
*/
uint32_t ScenarioCRC;

/***************************************************************************
** This class manages data specific to multiplayer games.
*/
SessionClass Session;

//
// These values store the min & max frame #'s for when MaxAhead >>increases<<.
// If MaxAhead increases, and the other systems free-run to the new MaxAhead
// value, they may miss an event generated after the MaxAhead event was sent,
// but before it executed, since it will have been scheduled with the older,
// shorter MaxAhead value.  This will cause a Packet_Received_Too_Late error.
// The frames from the point where the new MaxAhead takes effect, up to that
// frame Plus the new MaxAhead, represent a "period of vulnerability"; any
// events received that are scheduled to execute during this period should
// be re-scheduled for after that period.
//
int NewMaxAheadFrame1;
int NewMaxAheadFrame2;

bool bAftermathMultiplayer;  //	Is multiplayer game being played with Aftermath
                             // rules?

/***************************************************************************
**	This is the null modem manager class.  Declaring this class doesn't
** perform any allocations;
*/
NullModemClass NullModem(16,  // number of send entries
                         16,  // number of receive entries
                         (MAX_SERIAL_PACKET_SIZE / sizeof(EventClass) *
                          sizeof(EventClass)) +
                             sizeof(CommHeaderType),
                         0x1234);  // Magic number must have each digit unique
                                   // and different from the queue magic number

/***************************************************************************
**	This is the network IPX manager class.  It handles multiple remote
** connections.  Declaring this class doesn't perform any allocations;
** the class itself is 140 bytes.
*/
// IPXManagerClass Ipx (
//	std::max (sizeof (GlobalPacketType), sizeof(RemoteFileTransferType)),
//// size of Global Channel packets
//	((546 - sizeof(CommHeaderType)) / sizeof(EventClass) ) *
// sizeof(EventClass), 	10,
//// # entries in Global Queue 	8,
//// # entries in Private Queues 	VIRGIN_SOCKET,
//// Socket ID # 	IPXGlobalConnClass::kCommandAndConquer0);// Product ID
/// #

IPXManagerClass Ipx(
    std::max(sizeof(GlobalPacketType),
             sizeof(RemoteFileTransferType)),  // size of Global Channel packets
    (546 - sizeof(CommHeaderType)) / sizeof(EventClass) * sizeof(EventClass),
    160,                                       // # entries in Global Queue
    32,                                        // # entries in Private Queues
    VIRGIN_SOCKET,                             // Socket ID #
    IPXGlobalConnClass::kCommandAndConquer0);  // Product ID #

/***************************************************************************
**	This is the random-number seed; it's synchronized between systems for
** multiplayer games.
*/
int Seed = 0;

int WindowList[][8] = {
    /* xbyte, ypixel, bytewid, pixelht, cursor color, bkgd color,	cursor
       x, cursor y */

    /* do not change the first 2 entries!! they are necc. to the system */

    {0, 0, 40 * 16, 400, kWhite, kBlack, 0, 0},     /* screen window */
    {1 * 8, 75, 38 * 8, 100, kWhite, kBlack, 0, 0}, /* DOS Error window */

    // Tactical map.
    {0, 0, 40 * 16, 400, kWhite, kLtGrey, 0, 0},

    // Initial menu window.
    {12 * 8, 199 - 42, 16 * 8, 42, kLtGrey, DKGREY, 0, 0},

    // Sidebar clipping window.
    {0, 0, 0, 0, 0, 0, 0, 0},

    // Scenario editor window.
    {5 * 8, 30, 30 * 8, 140, 0, 0, 0, 0},

    // Partial object draw sub-window.
    {0, 0, 0, 0, kWhite, kBlack, 0, 0}};


bool SoundOn;
Timer<SystemTickSource> CountDownTimer;

TheaterType LastTheater =
    THEATER_NONE;  // Lets us know when theater type changes.

/***************************************************************************
**	This flag is for popping up dialogs that call the main loop.
*/
SpecialDialogType SpecialDialog = SDLG_NONE;

int RequiredCD = -1;
int CurrentCD = -1;
bool MouseInstalled;

/***************************************************************************
** Tick Count global timer object.
*/
Stopwatch<SystemTickSource> TickCount;

/***************************************************************************
**  Win32 specific globals
*/



bool bAutoSonarPulse = false;
