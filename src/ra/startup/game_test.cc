// Tests that a Red Alert Game can be built and torn down without a window.

#include "ra/startup/game.h"

#include <memory>
#include <type_traits>

#include "base/installed.h"
#include "engine/audio/audio_mixer.h"
#include "gtest/gtest.h"
#include "ra/ai/special.h"
#include "ra/assets/assets.h"
#include "ra/assets/palettes.h"
#include "ra/assets/screen.h"
#include "ra/assets/theme.h"
#include "ra/data/rules.h"
#include "ra/net/network.h"
#include "ra/net/session.h"
#include "ra/screens/goptions.h"

namespace ra {

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
           base::Installed<engine::audio::AudioMixer>::IsInstalled() &&
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

// The IPX manager stops listening when Game is destroyed, so the network has
// to be torn down while it is still installed.
TEST(GameTest, TearsDownANetworkThatIsListening) {
  auto game = std::make_unique<Game>();
  // An Internet session listens without opening an IPX socket.
  TheSession().Type = GAME_INTERNET;
  ASSERT_EQ(TheNetwork().ipx().Init(/*internet=*/true), 1);
  game.reset();
  EXPECT_FALSE(base::Installed<Network>::IsInstalled());
}

}  // namespace

}  // namespace ra
