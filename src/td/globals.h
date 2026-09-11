#ifndef CNC_RED_ALERT_TD_GLOBALS_H_
#define CNC_RED_ALERT_TD_GLOBALS_H_

#include <cstdint>
#include <string>

#include "sdllib/gbuffer.h"
#include "td/special.h"

// Headless save/load checks; consumed by Select_Game and Main_Loop.
extern std::string DebugNewGame;
extern bool DebugFactoryTest;
extern bool DebugTeamTest;
extern bool DebugWorldTest;
extern bool DebugBuildingTest;
extern int DebugLoadGame;
extern int DebugQuitAtFrame;
extern int DebugSaveSlot;
extern bool DebugNoMovies;
extern int64_t Frame;
//  True if we are currently in focus windows app
extern bool GameInFocus;
extern int ScreenWidth;
extern int ScreenHeight;
extern bool GameActive;
extern long LParam;
extern GraphicViewPortClass SeenBuff;
extern SpecialClass Special;

#endif  // CNC_RED_ALERT_TD_GLOBALS_H_
