// File: StartupOptions, what Tiberian Dawn's command line asks for.

#ifndef CNC_RED_ALERT_TD_STARTUP_OPTIONS_H_
#define CNC_RED_ALERT_TD_STARTUP_OPTIONS_H_

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "engine/base/installed.h"
#include "td/ipxaddr.h"

// Everything Parse_Command_Line() reads out of the command line, so that the
// parser writes no game state of its own: it fills a StartupOptions, and
// main() hands each value to whatever owns it. Some of them seed state the
// game goes on to change (the screen mode, the debug switches, the network
// options); the request itself never changes, so TheStartupOptions() hands
// out a const reference.
//
// Example:
//   if (TheStartupOptions().quit_at_frame >= 0) { ... }
struct StartupOptions {
  // -CD<path>: extra directories to search for game data, in the order given.
  // The paths keep the case they were typed in.
  std::vector<std::string> search_paths;

  // -480: ask for a 640x480 window rather than 640x400. The config file's
  // Resolution entry asks for the same thing.
  bool tall_screen = false;

  // -INSTALL: the installer runs the game once to play the intro.
  bool from_install = false;

  // The debug switches a cheat phrase, -PLAYTEST, -CHECKMAP or -X turns on.
  // See DebugState, which owns them once the game is running.
  bool developer_mode = false;
  bool playtest = false;
  bool map_editor_active = false;
  bool unshroud = false;
  bool quiet = false;      // -XQ
  bool check_map = false;  // -CHECKMAP

  // SpecialClass flags that the options dialog can change later: the two
  // difficulty phrases, the bonus scenarios, and the -X switches.
  bool easy = false;
  bool hard = false;
  bool jurassic = false;
  bool inert_weapons = false;   // -XI
  bool speed_build = false;     // -XH
  bool visible_target = false;  // -XV

  // Recording a multiplayer game: -XX records, -XY plays back, and -XS
  // flushes the file every frame so a crash keeps what was recorded.
  bool record = false;
  bool playback = false;
  bool super_record = false;

  // -O or -0: keep the network protocol compatible with version 1.07. The
  // config file's Compatibility entry asks for the same thing.
  bool compatibility_v107 = false;

  // -ENGLISH (Japanese build only): use an English keyboard layout.
  bool force_english = false;

  // Network options that the multiplayer dialogs can change later.
  bool net_stealth = false;       // -STEALTH: hide the player names
  bool outside_messages = false;  // -MESSAGES: accept outside messages
  bool attract = false;           // -ATTRACT: allow attract mode
  bool solo_net_play = false;     // -HANSOLO: one player in a network game

  // -SOCKET<offset>: the IPX socket to use, already offset from 0x4000.
  // Absent unless the offset was in range.
  std::optional<uint16_t> socket;

  // -DESTNET<address>: a network on the far side of an IPX bridge. Absent
  // unless the whole address parsed.
  std::optional<IPXAddressClass> bridge_net;

  // -WCHAT: Westwood Chat started the game, so go straight to the internet
  // menu rather than the intro.
  bool spawned_from_wchat = false;

  // -MMX: use the MMX instructions in the blitters.
  bool mmx_available = false;

  // -NOMOUSEGRAB and -NOMOVIES.
  bool no_mouse_grab = false;
  bool no_movies = false;

  // Developer switches for save-game checks without a display; see
  // Select_Game() and Main_Loop(). -1 or empty means the switch was not
  // given.
  std::string new_game;      // -NEWGAME<scenario>
  int load_game = -1;        // -LOADGAME<slot>
  int quit_at_frame = -1;    // -QUITFRAME<frame>
  int save_slot = -1;        // -SAVESLOT<slot>
  uint16_t custom_seed = 0;  // -SEED<n>, 0 for a random seed

  // Switches that fill one part of the game with known values before saving,
  // so a save/load check covers it: -GLOBALTEST, -MAPTEST and the rest.
  bool globals_test = false;
  bool map_test = false;
  bool mobile_test = false;
  bool building_test = false;
  bool world_test = false;
  bool team_test = false;
  bool factory_test = false;
};

// Returns the options Game was given. CHECK-fails outside a Game's lifetime
// unless a test installed its own.
inline const StartupOptions& TheStartupOptions() {
  return base::Installed<StartupOptions>::Get();
}

#endif  // CNC_RED_ALERT_TD_STARTUP_OPTIONS_H_
