/*
**	Command & Conquer(tm)
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

/* $Header:   F:\projects\c&c\vcs\code\globals.cpv   2.17   16 Oct 1995 16:52:22
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
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

#include "td/globals.h"

#include <cstdint>

#include "sdllib/keyboard.h"
#include "sdllib/timer.h"
#include "sdllib/ww_mouse.h"
#include "td/credits.h"
#include "td/defines.h"
#include "td/externs.h"
#include "td/ftimer.h"
#include "td/goptions.h"
#include "td/special.h"
#include "td/theme.h"
#include "tech/audio_mixer.h"
#include "winvq/vqa32/vqaplay.h"

#ifdef JAPANESE
bool ForceEnglish = false;
#endif

int In_Debugger = 0;


#ifdef PATCH
/***************************************************************************
**	Compatibility with version 1.07 flag.
*/
bool IsV107 = false;
char OverridePath[128] = ".";
#endif

/***************************************************************************
**	This is a list of all selected objects (for this map). The support
*functions *	are used to control access to this list. Do not modify it
*directly.
*/

/***************************************************************************
**	This holds the custom version text that is fetched from the version
**	text file. This version is displayed on the options dialog.
*/
char VersionText[16];

/***************************************************************************
**	This is the VQ animation controller structure. It is filled in by
*reading *	the PLAYER.INI and overridden through program control.
*/
VQAConfig AnimControl;

bool PreserveVQAScreen;       // Used for screen mode transition control.
bool BreakoutAllowed = true;  // "true" if aborting of movies is allowed.

/***************************************************************************
**	These are the movie names to use for mission briefing, winning, and
*losing *	sequences. They are read from the INI file.
*/

/***************************************************************************
**	This records the view hotspots for the player. These are the cell
*numbers *	of the upper left corner for the view position.
*/

/***************************************************************************
**	This records if the score (music) file is present. If not, then much of
**	the streaming score system can be disabled.
*/
bool ScoresPresent;

/***************************************************************************
**	This flag will control whether there is a response from game units.
**	By carefully controlling this global, multiple responses are supressed
**	when a large group of infantry is given the movement order.
*/
bool AllowVoice = true;

/***************************************************************************
**	This counts the number of crates on the map. When this value reaches
*zero, *	then a timer is started that will control crate creation.
*/
int CrateCount;
TCountDownTimerClass CrateTimer;
bool CrateMaker = false;

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
**	This is a special scenario count down value. End of game condition will
**	not be checked until this value reaches zero.
*/

/***************************************************************************
**	When the player sabotages a building (scenario #6 GDI only) then when
**	the next scenario starts, that building will already be destroyed.
*/

/***************************************************************************
**	If the Nod temple was destroyed by the ion cannon, then this flag will
**	be set to true.
*/

/***************************************************************************
**	This is true if the game is the currently in focus windows app
**
*/
bool GameInFocus;

/***************************************************************************
**	This is the options control class. The options control such things as
**	game speed, visual controls, and other user settings.
*/
GameOptionsClass Options;

/***************************************************************************
**	Logic processing is controlled by this element. It handles both graphic
**	and AI logic.
*/

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

/**************************************************************************
**	The running game score is handled by this class (and member functions).
*/

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
ScenarioVarType ScenVar;  // variation A/B/C

/***************************************************************************
** This flag is for the special command-line options.
*/
bool SpecialFlag = false;

/***************************************************************************
**	The game plays as long as this var is true.
*/
bool GameActive;

/***************************************************************************
**	This flag is for popping up dialogs that call the main loop.
*/
SpecialDialogType SpecialDialog = SDLG_NONE;

/***************************************************************************
** This value tells the sidebar what items it's allowed to add.  The
** lower the value, the simpler the sidebar will be.
*/

/***************************************************************************
**	This is a scratch variable that is used to when a reference is needed to
**	a long, but the value wasn't supplied to a function. This is used
**	specifically for the default reference value. As such, it is not stable.
*/

/***************************************************************************
** The currently-selected cell for the Scenario Editor
*/

/***************************************************************************
**	This is the house that the human player is currently playing.
*/

/***************************************************************************
**	These are the event queues. One is for holding events until they are
*ready to be *	sent to the remote computer for processing. The other list is
*for incoming events *	that need to be executed when the correct frame has been
*reached.
*/

/***************************************************************************
**	These are arrays/lists of trigger pointers for each cell & the houses.
*/

/***************************************************************************
**	This is an array of waypoints; each waypoint corresponds to a letter of
** the alphabet, and points to a cell number.  -1 means unassigned.
** The CellClass has a bit that tells if that cell has a waypoint attached to
** it; the only way to find which waypoint it is, is to scan this array.  This
** shouldn't be needed often; usually, you know the waypoint & you want the
*CELL.
*/

/***************************************************************************
**	This is the list of BuildingTypes that define the AI's base.
*/

/***************************************************************************
**	This value tells what type of multiplayer game we're playing.
*/

/***************************************************************************
**	This is the current communications protocol
*/


/***************************************************************************
**	This string stores the player's name.
*/

