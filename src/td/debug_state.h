// File: DebugState, the developer switches Tiberian Dawn changes while it runs.

#ifndef CNC_RED_ALERT_TD_DEBUG_STATE_H_
#define CNC_RED_ALERT_TD_DEBUG_STATE_H_

#include <cstdint>

#include "absl/base/attributes.h"
#include "engine/base/installed.h"
#include "td/defines.h"

// The developer and cheat switches, all of which start off. The command line
// turns some of them on before the game starts (see Parse_Command_Line()),
// and the debug keys in debug.cc, the map editor and the mission code toggle
// them while it runs. Game owns the one DebugState; everything else reaches
// it through TheDebugState().
//
// Most reads sit inside `if constexpr (config::kCheatKeysEnabled)`, so a
// shipping build drops them and the switches stay off.
//
// Example:
//   if (TheDebugState().developer_mode()) {
//     Debug_Key(input);
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

  // Whether the whole map is drawn regardless of the shroud.
  [[nodiscard]] bool unshroud() const { return unshroud_; }
  void set_unshroud(bool on) { unshroud_ = on; }

  // Whether sound and music are suppressed (-XQ). The audio device is still
  // open; nothing asks it to play.
  [[nodiscard]] bool quiet() const { return quiet_; }
  void set_quiet(bool on) { quiet_ = on; }

  // Whether the map is validated once a frame (-CHECKMAP), which reports the
  // frame a corrupted cell list first appears in.
  [[nodiscard]] bool check_map() const { return check_map_; }
  void set_check_map(bool on) { check_map_ = on; }

  // Whether the player can build everything: tech level 98 and no
  // prerequisites.
  [[nodiscard]] bool build_anything() const { return build_anything_; }
  void set_build_anything(bool on) { build_anything_ = on; }

  // Whether anything the player builds finishes at once.
  [[nodiscard]] bool instant_build() const { return instant_build_; }
  void set_instant_build(bool on) { instant_build_ = on; }

  // Whether each cell is drawn as its number, overlay and occupancy instead
  // of its terrain.
  [[nodiscard]] bool show_cell_info() const { return show_cell_info_; }
  void set_show_cell_info(bool on) { show_cell_info_ = on; }

  // Whether the map editor tints each cell by how passable it is to infantry.
  [[nodiscard]] bool show_passability() const { return show_passability_; }
  void set_show_passability(bool on) { show_passability_ = on; }

  // Whether the map is redrawn whenever a region's threat rating changes, so
  // the AI's view of the battlefield can be watched.
  [[nodiscard]] bool show_threat() const { return show_threat_; }
  void set_show_threat(bool on) { show_threat_ = on; }

  // Whether the path finder draws each step of its search as it runs.
  [[nodiscard]] bool trace_path_search() const { return trace_path_search_; }
  void set_trace_path_search(bool on) { trace_path_search_ = on; }

  // Whether Smart_Print() writes the heap and packet dumps to standard
  // output.
  [[nodiscard]] bool heap_dump() const { return heap_dump_; }
  void set_heap_dump(bool on) { heap_dump_ = on; }

  // The sync-bug trap, which CONQUER.INI's [SyncBug] section sets up. From
  // trap_frame() on, the game looks each frame for an object of
  // trap_object_type() at trap_coord() or at trap_this(), and parks it
  // where a debugger can see it. trap_check_heap() walks the heaps instead.
  int64_t& trap_frame() ABSL_ATTRIBUTE_LIFETIME_BOUND { return trap_frame_; }
  RTTIType& trap_object_type() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return trap_object_type_;
  }
  COORDINATE& trap_coord() ABSL_ATTRIBUTE_LIFETIME_BOUND { return trap_coord_; }
  void*& trap_this() ABSL_ATTRIBUTE_LIFETIME_BOUND { return trap_this_; }
  int& trap_check_heap() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return trap_check_heap_;
  }

 private:
  bool developer_mode_ = false;
  bool playtest_ = false;
  bool map_editor_active_ = false;
  bool unshroud_ = false;
  bool quiet_ = false;
  bool check_map_ = false;
  bool build_anything_ = false;
  bool instant_build_ = false;
  bool show_cell_info_ = false;
  bool show_passability_ = false;
  bool show_threat_ = false;
  bool trace_path_search_ = false;
  bool heap_dump_ = false;

  // 0x7fffffff means "never", which is what the INI file defaults to.
  int64_t trap_frame_ = 0x7fffffff;
  RTTIType trap_object_type_ = RTTI_NONE;
  COORDINATE trap_coord_ = 0;
  void* trap_this_ = nullptr;
  int trap_check_heap_ = 0;
};

// Returns the DebugState that Game installed. CHECK-fails outside a Game's
// lifetime unless a test installed its own.
inline DebugState& TheDebugState() {
  return base::Installed<DebugState>::Get();
}

#endif  // CNC_RED_ALERT_TD_DEBUG_STATE_H_
