#ifndef CNC_RED_ALERT_RA_STARTUP_H_
#define CNC_RED_ALERT_RA_STARTUP_H_

// Red Alert's startup and exit paths. main() itself lives in startup.cc.

// Leaves the game from anywhere: blanks the screen, runs ShutDown() and exits
// with `exit_code`.
[[noreturn]] void EmergencyExit(int exit_code);

// Cleans up the engine systems the game started (audio, mouse)
// before the process exits. ShutDown() calls it.
void ShutDownEngine();

// Releases everything the game set up - ShutDownEngine() and the Game, which
// owns the video pages - so the caller only has to exit. Every way out of the
// game goes through it: every return from main(), EmergencyExit() and the SDL
// quit handler. It does not draw, so it is safe before the video pages exist.
void ShutDown();

#endif  // CNC_RED_ALERT_RA_STARTUP_H_
