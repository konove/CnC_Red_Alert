// File: Game, the owner of Red Alert's subsystems.

#ifndef CNC_RED_ALERT_RA_STARTUP_GAME_H_
#define CNC_RED_ALERT_RA_STARTUP_GAME_H_

#include <memory>
#include <utility>

#include "base/installed.h"
#include "engine/audio/audio_mixer.h"
#include "engine/window/display.h"
#include "game/core/game_clock.h"
#include "game/ui/gadget.h"
#include "game/ui/input.h"
#include "ra/assets/assets.h"
#include "ra/assets/palettes.h"
#include "ra/assets/screen.h"
#include "ra/assets/theme.h"
#include "ra/core/object_heaps.h"
#include "ra/core/special_flags.h"
#include "ra/core/type_heaps.h"
#include "ra/data/rules.h"
#include "ra/debug/debug_state.h"
#include "ra/map/world.h"
#include "ra/net/network.h"
#include "ra/net/session.h"
#include "ra/screens/goptions.h"
#include "ra/startup/game_state.h"
#include "ra/startup/startup_options.h"
#include "ra/startup/version.h"
#include "ra/startup/winstub.h"
#include "ra/ui/screenshot.h"

namespace ra {

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
  // `display_kind` says what the game shows its picture on: its window, by
  // default; main() asks for the scripted window when a script plays the
  // game, and a test of a dialog for no window at all.
  explicit Game(const engine::window::DisplayKind display_kind =
                    engine::window::DisplayKind::kWindow)
      : display_(engine::window::MakeDisplay(display_kind, window_events_)) {
    game::Gadget::SetKeyHook(&Screenshot_Key);
  }
  // Stops the IPX manager listening first: that goes through TheNetwork(),
  // which no longer resolves by the time network_ is destroyed.
  ~Game() { network_.ipx().Stop_Listening(); }

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
  game::GameClock game_clock_;
  base::Installed<game::GameClock>::Scope game_clock_scope_{game_clock_};
  // Before the window's events, which feed its keyboard and mouse. The cursor
  // is a square large enough for every mouse shape the game draws.
  game::Input input_{48, 48};
  base::Installed<game::Input>::Scope input_scope_{input_};
  // Before the Display, which hands it the window's events.
  GameWindowEvents window_events_{input_};
  // Before Screen, which attaches its visible page to the window.
  std::unique_ptr<engine::window::Display> display_;
  base::Installed<engine::window::Display>::Scope display_scope_{*display_};
  Screen screen_;
  base::Installed<Screen>::Scope screen_scope_{screen_};
  Palettes palettes_;
  base::Installed<Palettes>::Scope palettes_scope_{palettes_};
  Assets assets_;
  base::Installed<Assets>::Scope assets_scope_{assets_};
  TypeHeaps type_heaps_;
  base::Installed<TypeHeaps>::Scope type_heaps_scope_{type_heaps_};
  ObjectHeaps object_heaps_;
  base::Installed<ObjectHeaps>::Scope object_heaps_scope_{object_heaps_};
  RulesClass rules_;
  base::Installed<RulesClass>::Scope rules_scope_{rules_};
  World world_;
  base::Installed<World>::Scope world_scope_{world_};
  VersionClass version_;
  base::Installed<VersionClass>::Scope version_scope_{version_};
  GameState game_state_;
  base::Installed<GameState>::Scope game_state_scope_{game_state_};
  GameOptionsClass options_;
  base::Installed<GameOptionsClass>::Scope options_scope_{options_};
  SpecialFlags special_ = InitialSpecialFlags();
  base::Installed<SpecialFlags>::Scope special_scope_{special_};
  // The mixer comes before the music player, which plays through it.
  engine::audio::AudioMixer audio_;
  base::Installed<engine::audio::AudioMixer>::Scope audio_scope_{audio_};
  ThemeClass theme_;
  base::Installed<ThemeClass>::Scope theme_scope_{theme_};
  SessionClass session_;
  base::Installed<SessionClass>::Scope session_scope_{session_};
  Network network_;
  base::Installed<Network>::Scope network_scope_{network_};
  DebugState debug_state_;
  base::Installed<DebugState>::Scope debug_state_scope_{debug_state_};
};

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_STARTUP_GAME_H_
