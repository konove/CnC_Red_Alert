#ifndef CNC_RED_ALERT_RA_STARTUP_H_
#define CNC_RED_ALERT_RA_STARTUP_H_

// Red Alert's startup and exit paths. main() itself lives in startup.cc.

// Runs ShutDown(), prints `message` and exits with status 1. Installed as
// Memory_Error_Exit while the game runs, when the game systems still have to
// be cleaned up.
[[noreturn]] void CleanUpAndExitWithError(char* message);

// Leaves the game from anywhere: blanks the screen, runs ShutDown() and exits
// with `exit_code`.
[[noreturn]] void EmergencyExit(int exit_code);

// Releases everything the game set up - Prog_End() and the Game, which owns
// the video pages - so the caller only has to exit. Every way out of the game
// goes through it: every return from main(), EmergencyExit(), the memory-error
// exits and the SDL quit handler. It does not draw, so it is safe before the
// video pages exist.
void ShutDown();

#endif  // CNC_RED_ALERT_RA_STARTUP_H_
