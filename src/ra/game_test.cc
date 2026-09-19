// Tests that a Red Alert Game can be built and torn down without a window.

#include "ra/game.h"

#include <memory>
#include <type_traits>

#include "base/installed.h"
#include "gtest/gtest.h"
#include "ra/palettes.h"
#include "ra/screen.h"
#include "sdllib/ww_win.h"

// ww_win.cc, pulled in through gbuffer, dispatches events to the app.
void SDL_Event_Handler(SDL_Event* /*event*/) {}

namespace {

// Only ShutDown() may end the one Game, so it cannot be copied or moved out.
static_assert(!std::is_copy_constructible_v<Game>);
static_assert(!std::is_move_constructible_v<Game>);

TEST(GameTest, BuildsAndTearsDownWithoutAWindowOrGameData) {
  auto game = std::make_unique<Game>();
  EXPECT_NE(game, nullptr);
  game.reset();
}

TEST(GameTest, InstallsItsSubsystemsForItsLifetime) {
  EXPECT_FALSE(base::Installed<Screen>::IsInstalled());
  EXPECT_FALSE(base::Installed<Palettes>::IsInstalled());
  {
    const Game game;
    EXPECT_TRUE(base::Installed<Screen>::IsInstalled());
    EXPECT_TRUE(base::Installed<Palettes>::IsInstalled());
  }
  EXPECT_FALSE(base::Installed<Screen>::IsInstalled());
  EXPECT_FALSE(base::Installed<Palettes>::IsInstalled());
}

}  // namespace
