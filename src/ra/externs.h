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

#include "ra/base.h"
#include "ra/building.h"
#include "ra/carry.h"
#include "ra/ccfile.h"
#include "ra/cell.h"
#include "ra/event.h"
#include "ra/goptions.h"
#include "ra/infantry.h"
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
#include "sdllib/playcd.h"
#include "tech/bench.h"
#include "tech/mixfile.h"
#include "tech/rndstraw.h"

extern char _staging_buffer[32000];
extern "C" {
void _PRO();
}

/*
**	Convenient alias for MixFileClass<CDFileClass> object. This allows
**	easier entry into the code and less clutter.
*/
typedef MixFileClass<CCFileClass> MFCD;

extern bool IsVQ640;
extern unsigned long GameVersion;
extern bool Debug_MotionCapture;
extern bool Debug_Quiet;
extern bool Debug_Cheat;
extern bool Debug_Remap;
extern bool Debug_Flag;
extern bool Debug_Lose;
extern bool MapEditorActive;
extern bool Debug_Win;
extern bool Debug_Icon;
extern bool Debug_Passable;
extern bool Debug_Unshroud;
extern bool Debug_Threat;
extern bool Debug_Find_Path;
extern bool Debug_Check_Map;
extern bool Debug_Playtest;

extern bool Debug_Heap_Dump;
extern bool Debug_Smart_Print;
extern bool Debug_Trap_Check_Heap;
extern bool Debug_Modem_Dump;
extern bool Debug_Print_Events;

extern const void* LightningShapes;

extern int NewINIFormat;

extern bool AntsEnabled;

extern bool NewUnitsEnabled;
extern bool SecretUnitsEnabled;
extern int MTankDistance;
extern int CarrierLaunchDelay;

extern const char* NameOverride[25];
extern int NameIDOverride[25];

extern bool GameInFocus;
extern unsigned char* InterpolatedPalettes[100];
extern bool PalettesRead;
extern unsigned PaletteCounter;
extern int AllDone;
extern bool InMovie;
extern WWMouseClass* WWMouse;
extern GraphicBufferClass HiddenPage;
#define SeenPage SeenBuff
extern GraphicBufferClass VisiblePage;
extern GraphicBufferClass SysMemPage;
extern int ScreenWidth;
extern int ScreenHeight;
extern GraphicBufferClass ModeXBuff;

/*
**	Dynamic global variables (these change or are initialized at run time).
*/
extern MissionControlClass MissionControl[MISSION_COUNT];
extern const char* TutorialTextData;
extern uint16_t TutorialTextOffsets[225];
extern Buffer* TheaterBuffer;
extern GetCDClass CDList;
extern CCINIClass RuleINI;
extern CCINIClass AftermathINI;
extern Benchmark* Benches;
extern int MapTriggerID;
extern int LogicTriggerID;
extern PKey FastKey;
extern PKey SlowKey;
extern RulesClass Rule;
extern KeyboardClass* Keyboard;
extern RandomStraw CryptRandom;
extern RandomClass NonCriticalRandomNumber;
extern CarryoverClass* Carryover;
extern ScenarioClass Scen;
extern RemapControlType ColorRemaps[PCOLOR_COUNT];
extern RemapControlType MetalScheme;
extern RemapControlType GreyScheme;
extern VersionClass VerNum;
extern bool SlowPalette;
extern bool ScoresPresent;
extern bool AllowVoice;
extern NewConfigType NewConfig;
extern VoxType SpeakQueue;
extern bool PlayerWins;
extern bool PlayerLoses;
extern bool PlayerRestarts;
extern VoxType SpeechRecord[2];
extern void* SpeechBuffer[2];
extern int PreserveVQAScreen;
extern bool BreakoutAllowed;
extern bool Brokeout;

extern GameOptionsClass Options;

extern LogicClass Logic;
extern MapEditClass Map;
extern ScoreClass Score;
extern MonoClass MonoArray[DMONO_COUNT];
extern MFCD* TheaterData;
extern MFCD* MoviesMix;
extern MFCD* GeneralMix;
extern MFCD* ScoreMix;
extern MFCD* MainMix;
extern MFCD* ConquerMix;
extern ThemeClass Theme;
extern SpecialClass Special;

