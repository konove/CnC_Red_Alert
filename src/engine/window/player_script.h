// File: LoadPlayerScript(), a ScriptedPlayer's steps read from text.

#ifndef CNC_RED_ALERT_ENGINE_WINDOW_PLAYER_SCRIPT_H_
#define CNC_RED_ALERT_ENGINE_WINDOW_PLAYER_SCRIPT_H_

#include <functional>
#include <string>
#include <string_view>

#include "engine/window/scripted_player.h"

namespace engine::window {

// What a script asks of the program running it, which a player cannot do
// with keys and clicks.
struct PlayerScriptHooks {
  // Saves a picture of the screen under `name`. Called for a `shot` step.
  std::function<void(std::string_view name)> take_shot;
  // Ends the program. Called for a `quit` step.
  std::function<void()> quit;
};

// Queues on `player` the steps of the script `text`, one per line:
//
//   at <frame> key <name>          press and release a key: ESC, RETURN, A,
//                                  F1, UP, ... (the KN_ names without KN_)
//   at <frame> keydown <name>      hold a key down: SHIFT, CTRL, ALT or any
//   at <frame> keyup <name>        other, until its keyup
//   at <frame> type <text>         type letters and digits
//   at <frame> click <x> <y>       click the left button there
//   at <frame> rclick <x> <y>      click the right button there
//   at <frame> press <x> <y>       hold the left button down there ...
//   at <frame> move <x> <y>        ... move the pointer ...
//   at <frame> release <x> <y>     ... and let go: a drag
//   at <frame> shot <name>         hooks.take_shot(name)
//   at <frame> quit                hooks.quit()
//
// Frames count the frames the Display ends, from 1, and must not go back.
// Blank lines and lines starting with '#' are skipped. Returns an empty
// string, or what is wrong with the first bad line, in which case the steps
// before it have been queued.
//
// Example:
//   const std::string error = LoadPlayerScript(
//       "at 30 key ESC\nat 40 shot menu\nat 41 quit\n", player, hooks);
std::string LoadPlayerScript(std::string_view text, ScriptedPlayer& player,
                             const PlayerScriptHooks& hooks);

}  // namespace engine::window

#endif  // CNC_RED_ALERT_ENGINE_WINDOW_PLAYER_SCRIPT_H_
