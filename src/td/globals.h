#ifndef CNC_RED_ALERT_TD_GLOBALS_H_
#define CNC_RED_ALERT_TD_GLOBALS_H_

#include <cstdint>
#include <string>

#include "td/special.h"

extern int64_t Frame;
//  True if we are currently in focus windows app
extern bool GameInFocus;
extern bool GameActive;
extern int32_t LParam;
extern SpecialClass Special;
extern bool InMovie;

#endif  // CNC_RED_ALERT_TD_GLOBALS_H_
