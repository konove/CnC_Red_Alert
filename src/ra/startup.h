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

// Releases everything the game set up - Prog_End(), the video pages and the
// Game - so the caller only has to exit. Every way out of the game goes through
// it: every return from main(), EmergencyExit(), the memory-error exits and the
// SDL quit handler. It does not draw, so it is safe before the video pages
// exist.
void ShutDown();

// Sets the video mode for ScreenWidth x ScreenHeight (falling back from 400
// to 480 lines), creates visible_page and hidden_page, and attaches
// visible_view and hidden_view to the 400-line game area. Leaves ScreenHeight
// at 400. Returns false if no mode could be set.
bool InitVideo();

#endif  // CNC_RED_ALERT_RA_STARTUP_H_
