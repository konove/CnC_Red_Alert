#ifndef CNC_RED_ALERT_RA_STARTUP_H_
#define CNC_RED_ALERT_RA_STARTUP_H_

// Red Alert's startup and exit paths. main() itself lives in startup.cc.

// Runs Prog_End(), prints `string` and exits with status 1. Installed as
// Memory_Error_Exit while the game runs, and called directly by fatal paths
// that still have the game systems to clean up.
[[noreturn]] void Print_Error_End_Exit(char* string);

// Leaves the game from anywhere, cleaning up as a normal quit would: blanks
// the screen, runs Prog_End(), releases the video pages and exits with
// `code`.
[[noreturn]] void Emergency_Exit(int code);

// Sets the video mode for ScreenWidth x ScreenHeight (falling back from 400
// to 480 lines), creates VisiblePage and HiddenPage, and attaches SeenBuff
// and HidPage to the 400-line game area. Leaves ScreenHeight at 400.
// Returns false, after stopping the tick timer, if no mode could be set.
bool InitDDraw();

#endif  // CNC_RED_ALERT_RA_STARTUP_H_
