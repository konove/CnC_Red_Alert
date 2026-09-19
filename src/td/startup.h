#ifndef CNC_RED_ALERT_TD_STARTUP_H_
#define CNC_RED_ALERT_TD_STARTUP_H_

// Tiberian Dawn's shared exit path. main() itself lives in startup.cc.

// Releases everything the game set up - Prog_End(), the video pages and the
// Game - so the caller only has to exit. Every way out of the game goes through
// it: every return from main(), the fatal-error and missing-CD exits, the
// memory-error exits and the SDL quit handler. It does not draw, so it is safe
// before the video pages exist.
void ShutDown();

#endif  // CNC_RED_ALERT_TD_STARTUP_H_
