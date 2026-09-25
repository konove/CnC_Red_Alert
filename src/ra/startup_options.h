// File: StartupOptions, what Red Alert's command line asks for.

#ifndef CNC_RED_ALERT_RA_STARTUP_OPTIONS_H_
#define CNC_RED_ALERT_RA_STARTUP_OPTIONS_H_

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "engine/base/installed.h"
#include "ra/ipxaddr.h"

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

  // The debug switches a cheat phrase, -PLAYTEST, -CHECKMAP, -CHECKHEAPS or
  // -X turns on.
  // See DebugState, which owns them once the game is running.
  bool developer_mode = false;
  bool playtest = false;
  bool map_editor_active = false;
  bool unshroud = false;
  bool quiet = false;         // -XQ
  bool print_events = false;  // -XP
  bool check_map = false;     // -CHECKMAP
  bool check_heaps = false;   // -CHECKHEAPS

  // -XI and -XH: SpecialClass flags that the options dialog can change later.
  bool inert_weapons = false;
  bool speed_build = false;

  // -NOFADE, and every -QUITFRAME run: nobody watches an automated run, and
  // its palette fades only add wall time.
  bool disable_fades = false;

  // -NOMOVIES and -NOMOUSEGRAB.
  bool no_movies = false;
  bool no_mouse_grab = false;

  // Network options that the multiplayer dialogs can change later.
  bool net_stealth = false;       // -STEALTH: hide the player names
  bool outside_messages = false;  // -MESSAGES: accept messages from outside
  bool attract = false;           // -ATTRACT: allow attract mode
  bool record = false;            // -XX: record a multiplayer game
  bool play = false;              // -XY: play a recording back

  // -SOCKET<offset>: the IPX socket to use, already offset from 0x4000.
  // Absent unless the offset was in range.
  std::optional<uint16_t> socket;

  // -DESTNET<address>: a network on the far side of an IPX bridge. Absent
  // unless the whole address parsed.
  std::optional<IPXAddressClass> bridge_net;

  // Developer switches for save-game checks without a display; see
  // Select_Game() and RunFrame(). -1 or empty means the switch was not given.
  std::string new_game;        // -NEWGAME<scenario>
  int load_game = -1;          // -LOADGAME<slot>
  int64_t quit_at_frame = -1;  // -QUITFRAME<frame>
  int save_slot = -1;          // -SAVESLOT<slot>
  uint16_t custom_seed = 0;    // -SEED<n>, 0 for a random seed
};

// Returns the options Game was given. CHECK-fails outside a Game's lifetime
// unless a test installed its own.
inline const StartupOptions& TheStartupOptions() {
  return base::Installed<StartupOptions>::Get();
}

#endif  // CNC_RED_ALERT_RA_STARTUP_OPTIONS_H_
