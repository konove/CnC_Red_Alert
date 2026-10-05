// File: ScriptedRun, what plays a script file into the whole game.

#ifndef CNC_RED_ALERT_ENGINE_WINDOW_SCRIPTED_RUN_H_
#define CNC_RED_ALERT_ENGINE_WINDOW_SCRIPTED_RUN_H_

#include <memory>
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
// same thing every time and as fast as the machine allows. The game pairs it
// with a ScriptedWindowDisplay, which keeps no time of its own either.
//
// A run with Pace::kRealTime has no clock: the game keeps the machine's time
// and its ordinary window, and only the keys and clicks are the script's. Two
// instances playing each other over the network need that, since each waits
// for the other in real time.
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
  // How the run keeps time.
  enum class Pace {
    kFrames,    // a tick a frame, on a clock of its own: repeatable
    kRealTime,  // the machine's clock: for two instances on a network
  };

  // The player keeps them, behind a pointer the analysis does not follow.
  // NOLINTBEGIN(clang-diagnostic-lifetime-safety-lifetimebound-violation)
  ScriptedRun(Display& display ABSL_ATTRIBUTE_LIFETIME_BOUND,
              KeyBuffer& keyboard ABSL_ATTRIBUTE_LIFETIME_BOUND,
              HardwareCursor& mouse ABSL_ATTRIBUTE_LIFETIME_BOUND,
              Pace pace = Pace::kFrames);
  // NOLINTEND(clang-diagnostic-lifetime-safety-lifetimebound-violation)

  // Queues the steps of the script `text` (player_script.h). Returns an
  // empty string, or what is wrong with it.
  std::string Load(std::string_view text, const PlayerScriptHooks& hooks);

 private:
  // How many polls without a frame end one.
  static constexpr int kIdlePumpsPerFrame = 4;

  // nullptr in a real-time run.
  std::unique_ptr<engine::platform::ManualClock> clock_;
  // Built in the constructor, once it is known whether there is a clock;
  // never null after it.
  std::unique_ptr<ScriptedPlayer> player_;
};

}  // namespace engine::window

#endif  // CNC_RED_ALERT_ENGINE_WINDOW_SCRIPTED_RUN_H_
