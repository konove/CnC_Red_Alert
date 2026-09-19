// Tests that a Tiberian Dawn Game can be built and torn down without a window.

#include "td/game.h"

#include <memory>
#include <type_traits>

#include "gtest/gtest.h"

namespace {

// Only ShutDown() may end the one Game, so it cannot be copied or moved out.
static_assert(!std::is_copy_constructible_v<Game>);
static_assert(!std::is_move_constructible_v<Game>);

TEST(GameTest, BuildsAndTearsDownWithoutAWindowOrGameData) {
  auto game = std::make_unique<Game>();
  EXPECT_NE(game, nullptr);
  game.reset();
}

}  // namespace
