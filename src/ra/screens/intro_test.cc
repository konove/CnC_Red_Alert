// Tests for the first-launch introduction. The dialog, the movie player and
// the title page are link-time stubs that record what was asked of them; the
// mouse is a real one, in an Input of the test's own, whose hide count shows
// what was done to it.

#include "ra/screens/intro.h"

#include <cstddef>
#include <cstdint>
#include <vector>

#include "base/installed.h"
#include "engine/gfx/pixel_buffer.h"
#include "gtest/gtest.h"
#include "ra/assets/palette.h"
#include "ra/assets/palettes.h"
#include "ra/core/defines.h"
#include "ra/net/mplayer.h"
#include "ra/screens/movie.h"
#include "ra/startup/game_state.h"
#include "ra/startup/init.h"
#include "ra/ui/input.h"
#include "ra/ui/msgbox.h"

namespace ra {

namespace {

bool using_dvd = false;
int dialog_answer = 0;  // Index of the button the stub dialog "presses".
int dialogs_shown = 0;
int movies_played = 0;

// intro.cc reads the disc number out of the run state, so the test
// installs one.
GameState game_state;
const base::Installed<GameState>::Scope game_state_scope(game_state);

}  // namespace

bool Using_DVD() { return using_dvd; }
void Load_Title_Page(bool /*visible*/) {}
void PaletteClass::Set(int /*time*/, void (* /*callback*/)()) {}
void Play_Movie(VQType /*name*/, ThemeType /*theme*/, bool /*clear_screen*/) {
  ++movies_played;
}
// Stands in for the game's member function, so it cannot be static.
// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
int WWMessageBox::Process(int /*msg*/, int /*b1txt*/, int /*b2txt*/,
                          int /*b3txt*/, bool /*preserve*/) const {
  ++dialogs_shown;
  return dialog_answer;
}

namespace {

class IntroTest : public testing::Test {
 protected:
  void SetUp() override {
    TheGameState().current_cd() = -1;
    dialogs_shown = 0;
    movies_played = 0;
    // Hidden once, so that a Show() without its Hide() shows up as well.
    TheMouse().Hide();
  }

  static constexpr int kWidth = 8;
  static constexpr int kHeight = 8;
  std::vector<uint8_t> hidden_pixels_ =
      std::vector<uint8_t>(size_t{kWidth} * kHeight);
  std::vector<uint8_t> seen_pixels_ =
      std::vector<uint8_t>(size_t{kWidth} * kHeight);
  PixelBuffer hidden_{kWidth, kHeight, hidden_pixels_};
  PixelBuffer seen_{kWidth, kHeight, seen_pixels_};
  Palettes palettes_;
  base::Installed<Palettes>::Scope palettes_scope_{palettes_};
  Input input_;
  base::Installed<Input>::Scope input_scope_{input_};
};

TEST_F(IntroTest, CdInstallPlaysTheIntroWithoutAsking) {
  using_dvd = false;

  PlayFirstLaunchIntro(hidden_.view(), seen_.view());

  EXPECT_EQ(dialogs_shown, 0);
  EXPECT_EQ(movies_played, 1);
  EXPECT_EQ(TheGameState().current_cd(), -1);
  EXPECT_EQ(TheMouse().hide_count(), 1);
}

TEST_F(IntroTest, DvdAsksForTheSideBeforeTheIntro) {
  using_dvd = true;
  dialog_answer = 1;

  PlayFirstLaunchIntro(hidden_.view(), seen_.view());

  EXPECT_EQ(dialogs_shown, 1);
  EXPECT_EQ(movies_played, 1);
  EXPECT_EQ(TheGameState().current_cd(), 1);
}

TEST_F(IntroTest, DvdLeavesTheMouseAsItFoundIt) {
  using_dvd = true;

  PlayFirstLaunchIntro(hidden_.view(), seen_.view());

  EXPECT_EQ(TheMouse().hide_count(), 1);
}

}  // namespace

}  // namespace ra
