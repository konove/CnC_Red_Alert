// File: The -SCRIPT switch: the game played by a script file.

#ifndef CNC_RED_ALERT_TD_STARTUP_SCRIPTED_RUN_H_
#define CNC_RED_ALERT_TD_STARTUP_SCRIPTED_RUN_H_

#include <string>

namespace td {

// Starts playing the script file `path` into the game: its keys and clicks
// arrive as the player's, on the frames it names, and the game's clock moves
// a tick a frame instead of with real time, so the run is the same every
// time. The format is that of engine/window/player_script.h. A `shot <name>`
// step saves the screen as <name>.pcx in the script's directory, in the
// palette being shown; `quit` ends the game with exit code 0.
//
// Call once, when the window is open and before the first screen. Returns
// false, having said why on stderr, when the file cannot be read or holds a
// line that is no step. tools/ui_check.py runs scripts and compares their
// screenshots.
bool StartScriptedRun(const std::string& path);

}  // namespace td

#endif  // CNC_RED_ALERT_TD_STARTUP_SCRIPTED_RUN_H_
