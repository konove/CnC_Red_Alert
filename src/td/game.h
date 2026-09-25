// File: Game, the owner of Tiberian Dawn's subsystems.

#ifndef CNC_RED_ALERT_TD_GAME_H_
#define CNC_RED_ALERT_TD_GAME_H_

#include <utility>

#include "engine/base/installed.h"
#include "sdllib/display.h"
#include "td/assets.h"
#include "td/debug_state.h"
#include "td/game_clock.h"
#include "td/game_state.h"
#include "td/goptions.h"
#include "td/input.h"
#include "td/network.h"
#include "td/object_heaps.h"
#include "td/palettes.h"
#include "td/screen.h"
#include "td/session.h"
#include "td/special.h"
#include "td/startup_options.h"
#include "td/theme.h"
#include "td/world.h"
#include "tech/audio_mixer.h"

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
  // Before Screen, which attaches its visible page to the window.
  Display display_;
  base::Installed<Display>::Scope display_scope_{display_};
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
  GameState game_state_;
  base::Installed<GameState>::Scope game_state_scope_{game_state_};
  Input input_;
  base::Installed<Input>::Scope input_scope_{input_};
  GameOptionsClass options_;
  base::Installed<GameOptionsClass>::Scope options_scope_{options_};
  SpecialClass special_{};
  base::Installed<SpecialClass>::Scope special_scope_{special_};
  // The mixer comes before the music player, which plays through it.
  AudioMixer audio_;
  base::Installed<AudioMixer>::Scope audio_scope_{audio_};
  ThemeClass theme_;
  base::Installed<ThemeClass>::Scope theme_scope_{theme_};
  SessionClass session_;
  base::Installed<SessionClass>::Scope session_scope_{session_};
  Network network_;
  base::Installed<Network>::Scope network_scope_{network_};
  DebugState debug_state_;
  base::Installed<DebugState>::Scope debug_state_scope_{debug_state_};
};

#endif  // CNC_RED_ALERT_TD_GAME_H_
