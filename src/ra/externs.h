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

/* $Header: /counterstrike/EXTERNS.H 2     3/10/97 6:23p Steve_tall $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : EXTERNS.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : May 27, 1994 *
 *                                                                                             *
 *                  Last Update : May 27, 1994   [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_RA_EXTERNS_H_
#define CNC_RED_ALERT_RA_EXTERNS_H_

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "base/enum_array.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/base.h"
#include "ra/building.h"
#include "ra/carry.h"
#include "ra/cell.h"
#include "ra/event.h"
#include "ra/goptions.h"
#include "ra/infantry.h"
#include "ra/internet.h"
#include "ra/ipxmgr.h"
#include "ra/logic.h"
#include "ra/mapedit.h"
#include "ra/mouse.h"
#include "ra/overlay.h"
#include "ra/queue.h"
#include "ra/rules.h"
#include "ra/scenario.h"
#include "ra/score.h"
#include "ra/smudge.h"
#include "ra/taction.h"
#include "ra/techno.h"
#include "ra/template.h"
#include "ra/tevent.h"
#include "ra/theme.h"
#include "ra/type.h"
#include "ra/unit.h"
#include "ra/version.h"
#include "ra/vortex.h"
#include "ra/warhead.h"
#include "tech/audio_mixer.h"
#include "tech/buff.h"
#include "tech/game_file.h"
#include "tech/mix_archive.h"
#include "winvq/vqa32/vqaplay.h"

// Scratch space for packing and unpacking the MapPack and OverlayPack INI blocks.
inline char staging_buffer[32000];




extern int NewINIFormat;

extern bool AntsEnabled;


extern const char* NameOverride[25];
extern int NameIDOverride[25];

extern bool GameInFocus;
extern bool InMovie;
extern WWMouseClass* WWMouse;

/*
**	Dynamic global variables (these change or are initialized at run time).
*/
extern int MapTriggerID;
extern int LogicTriggerID;
extern PKey FastKey;
extern KeyboardClass* Keyboard;
extern RandomClass local_rng;
extern std::vector<CarryoverClass> Carryover;
extern ScenarioClass Scen;
extern VersionClass VerNum;
extern bool ScoresPresent;
extern bool AllowVoice;
extern bool PlayerWins;
extern bool PlayerLoses;
extern bool PlayerRestarts;
extern bool BreakoutAllowed;

extern GameOptionsClass Options;

extern LogicClass Logic;
extern MapEditClass Map;
extern ScoreClass Score;
extern AudioMixer Audio;
extern ThemeClass Theme;
extern SpecialClass Special;

/*
**	Game object allocation and tracking classes.
*/

extern QueueClass<EventClass, kMaxEvents> OutList;
extern QueueClass<EventClass, kMaxEvents * 64> DoList;

extern DynamicVectorClass<ObjectClass*> CurrentObject;
extern DynamicVectorClass<TriggerClass*> LogicTriggers;
extern DynamicVectorClass<TriggerClass*> MapTriggers;
extern base::EnumArray<HousesType, DynamicVectorClass<TriggerClass*>>
    HouseTriggers;

extern BaseClass Base;

/* These variables are used to keep track of the slowest speed of a team */
extern MPHType TeamMaxSpeed[10];
extern SpeedType TeamSpeed[10];
extern bool FormMove;
extern SpeedType FormSpeed;
extern MPHType FormMaxSpeed;

extern bool IsTanyaDead;
extern bool SaveTanya;

extern bool TimeQuake;

extern bool PendingTimeQuake;
extern TARGET TimeQuakeCenter;

/*
**	Miscellaneous globals.
*/
extern ChronalVortexClass ChronalVortex;
extern Stopwatch<SystemTickSource> TickCount;
extern HousesType Whom;
extern VQAConfig AnimControl;
extern int ScenarioInit;
extern HouseClass* PlayerPtr;
extern int BuildLevel;
extern uint32_t ScenarioCRC;

extern bool bAftermathMultiplayer;  //	Is multiplayer game being played with
                                    // Aftermath rules?

extern bool bAutoSonarPulse;

extern CELL CurrentCell;

class SessionClass;
extern SessionClass Session;
class NullModemClass;
extern NullModemClass NullModem;
extern IPXManagerClass Ipx;

extern int NewMaxAheadFrame1;
extern int NewMaxAheadFrame2;

extern Timer<SystemTickSource> CountDownTimer;

extern SpecialDialogType SpecialDialog;

extern int RequiredCD;
extern int CurrentCD;
extern bool MouseInstalled;




extern TheaterType LastTheater;

void Do_Vortex(int x, int y, int frame);

[[noreturn]] void Memory_Error_Handler();  // Memory error handler function
void WWDebugString(const char* string);
void Check_For_Focus_Loss();  // Pumps the event queue while focus is lost
void Create_Main_Window(void* instance, int command_show, int width,
                        int height);
void Check_VQ_Palette_Set();  // Applies a palette change queued by a movie

/*************************************************************
** Internet specific externs
*/
extern void* PacketLater;
void Register_Game_Start_Time();
void Register_Game_End_Time();
void Send_Statistics_Packet();

/*
** From SENDFILE.CPP - externs for scenario file transfers
*/
bool Receive_Remote_File(char* file_name, unsigned int file_length,
                         unsigned int crc, int gametype);
bool Send_Remote_File(const char* file_name, int gametype);
bool Get_Scenario_File_From_Host(std::span<char> return_name, size_t dest_size,
                                 int gametype);

bool Find_Local_Scenario(const char* description, std::span<char> filename,
                         unsigned int length, const char* digest,
                         bool official);

void Focus_Loss();
void Focus_Restore();

#endif  // CNC_RED_ALERT_RA_EXTERNS_H_
