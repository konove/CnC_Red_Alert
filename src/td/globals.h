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
extern int ScreenWidth;
extern int ScreenHeight;
extern bool GameActive;
extern long LParam;
extern GraphicViewPortClass SeenBuff;
extern SpecialClass Special;

// MFCD is an alias in externs.h and cannot be forward-declared; name the type
// it stands for.
// The virtual-destructor exemption on MixFileClass's definition does not reach
// this forward declaration.
template <class T>
// NOLINTNEXTLINE(cppcoreguidelines-virtual-class-destructor)
class MixFileClass;
class CCFileClass;
extern MixFileClass<CCFileClass>* TheaterIcons;
extern bool InMovie;

#endif  // CNC_RED_ALERT_TD_GLOBALS_H_
