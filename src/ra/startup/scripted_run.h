// File: The -SCRIPT switch: the game played by a script file.

#ifndef CNC_RED_ALERT_RA_STARTUP_SCRIPTED_RUN_H_
#define CNC_RED_ALERT_RA_STARTUP_SCRIPTED_RUN_H_

#include <string>

namespace ra {

// Starts playing the script file `path` into the game: its keys and clicks
// arrive as the player's, on the frames it names, and the game's clock moves
// a tick a frame instead of with real time, so the run is the same every
// time. The format is that of engine/window/player_script.h. A `shot <name>`
// step saves what the window shows -- a movie frame or backdrop included --
// as <name>.pcx in the script's directory, in the palette being shown; `quit`
// ends the game with exit code 0.
//
// Call once, when the window is open and before the first screen. Returns
// false, having said why on stderr, when the file cannot be read or holds a
// line that is no step. tools/ui_check.py runs scripts and compares their
// screenshots.
//
// With `real_time` the game keeps the machine's clock and only the keys and
// clicks are the script's: for a network game against another instance,
// where each side waits for the other in real time. What frame a step lands
// on then varies, so such a script leaves wide gaps and its screenshots are
// not compared.
bool StartScriptedRun(const std::string& path, bool real_time = false);

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_STARTUP_SCRIPTED_RUN_H_
