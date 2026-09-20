// File: DebugState, the developer switches Red Alert changes while it runs.

#ifndef CNC_RED_ALERT_RA_DEBUG_STATE_H_
#define CNC_RED_ALERT_RA_DEBUG_STATE_H_

#include "base/installed.h"

// The developer and cheat switches, all of which start off. The command line
// turns some of them on before the game starts (see StartupOptions and
// Parse_Command_Line()), and the debug keys in debug.cc, the map editor and
// the mission code toggle them while it runs. Game owns the one DebugState;
// everything else reaches it through TheDebugState().
//
// Most reads sit inside `if constexpr (config::kCheatKeysEnabled)`, so a
// shipping build drops them and the switches stay off.
//
// Example:
//   if (TheDebugState().developer_mode()) {
//     DebugKeys(input);
//   }
class DebugState {
 public:
  DebugState() = default;
  ~DebugState() = default;

  DebugState(const DebugState&) = delete;
  DebugState& operator=(const DebugState&) = delete;
  DebugState(DebugState&&) = delete;
  DebugState& operator=(DebugState&&) = delete;

  // Whether the debug keys, the developer shortcuts and the extra reporting
  // are live. A cheat phrase on the command line turns it on.
  [[nodiscard]] bool developer_mode() const { return developer_mode_; }
  void set_developer_mode(bool on) { developer_mode_ = on; }

  // Whether the smaller set of playtester shortcuts is live. Every cheat
  // phrase turns this on too; -PLAYTEST turns on only this.
  [[nodiscard]] bool playtest() const { return playtest_; }
  void set_playtest(bool on) { playtest_ = on; }

  // Whether the scenario editor is running in place of the normal game.
  [[nodiscard]] bool map_editor_active() const { return map_editor_active_; }
  void set_map_editor_active(bool on) { map_editor_active_ = on; }

  // Whether the whole map is drawn regardless of the shroud. Saved with a
  // multiplayer game, so it stays in sync between the players.
  [[nodiscard]] bool unshroud() const { return unshroud_; }
  void set_unshroud(bool on) { unshroud_ = on; }

  // Whether sound and music are suppressed (-XQ). The audio device is still
  // open; nothing asks it to play.
  [[nodiscard]] bool quiet() const { return quiet_; }
  void set_quiet(bool on) { quiet_ = on; }

  // Whether every queued game event is logged, for tracking down a
  // multiplayer desync.
  [[nodiscard]] bool print_events() const { return print_events_; }
  void set_print_events(bool on) { print_events_ = on; }

  // Whether the map is validated once a frame (-CHECKMAP), which reports the
  // frame a corrupted cell list first appears in.
  [[nodiscard]] bool check_map() const { return check_map_; }
  void set_check_map(bool on) { check_map_ = on; }

  // Whether the player can build everything: tech level 98 and no
  // prerequisites.
  [[nodiscard]] bool build_anything() const { return build_anything_; }
  void set_build_anything(bool on) { build_anything_ = on; }

  // Whether each cell is drawn as its coordinates and movement zones instead
  // of its terrain.
  [[nodiscard]] bool show_cell_info() const { return show_cell_info_; }
  void set_show_cell_info(bool on) { show_cell_info_ = on; }

  // Whether the map editor tints each cell by how passable it is to infantry.
  [[nodiscard]] bool show_passability() const { return show_passability_; }
  void set_show_passability(bool on) { show_passability_ = on; }

  // Whether the next frame is dumped for a motion capture. RunFrame() clears
  // it again after the frame it applies to.
  [[nodiscard]] bool motion_capture() const { return motion_capture_; }
  void set_motion_capture(bool on) { motion_capture_ = on; }

 private:
  bool developer_mode_ = false;
  bool playtest_ = false;
  bool map_editor_active_ = false;
  bool unshroud_ = false;
  bool quiet_ = false;
  bool print_events_ = false;
  bool check_map_ = false;
  bool build_anything_ = false;
  bool show_cell_info_ = false;
  bool show_passability_ = false;
  bool motion_capture_ = false;
};

// Returns the DebugState that Game installed. CHECK-fails outside a Game's
// lifetime unless a test installed its own.
inline DebugState& TheDebugState() { return base::Installed<DebugState>::Get(); }

#endif  // CNC_RED_ALERT_RA_DEBUG_STATE_H_
