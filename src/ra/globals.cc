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

#include <cstdint>

#include "ra/compat.h"
#include "ra/credits.h"
#include "ra/defines.h"
#include "ra/externs.h"
#include "ra/jshell.h"
#include "ra/version.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/wwstd.h"
#include "winvq/vqa32/vqaplay.h"



/*
**	These are the instantiate static heap pointers for the various
**	CCPtr class objects that are allowed to exist. If the linker generates
**	an error about a missing heap pointer, then this indicates that CCPtr
*objects *	for that type are not allowed. For every case of a TFixedIHeap
*manager of *	game objects, then a CCPtr can be instantiated for it.
*/


/* These variables are used to keep track of the slowest speed of a team */

/*
** Global flag for the life of Tanya.  If this flag is set, she is
** no longer available.
*/





WWMouseClass* WWMouse = nullptr;
bool InMovie = false;  // Are we currently playing a VQ movie?

/***************************************************************************
**	This is true if the game is the currently in focus windows app
**
*/
bool GameInFocus = false;

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

/***************************************************************************
**	This is a list of all selected objects (for this map). The support
*functions *	are used to control access to this list. Do not modify it
*directly.
*/

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

/***************************************************************************
**	Logic processing is controlled by this element. It handles both graphic
**	and AI logic.
*/

// The sound device and its four channels. Defined ahead of Theme, which
// plays through it.

/***************************************************************************
**	This handles the background music.
*/

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
**	This is the list of BuildingTypes that define the AI's base.
*/

/***************************************************************************
**	This is the list of carry over objects. These objects are part of the
**	pseudo saved game that might be carried along with the current saved
**	game.
*/

/***************************************************************************
** This value is computed every time a new scenario is loaded; it's a
** CRC of the INI and binary map files.
*/




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


/***************************************************************************
**	This flag is for popping up dialogs that call the main loop.
*/
SpecialDialogType SpecialDialog = SDLG_NONE;

int RequiredCD = -1;
int CurrentCD = -1;
bool MouseInstalled;


/***************************************************************************
**  Win32 specific globals
*/



