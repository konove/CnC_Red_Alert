#ifndef CNC_RED_ALERT_RA_STARTUP_H_
#define CNC_RED_ALERT_RA_STARTUP_H_

// Red Alert's startup and exit paths. main() itself lives in startup.cc.

// Runs Prog_End(), prints `message` and exits with status 1. Installed as
// Memory_Error_Exit while the game runs, when the game systems still have to
// be cleaned up.
[[noreturn]] void CleanUpAndExitWithError(char* message);

// Leaves the game from anywhere, cleaning up as a normal quit would: blanks
// the screen, runs Prog_End(), releases the video pages and exits with
// `exit_code`.
[[noreturn]] void EmergencyExit(int exit_code);

// Sets the video mode for ScreenWidth x ScreenHeight (falling back from 400
// to 480 lines), creates VisiblePage and HiddenPage, and attaches SeenBuff
// and HidPage to the 400-line game area. Leaves ScreenHeight at 400.
// Returns false, after stopping the tick timer, if no mode could be set.
bool InitVideo();

#endif  // CNC_RED_ALERT_RA_STARTUP_H_
