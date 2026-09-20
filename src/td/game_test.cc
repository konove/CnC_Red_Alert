// Tests that a Tiberian Dawn Game can be built and torn down without a window.

#include "td/game.h"

#include <cstddef>
#include <memory>
#include <type_traits>
#include <vector>

#include "base/installed.h"
#include "gtest/gtest.h"
#include "sdllib/ww_win.h"
#include "td/assets.h"
#include "td/palettes.h"
#include "td/screen.h"

// ww_win.cc, pulled in through gbuffer, dispatches events to the app.
void SDL_Event_Handler(SDL_Event* /*event*/) {}

// LoadFonts() and LoadStrings() are not run here; these stand in for the game
// code they call, which would pull in most of the engine.
class File;
// NOLINTBEGIN(misc-use-internal-linkage): these satisfy other units' externs.
std::vector<std::byte> LoadAllocData(File& file);
std::vector<std::byte> LoadAllocData(File& /*file*/) { return {}; }
const char* Language_Name(const char* basename);
const char* Language_Name(const char* /*basename*/) { return "CONQUER.ENG"; }
int Get_CD_Index(int cd_drive, int timeout);
int Get_CD_Index(int /*cd_drive*/, int /*timeout*/) { return -1; }
// NOLINTEND(misc-use-internal-linkage)

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
  EXPECT_FALSE(base::Installed<Assets>::IsInstalled());
  {
    const Game game;
    EXPECT_TRUE(base::Installed<Screen>::IsInstalled());
    EXPECT_TRUE(base::Installed<Palettes>::IsInstalled());
    EXPECT_TRUE(base::Installed<Assets>::IsInstalled());
  }
  EXPECT_FALSE(base::Installed<Screen>::IsInstalled());
  EXPECT_FALSE(base::Installed<Palettes>::IsInstalled());
  EXPECT_FALSE(base::Installed<Assets>::IsInstalled());
}

}  // namespace
