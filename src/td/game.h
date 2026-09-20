// File: Game, the owner of Tiberian Dawn's subsystems.

#ifndef CNC_RED_ALERT_TD_GAME_H_
#define CNC_RED_ALERT_TD_GAME_H_

#include <utility>

#include "base/installed.h"
#include "td/assets.h"
#include "td/debug_state.h"
#include "td/game_clock.h"
#include "td/object_heaps.h"
#include "td/palettes.h"
#include "td/screen.h"
#include "td/startup_options.h"
#include "td/world.h"

// Owns the game's subsystems and so fixes the order they are built and torn
// down in. Members are declared in dependency order, which C++ constructs
// front to back and destroys back to front; each one is installed with
// base::Installed as soon as it is built. docs/GLOBALS_PLAN.md moves the
// globals into it one subsystem at a time.
//
// main() creates the one Game on the heap before anything else, and
// ShutDown() destroys it, so every way out of the game tears the subsystems
// down in the same order. Constructing a Game does no I/O and needs neither a
// window nor the game data, so a test can build one.
//
// Example:
//   auto* game = new Game();
//   ...
//   delete game;  // In ShutDown().
class Game {
 public:
  Game() = default;
  ~Game() = default;

  Game(const Game&) = delete;
  Game& operator=(const Game&) = delete;
  Game(Game&&) = delete;
  Game& operator=(Game&&) = delete;

  // Installs what the command line asked for. main() calls this once, as
  // soon as it has parsed the arguments and before anything reads them.
  void set_startup_options(StartupOptions options) {
    startup_options_ = std::move(options);
  }

 private:
  StartupOptions startup_options_;
  base::Installed<StartupOptions>::Scope startup_options_scope_{
      startup_options_};
  GameClock game_clock_;
  base::Installed<GameClock>::Scope game_clock_scope_{game_clock_};
  Screen screen_;
  base::Installed<Screen>::Scope screen_scope_{screen_};
  Palettes palettes_;
  base::Installed<Palettes>::Scope palettes_scope_{palettes_};
  Assets assets_;
  base::Installed<Assets>::Scope assets_scope_{assets_};
  ObjectHeaps object_heaps_;
  base::Installed<ObjectHeaps>::Scope object_heaps_scope_{object_heaps_};
  World world_;
  base::Installed<World>::Scope world_scope_{world_};
  DebugState debug_state_;
  base::Installed<DebugState>::Scope debug_state_scope_{debug_state_};
};

#endif  // CNC_RED_ALERT_TD_GAME_H_