/*
**	Game object allocation and tracking classes.
*/
extern TFixedIHeapClass<AircraftClass> Aircraft;
extern TFixedIHeapClass<AnimClass> Anims;
extern TFixedIHeapClass<BuildingClass> Buildings;
extern TFixedIHeapClass<BulletClass> Bullets;
extern TFixedIHeapClass<FactoryClass> Factories;
extern TFixedIHeapClass<HouseClass> Houses;
extern TFixedIHeapClass<InfantryClass> Infantry;
extern TFixedIHeapClass<OverlayClass> Overlays;
extern TFixedIHeapClass<SmudgeClass> Smudges;
extern TFixedIHeapClass<TeamClass> Teams;
extern TFixedIHeapClass<TeamTypeClass> TeamTypes;
extern TFixedIHeapClass<TemplateClass> Templates;
extern TFixedIHeapClass<TerrainClass> Terrains;
extern TFixedIHeapClass<TriggerClass> Triggers;
extern TFixedIHeapClass<UnitClass> Units;
extern TFixedIHeapClass<VesselClass> Vessels;
extern TFixedIHeapClass<TriggerTypeClass> TriggerTypes;

extern TFixedIHeapClass<HouseTypeClass> HouseTypes;
extern TFixedIHeapClass<BuildingTypeClass> BuildingTypes;
extern TFixedIHeapClass<AircraftTypeClass> AircraftTypes;
extern TFixedIHeapClass<InfantryTypeClass> InfantryTypes;
extern TFixedIHeapClass<BulletTypeClass> BulletTypes;
extern TFixedIHeapClass<AnimTypeClass> AnimTypes;
extern TFixedIHeapClass<UnitTypeClass> UnitTypes;
extern TFixedIHeapClass<VesselTypeClass> VesselTypes;
extern TFixedIHeapClass<TemplateTypeClass> TemplateTypes;
extern TFixedIHeapClass<TerrainTypeClass> TerrainTypes;
extern TFixedIHeapClass<OverlayTypeClass> OverlayTypes;
extern TFixedIHeapClass<SmudgeTypeClass> SmudgeTypes;

extern FixedIHeapClass* HeapPointers[RTTI_COUNT];

extern TFixedIHeapClass<WeaponTypeClass> Weapons;
extern TFixedIHeapClass<WarheadTypeClass> Warheads;

extern QueueClass<EventClass, MAX_EVENTS> OutList;
extern QueueClass<EventClass, MAX_EVENTS * 64> DoList;

extern DynamicVectorClass<ObjectClass*> CurrentObject;
extern DynamicVectorClass<TriggerClass*> LogicTriggers;
extern DynamicVectorClass<TriggerClass*> MapTriggers;
extern DynamicVectorClass<TriggerClass*> HouseTriggers[HOUSE_COUNT];

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
extern fixed QuakeUnitDamage;
extern fixed QuakeBuildingDamage;
extern int QuakeInfantryDamage;
extern int QuakeDelay;
extern fixed ChronoTankDuration;  // chrono override for chrono tanks
extern fixed EngineerDamage;      // Amount of damage an engineer does
extern fixed
    EngineerCaptureLevel;  // Building damage level before engineer can capture

/*
**	Loaded data file pointers.
*/
extern const void* Metal12FontPtr;
extern const void* MapFontPtr;
extern const void* VCRFontPtr;
extern const void* TypeFontPtr;
extern const void* Font3Ptr;
extern const void* Font6Ptr;
extern const void* EditorFont;
extern const void* Font8Ptr;
extern const void* FontLEDPtr;
extern const void* ScoreFontPtr;
extern const void* GradFont6Ptr;
extern const char* SystemStrings;
extern const char* DebugStrings;

