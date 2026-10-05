// File: ScriptedRun, what plays a script file into the whole game.

#ifndef CNC_RED_ALERT_ENGINE_WINDOW_SCRIPTED_RUN_H_
#define CNC_RED_ALERT_ENGINE_WINDOW_SCRIPTED_RUN_H_

#include <string>
#include <string_view>

#include "absl/base/attributes.h"
#include "engine/platform/manual_clock.h"
#include "engine/window/display.h"
#include "engine/window/hardware_cursor.h"
#include "engine/window/keyboard.h"
#include "engine/window/player_script.h"
#include "engine/window/scripted_player.h"

namespace engine::window {

// A scripted player and the clock it moves, for a game started with a script
// to play instead of a person: the games' -SCRIPT switch. While one exists
// the game reads its time from the clock, a tick a frame, so a run does the
// same thing every time and as fast as the machine allows, and the Display
// stops keeping time of its own.
//
// Unlike a test's player it ends frames for loops that only poll the
// keyboard, since the whole game has such loops and a script has to get
// through them.
//
// The Display and the devices must outlive it. A game that ends from inside
// a script step (`quit`) must not destroy it on the way out: the step that
// is running belongs to it.
//
// Example:
//   auto run = std::make_unique<ScriptedRun>(TheDisplay(), TheKeyboard(),
//                                            TheMouse());
//   const std::string error = run->Load(script_text, hooks);
class ScriptedRun {
 public:
  ScriptedRun(Display& display ABSL_ATTRIBUTE_LIFETIME_BOUND,
              KeyBuffer& keyboard ABSL_ATTRIBUTE_LIFETIME_BOUND,
              HardwareCursor& mouse ABSL_ATTRIBUTE_LIFETIME_BOUND);

  // Queues the steps of the script `text` (player_script.h). Returns an
  // empty string, or what is wrong with it.
  std::string Load(std::string_view text, const PlayerScriptHooks& hooks);

 private:
  // How many polls without a frame end one.
  static constexpr int kIdlePumpsPerFrame = 4;

  engine::platform::ManualClock clock_;
  ScriptedPlayer player_;
};

}  // namespace engine::window

#endif  // CNC_RED_ALERT_ENGINE_WINDOW_SCRIPTED_RUN_H_
