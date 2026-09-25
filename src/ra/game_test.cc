// Tests that a Red Alert Game can be built and torn down without a window.

#include "ra/game.h"

#include <memory>
#include <type_traits>

#include "base/installed.h"
#include "engine/audio/audio_mixer.h"
#include "gtest/gtest.h"
#include "ra/assets.h"
#include "ra/goptions.h"
#include "ra/network.h"
#include "ra/palettes.h"
#include "ra/rules.h"
#include "ra/screen.h"
#include "ra/session.h"
#include "ra/special.h"
#include "ra/theme.h"

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
  const auto installed = [] {
    return base::Installed<Screen>::IsInstalled() &&
           base::Installed<Palettes>::IsInstalled() &&
           base::Installed<Assets>::IsInstalled() &&
           base::Installed<RulesClass>::IsInstalled() &&
           base::Installed<GameOptionsClass>::IsInstalled() &&
           base::Installed<SpecialClass>::IsInstalled() &&
           base::Installed<AudioMixer>::IsInstalled() &&
           base::Installed<ThemeClass>::IsInstalled() &&
           base::Installed<SessionClass>::IsInstalled() &&
           base::Installed<Network>::IsInstalled();
  };
  EXPECT_FALSE(installed());
  {
    const Game game{};
    EXPECT_TRUE(installed());
  }
  EXPECT_FALSE(installed());
}

}  // namespace
