// Tests that a Tiberian Dawn Game can be built and torn down without a window.

#include "td/startup/game.h"

#include <memory>
#include <type_traits>

#include "base/installed.h"
#include "gtest/gtest.h"
#include "td/assets/assets.h"
#include "td/assets/palettes.h"
#include "td/assets/screen.h"
#include "td/core/object_heaps.h"
#include "td/core/special_flags.h"
#include "td/map/world.h"

namespace td {

namespace {

// Only ShutDown() may end the one Game, so it cannot be copied or moved out.
static_assert(!std::is_copy_constructible_v<Game>);
static_assert(!std::is_move_constructible_v<Game>);

TEST(GameTest, BuildsAndTearsDownWithoutAWindowOrGameData) {
  auto game = std::make_unique<Game>();
  EXPECT_NE(game, nullptr);
  game.reset();
}

TEST(GameTest, StartsWithItsDefaultSpecialFlags) {
  const Game game{};

  const SpecialFlags& flags = TheSpecialFlags();
  EXPECT_TRUE(flags.enemy_health_bars);
  EXPECT_TRUE(flags.defender_advantage);
  EXPECT_TRUE(flags.tiberium_grows);
  EXPECT_TRUE(flags.tiberium_spreads);
  EXPECT_TRUE(flags.tiberium_fast);
  EXPECT_FALSE(flags.easy);
  EXPECT_FALSE(flags.difficult);
  EXPECT_FALSE(flags.scatter);
  EXPECT_FALSE(flags.three_point_turns);
  EXPECT_FALSE(flags.from_wchat);
}

TEST(GameTest, InstallsItsSubsystemsForItsLifetime) {
  EXPECT_FALSE(base::Installed<Screen>::IsInstalled());
  EXPECT_FALSE(base::Installed<Palettes>::IsInstalled());
  EXPECT_FALSE(base::Installed<Assets>::IsInstalled());
  EXPECT_FALSE(base::Installed<ObjectHeaps>::IsInstalled());
  EXPECT_FALSE(base::Installed<World>::IsInstalled());
  {
    const Game game{};
    EXPECT_TRUE(base::Installed<Screen>::IsInstalled());
    EXPECT_TRUE(base::Installed<Palettes>::IsInstalled());
    EXPECT_TRUE(base::Installed<Assets>::IsInstalled());
    EXPECT_TRUE(base::Installed<ObjectHeaps>::IsInstalled());
    EXPECT_TRUE(base::Installed<World>::IsInstalled());
  }
  EXPECT_FALSE(base::Installed<Screen>::IsInstalled());
  EXPECT_FALSE(base::Installed<Palettes>::IsInstalled());
  EXPECT_FALSE(base::Installed<Assets>::IsInstalled());
  EXPECT_FALSE(base::Installed<ObjectHeaps>::IsInstalled());
  EXPECT_FALSE(base::Installed<World>::IsInstalled());
}

}  // namespace

}  // namespace td
