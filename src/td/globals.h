#ifndef CNC_RED_ALERT_TD_GLOBALS_H_
#define CNC_RED_ALERT_TD_GLOBALS_H_

#include <cstdint>
#include <string>

#include "td/special.h"

// Headless save/load checks; consumed by Select_Game and Main_Loop.
extern std::string DebugNewGame;
extern bool DebugFactoryTest;
extern bool DebugTeamTest;
extern bool DebugWorldTest;
extern bool DebugBuildingTest;
extern bool DebugMobileTest;
extern bool DebugMapTest;
extern bool DebugGlobalsTest;
extern int DebugLoadGame;
extern int DebugQuitAtFrame;
extern int DebugSaveSlot;
extern bool DebugNoMovies;
extern int64_t Frame;
//  True if we are currently in focus windows app
extern bool GameInFocus;
extern bool GameActive;
extern int32_t LParam;
extern SpecialClass Special;
extern bool InMovie;

#endif  // CNC_RED_ALERT_TD_GLOBALS_H_
