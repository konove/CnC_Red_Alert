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
#include "sdllib/playcd.h"
#include "tech/bench.h"
#include "tech/buff.h"
#include "tech/game_file.h"
#include "tech/mix_archive.h"

// Scratch space for packing and unpacking the MapPack and OverlayPack INI blocks.
inline char staging_buffer[32000];


extern bool IsVQ640;
extern uint32_t GameVersion;

// Developer switches (-NEWGAME<scenario>, -LOADGAME<n>, -QUITFRAME<n>,
// -SAVESLOT<n>) for save-game checks without a display; see init.cc and
// conquer.cc. -1 or empty means unset.
extern std::string DebugNewGame;
extern int DebugLoadGame;
extern int64_t DebugQuitAtFrame;
extern int DebugSaveSlot;
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

extern std::span<const std::byte> LightningShapes;

extern int NewINIFormat;

extern bool AntsEnabled;

extern bool NewUnitsEnabled;
extern bool SecretUnitsEnabled;
extern int MTankDistance;
extern int CarrierLaunchDelay;

extern const char* NameOverride[25];
extern int NameIDOverride[25];

extern bool GameInFocus;
// One interpolation table per palette a movie can use; VQAs in this game never
// come close to the limit.
inline std::vector<unsigned char> InterpolatedPalettes[100];
inline bool PalettesRead = false;
inline int PaletteCounter = 0;
extern int AllDone;
extern bool InMovie;
extern WWMouseClass* WWMouse;
extern GraphicBufferClass HiddenPage;
extern GraphicBufferClass VisiblePage;
extern GraphicBufferClass SysMemPage;
extern int ScreenWidth;
extern int ScreenHeight;
extern GraphicBufferClass ModeXBuff;
extern GraphicBufferClass VQ640;  // 640x400 staging page for hi-res movies

/*
**	Dynamic global variables (these change or are initialized at run time).
*/
extern base::EnumArray<MissionType, MissionControlClass> MissionControl;
extern std::vector<char> TutorialTextData;
extern uint16_t TutorialTextOffsets[225];
extern Buffer* TheaterBuffer;
extern GetCDClass CDList;
extern CCINIClass RuleINI;
extern CCINIClass AftermathINI;
extern std::vector<Benchmark> Benches;
extern int MapTriggerID;
extern int LogicTriggerID;
extern PKey FastKey;
extern PKey SlowKey;
extern RulesClass Rule;
extern KeyboardClass* Keyboard;
extern RandomClass local_rng;
extern std::vector<CarryoverClass> Carryover;
extern ScenarioClass Scen;
extern base::EnumArray<PlayerColorType, RemapControlType> ColorRemaps;
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
extern std::vector<std::byte> SpeechBuffer[2];
extern int PreserveVQAScreen;
extern bool BreakoutAllowed;
extern bool Brokeout;

extern GameOptionsClass Options;

extern LogicClass Logic;
extern MapEditClass Map;
extern ScoreClass Score;
extern base::EnumArray<DMonoType, MonoClass> MonoArray;
extern MixArchive* TheaterData;
extern MixArchive* MoviesMix;
extern MixArchive* GeneralMix;
extern MixArchive* ScoreMix;
extern MixArchive* MainMix;
extern MixArchive* ConquerMix;
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

extern base::EnumArray<RTTIType, FixedIHeapClass*> HeapPointers;

extern TFixedIHeapClass<WeaponTypeClass> Weapons;
extern TFixedIHeapClass<WarheadTypeClass> Warheads;

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
extern std::span<const std::byte> Metal12FontPtr;
extern std::span<const std::byte> MapFontPtr;
extern std::span<const std::byte> VCRFontPtr;
extern std::span<const std::byte> TypeFontPtr;
extern std::span<const std::byte> Font3Ptr;
extern std::span<const std::byte> Font6Ptr;
extern std::span<const std::byte> EditorFont;
extern std::span<const std::byte> Font8Ptr;
extern std::span<const std::byte> FontLEDPtr;
extern std::span<const std::byte> ScoreFontPtr;
extern std::span<const std::byte> GradFont6Ptr;
// Tutorial prompts, dialog text, and other UI strings loaded from the mix file.
// Accessed via Text_String() for indices 0–999.
extern std::span<const std::byte> SystemStrings;

// Debug/developer strings loaded from DEBUG.ENG. Accessed via Text_String()
// for indices >= 1000 (offset by 1000 into this table).
extern std::span<const std::byte> DebugStrings;

/*
**	Miscellaneous globals.
*/
extern ChronalVortexClass ChronalVortex;
extern Stopwatch<SystemTickSource> TickCount;
extern bool PassedProximity;  // used in display.cpp
extern HousesType Whom;
extern VQAConfig AnimControl;
extern int64_t SpareTicks;
extern int32_t PathCount;
extern int32_t CellCount;
extern int32_t TargetScan;
extern int32_t SidebarRedraws;
extern DMonoType MonoPage;
extern bool SpecialFlag;
extern int ScenarioInit;
extern HouseClass* PlayerPtr;
extern PaletteClass CCPalette;
extern PaletteClass BlackPalette;
extern PaletteClass WhitePalette;
extern PaletteClass GamePalette;
extern PaletteClass OriginalPalette;
extern PaletteClass ScorePalette;
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

extern GraphicViewPortClass HidPage;
extern int MenuList[1][8];
extern Timer<SystemTickSource> FrameTimer;
extern Timer<SystemTickSource> CountDownTimer;

extern SpecialDialogType SpecialDialog;

extern int RequiredCD;
extern int CurrentCD;
extern bool MouseInstalled;

extern int LogLevel;
extern int64_t LogLevelTime[kMaxLogLevel];
extern int64_t LogLastTime;

extern DynamicVectorClass<EventChoiceClass> test2;
extern DynamicVectorClass<ActionChoiceClass> test3;

extern bool LogDump_Print;


extern TheaterType LastTheater;

void Do_Vortex(int x, int y, int frame);

// Shutdown state: 0 = running, 1 = clean shutdown, 2 = complete, 3 = emergency.
extern int ReadyToQuit;
extern bool InDebugger;       // Are we being run from a debugger
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
extern int UnitBuildPenalty;

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