/***************************************************************************
**	This is the array of remap colors.  Each player in a network game is
** assigned one of these colors.  The 'G' is for graphics drawing; the 'T'
** is for text printing (indicates a remap table for the font to use).
*/


/***************************************************************************
**	This is a list of all the names of the multiplayer scenarios that use
** bases (production), and those that don't.  There is a list for
** descriptions, and another for actual filenames.
*/

/***************************************************************************
**	This value determines the max allowable # of players.
*/

/***************************************************************************
**	Multiplayer game options
*/

/*---------------------------------------------------------------------------
MPlayerMaxAhead is the number of frames ahead of this one to execute a given
packet.  It's set by the RESPONSE_TIME event.
---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
'FrameSendRate' is the # frames between data packets
'FrameRateDelay' is the time ticks to wait between frames, for smoothing.
---------------------------------------------------------------------------*/

/***************************************************************************
**	Multiplayer ID's, stored in order of event execution.
** Format:
** bits 0-3: the "preferred" house of the player (GDI/NOD)
** bits 4-7: the player's Color Index
** These values are used as the IPX connection ID's.
*/

/***************************************************************************
** This array stores the actual HousesType for all players (MULT1, etc).
*/

/***************************************************************************
** This array stores the names of all players in a multiplayer game.
*/

/***************************************************************************
**	This is a list of the messages received from / sent to other players,
** the address to send to (IPX only), and the last message received or
** sent (for the computer's messages).
*/

/***************************************************************************
** If this flag is set, computer AI will blitz the humans all at once;
** otherwise, the computer units trickle gradually out.
*/

/***************************************************************************
** If this flag is set, we can move around the map, but we can't do anything.
** It means we've been defeated, but we're still allowed to watch the action.
*/

/***************************************************************************
** These variables keep track of the multiplayer game scores.
*/

/***************************************************************************
**	These variables are just to help find sync bugs.
*/
int32_t TrapFrame = 0x7fffffff;    // frame to start trapping object values at
RTTIType TrapObjType = RTTI_NONE;  // type of object to trap
COORDINATE TrapCoord = 0;               // COORD of object to trap
void* TrapThis = nullptr;               // 'this' ptr of object to trap
int TrapCheckHeap = 0;                  // start checking the Heap

/***************************************************************************
**	This is the network IPX manager class.  It handles multiple remote
** connections.  Declaring this class doesn't perform any allocations;
** the class itself is 140 bytes.
*/
// #endif

/***************************************************************************
**	This is the user-specified IPX address of a desired game owner machine.
** Use this to cross a bridge.  Only the 1st 4 numbers in the address are
** used; the rest are set to ff's, for broadcasting.  'IsBridge' is set
** if this address should be used.
*/

/***************************************************************************
**	This flag is true if the user has requested that this game be "secret"
** (The game will not appear to other systems just starting up.)
*/

/***************************************************************************
**	If this flag is true, the user won't receive messages from any player
** other than those in his own game. It defaults to protected mode.
*/

/***************************************************************************
**	This flag indicates whether the game is "open" or not to other network
*players.
*/

/***************************************************************************
**	This string stores the game's network name.
** GameName does not include the "'s Game"; comparing GameName to
** PlayerName can determine if this player is the originator of the game.
*/

/***************************************************************************
**	These variables are for servicing the Global Channel.
*/

/***************************************************************************
**	This is the random-number seed; it's synchronized between systems for
** multiplayer games.
*/


bool SoundOn;
static CountDownTimerClass DebugTimer{0L};
CountDownTimerClass CountDownTimer{0L};

NewConfigType NewConfig;

/***************************************************************************
**	These measure how long (in ticks) it takes to process the game's logic,
** with no packet processing or artificial delays.
*/

/*
** This flags if used to tell can enter cell that we are in a find path
** check and thus should not uncloak units via Can_Enter_Cell.
*/
// bool	IsFindPath = false;

/***************************************************************************
**	Globals for the network Dialogs.
*/

/*
**	List of all games out there, & the address of the game's owner
*/

/*
**	List of names & addresses of all the players in the game I'm joining.
**	This is the really critical list, since it's used to form connections
*with *	all other players in my game.  It's updated when I get a response to my
**	outgoing query, or when I get a query from another system in my game
*asking *	who I am.  This double-insurance means that if any system knows
*about me, *	I know about them too.  The only catch is that if the game is
*started very, *	very soon after a player joins, not everyone may know
*about him; to prevent *	this, a timer restriction is put on the New Game
*dialog's GO button.
*/

#ifdef DEMO
int RequiredCD = -2;
#else
int RequiredCD = -1;
#endif
bool MouseInstalled;

/*
** Certain options must be enabled by both a command-line option, and an
** an entry in an INI file.  If this flag is 'true', those options have been
** enabled by the INI file.
*/
bool AreThingiesEnabled = false;

WWKeyboardClass Kbd;
WWMouseClass* WWMouse = nullptr;
bool InMovie = false;       // Are we currently playing a VQ movie?
bool MMXAvailable = false;  // Does this CPU support MMX extensions?

TheaterType LastTheater = THEATER_NONE;

WWKeyboardClass* ActiveKeyboard = &Kbd;
