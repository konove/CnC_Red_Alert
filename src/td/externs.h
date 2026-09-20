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

/* $Header:   F:\projects\c&c\vcs\code\externs.h_v   2.15   16 Oct 1995 16:45:34
 * JOE_BOSTIC  $ */
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

#ifndef CNC_RED_ALERT_TD_EXTERNS_H_
#define CNC_RED_ALERT_TD_EXTERNS_H_

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "base/enum_array.h"
#include "port/platform.h"
#include "td/aircraft.h"
#include "td/anim.h"
#include "td/base.h"
#include "td/building.h"
#include "td/bullet.h"
#include "td/cell.h"
#include "td/event.h"
#include "td/factory.h"
#include "td/goptions.h"
#include "td/heap.h"
#include "td/house.h"
#include "td/infantry.h"
#include "td/ipxmgr.h"
#include "td/logic.h"
#include "td/mapedit.h"
#include "td/msglist.h"
#include "td/nodename.h"
#include "td/nullmgr.h"
#include "td/overlay.h"
#include "td/phone.h"
#include "td/queue.h"
#include "td/score.h"
#include "td/smudge.h"
#include "td/team.h"
#include "td/teamtype.h"
#include "td/template.h"
#include "td/terrain.h"
#include "td/theme.h"
#include "td/trigger.h"
#include "td/type.h"
#include "td/unit.h"
#include "tech/audio_mixer.h"
#include "tech/game_file.h"
#include "tech/mix_archive.h"
#include "winvq/vqa32/vqaplay.h"

#ifdef JAPANESE
extern bool ForceEnglish;
#endif

extern std::span<const std::byte> WarFactoryOverlay;

/*
**	Dynamic global variables (these change or are initialized at run time).
*/
#ifdef PATCH
extern bool IsV107;
extern char OverridePath[128];
#endif
extern char VersionText[16];
extern bool ScoresPresent;
extern int CrateCount;
extern TCountDownTimerClass CrateTimer;
extern bool CrateMaker;
extern bool AllowVoice;
extern NewConfigType NewConfig;
extern bool PlayerWins;
extern bool PlayerLoses;
extern bool PlayerRestarts;
extern bool PreserveVQAScreen;
extern bool BreakoutAllowed;

extern GameOptionsClass Options;

extern AudioMixer Audio;
extern ThemeClass Theme;

/*
**	Game object allocation and tracking classes.
*/





/*
**	Loaded data file pointers.
*/

/*
**	Miscellaneous globals.
*/
extern VQAConfig AnimControl;
extern bool SpecialFlag;
extern ScenarioVarType ScenVar;






/*
** Modem globals
*/

/*
** Network/Modem globals
*/

extern int32_t TrapFrame;
extern RTTIType TrapObjType;
extern COORDINATE TrapCoord;
extern void* TrapThis;
extern int TrapCheckHeap;

/*
** Network (IPX) globals
*/

/*
**	Constant externs (data is not modified during game play).
*/

extern bool SoundOn;
extern CountDownTimerClass CountDownTimer;


extern SpecialDialogType SpecialDialog;
// extern bool						IsFindPath;

extern int RequiredCD;
extern bool MouseInstalled;
extern bool AreThingiesEnabled;

extern WWKeyboardClass Kbd;
extern int In_Debugger;
extern WWMouseClass* WWMouse;
extern HANDLE hInstance;
extern "C" bool MMXAvailable;
extern int Get_CD_Index(int cd_drive, int timeout);
[[noreturn]] void Memory_Error_Handler();
extern bool InMainLoop;  // True if in game state rather than menu state
void CCDebugString(const char* string);
void Load_Title_Screen(const char* name, GraphicViewPortClass* video_page,
                       std::span<unsigned char> palette);

extern TheaterType LastTheater;

bool Do_The_Internet_Menu_Thang();
bool Spawn_WChat(bool can_launch);
extern bool SpawnedFromWChat;
extern bool VQPaletteChange;
extern int WChatMaxAhead;
extern int WChatSendRate;
extern uint32_t PlanetWestwoodGameID;
extern uint32_t PlanetWestwoodStartTime;
void Check_For_Focus_Loss();
void Create_Main_Window(void* instance, int command_show, int width,
                        int height);
void Check_From_WChat(const char* wchat_name);
void Check_VQ_Palette_Set();
void Focus_Loss();
void Focus_Restore();
void Register_Game_End_Time();
void Register_Game_Start_Time();
void Send_Statistics_Packet();

#endif  // CNC_RED_ALERT_TD_EXTERNS_H_
