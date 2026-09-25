// Tests the initial state of DebugState and TheDebugState().

#include "ra/debug_state.h"

#include "base/installed.h"
#include "gtest/gtest.h"

namespace {

TEST(DebugStateTest, EverySwitchStartsOff) {
  const DebugState debug_state;
  EXPECT_FALSE(debug_state.developer_mode());
  EXPECT_FALSE(debug_state.playtest());
  EXPECT_FALSE(debug_state.map_editor_active());
  EXPECT_FALSE(debug_state.unshroud());
  EXPECT_FALSE(debug_state.quiet());
  EXPECT_FALSE(debug_state.print_events());
  EXPECT_FALSE(debug_state.check_map());
  EXPECT_FALSE(debug_state.check_heaps());
  EXPECT_FALSE(debug_state.build_anything());
  EXPECT_FALSE(debug_state.show_cell_info());
  EXPECT_FALSE(debug_state.show_passability());
  EXPECT_FALSE(debug_state.motion_capture());
}

TEST(DebugStateTest, SwitchesRememberWhatWasSet) {
  DebugState debug_state;
  debug_state.set_developer_mode(true);
  debug_state.set_unshroud(true);
  EXPECT_TRUE(debug_state.developer_mode());
  EXPECT_TRUE(debug_state.unshroud());
  EXPECT_FALSE(debug_state.playtest());

  debug_state.set_unshroud(false);
  EXPECT_FALSE(debug_state.unshroud());
}

TEST(DebugStateTest, TheDebugStateReturnsTheInstalledDebugState) {
  DebugState debug_state;
  const base::Installed<DebugState>::Scope scope(debug_state);
  EXPECT_EQ(&TheDebugState(), &debug_state);
}

}  // namespace