/*
**	Miscellaneous globals.
*/
extern ChronalVortexClass ChronalVortex;
extern TTimerClass<SystemTimerClass> TickCount;
extern bool PassedProximity;  // used in display.cpp
extern HousesType Whom;
extern _VQAConfig AnimControl;
extern long SpareTicks;
extern long PathCount;
extern long CellCount;
extern long TargetScan;
extern long SidebarRedraws;
extern DMonoType MonoPage;
extern bool SpecialFlag;
extern int ScenarioInit;
extern HouseClass* PlayerPtr;
extern PaletteClass CCPalette;
extern PaletteClass BlackPalette;
extern PaletteClass WhitePalette;
extern PaletteClass GamePalette;
// extern PaletteClass 				InGamePalette;
#define InGamePalette GamePalette
extern PaletteClass OriginalPalette;
extern PaletteClass ScorePalette;
extern int BuildLevel;
extern unsigned long ScenarioCRC;

extern bool bAftermathMultiplayer;  //	Is multiplayer game being played with
                                    // Aftermath rules?

extern bool bAutoSonarPulse;

extern CELL CurrentCell;

class SessionClass;
extern SessionClass Session;
class NullModemClass;
extern NullModemClass NullModem;
extern IPXManagerClass Ipx;

#if (TEN)
extern TenConnManClass* Ten;
#endif

#if (MPATH)
extern MPlayerManClass* MPath;
#endif

#if (TIMING_FIX)
extern int NewMaxAheadFrame1;
extern int NewMaxAheadFrame2;
#endif

extern GraphicViewPortClass HidPage;
extern int MenuList[][8];
extern CDTimerClass<SystemTimerClass> FrameTimer;
extern CDTimerClass<SystemTimerClass> CountDownTimer;

extern SpecialDialogType SpecialDialog;

extern int RequiredCD;
extern int CurrentCD;
extern int MouseInstalled;

extern int LogLevel;
extern unsigned long LogLevelTime[MAX_LOG_LEVEL];
extern unsigned long LogLastTime;

extern DynamicVectorClass<EventChoiceClass> test2;
extern DynamicVectorClass<ActionChoiceClass> test3;

extern bool LogDump_Print;

extern "C" {
extern bool IsTheaterShape;
}

extern void Reset_Theater_Shapes();
extern TheaterType LastTheater;

void Coordinate_Remap(GraphicViewPortClass* inbuffer, int x, int y, int width,
                      int height, unsigned char* remap_table);
void Do_Vortex(int x, int y, int frame);

extern bool ReadyToQuit;      // Are we about to exit cleanly
extern bool InDebugger;       // Are we being run from a debugger
void Memory_Error_Handler();  // Memory error handler function
void WWDebugString(const char* string);

/*************************************************************
** Internet specific externs
*/
extern char PlanetWestwoodHandle[];     // Planet WW user name
extern char PlanetWestwoodPassword[];   // Planet WW password
extern char PlanetWestwoodIPAddress[];  // IP of server or other player
extern long PlanetWestwoodPortNumber;   // Port number to send to
extern bool
    PlanetWestwoodIsHost;  // Flag true if player has control of game options
extern unsigned long PlanetWestwoodGameID;  // Game ID
#ifdef _WIN32
extern HWND WChatHWND;  // Handle to Wchat window.
#endif
extern bool GameStatisticsPacketSent;
extern bool ConnectionLost;
extern void* PacketLater;
extern bool SpawnedFromWChat;
extern int ShowCommand;
void Register_Game_Start_Time();
void Register_Game_End_Time();
void Send_Statistics_Packet();
void Check_From_WChat(char* wchat_name);
bool Do_The_Internet_Menu_Thang();
bool Server_Remote_Connect();
bool Client_Remote_Connect();
extern int UnitBuildPenalty;

/*
** From SENDFILE.CPP - externs for scenario file transfers
*/
bool Receive_Remote_File(char* file_name, unsigned int file_length,
                         unsigned int crc, int gametype);
bool Send_Remote_File(char* file_name, int gametype);
bool Get_Scenario_File_From_Host(char* return_name, size_t dest_size,
                                 int gametype);

bool Find_Local_Scenario(char* description, char* filename, unsigned int length,
                         char* digest, bool official);

#ifdef MPEGMOVIE  // Denzil 6/15/98

#ifdef MCIMPEG

#include "ra/mcimovie.h"
extern MCIMovie* MciMovie;

#endif

#include "ra/mpgset.h"
extern MPGSettings* MpgSettings;

#endif

#endif  // CNC_RED_ALERT_RA_EXTERNS_H_
