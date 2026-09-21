// Tests for the first-launch introduction. The dialog, the movie player, the
// mouse and the title page are link-time stubs that record what was asked of
// them.

#include "ra/intro.h"

#include <cstddef>
#include <cstdint>
#include <vector>

#include "base/installed.h"
#include "gtest/gtest.h"
#include "ra/defines.h"
#include "ra/game_state.h"
#include "ra/init.h"
#include "ra/movie.h"
#include "ra/mplayer.h"
#include "ra/msgbox.h"
#include "ra/palette.h"
#include "ra/palettes.h"
#include "sdllib/graphic_buffer.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/ww_win.h"

namespace {

bool using_dvd = false;
int dialog_answer = 0;  // Index of the button the stub dialog "presses".
int dialogs_shown = 0;
int movies_played = 0;
int mouse_hides = 0;  // Hide_Mouse() calls not yet matched by Show_Mouse().

// intro.cc reads the disc number out of the run state, so the test
// installs one.
GameState game_state;
const base::Installed<GameState>::Scope game_state_scope(game_state);

}  // namespace

bool Using_DVD() { return using_dvd; }
void Hide_Mouse() { ++mouse_hides; }
void Show_Mouse() { --mouse_hides; }
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

// ww_win.cc, pulled in through graphic_buffer, dispatches events to the app.
void SDL_Event_Handler(SDL_Event* /*event*/) {}

namespace {

class IntroTest : public testing::Test {
 protected:
  void SetUp() override {
    TheGameState().current_cd() = -1;
    dialogs_shown = 0;
    movies_played = 0;
    mouse_hides = 0;
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
};

TEST_F(IntroTest, CdInstallPlaysTheIntroWithoutAsking) {
  using_dvd = false;

  PlayFirstLaunchIntro(hidden_, seen_);

  EXPECT_EQ(dialogs_shown, 0);
  EXPECT_EQ(movies_played, 1);
  EXPECT_EQ(TheGameState().current_cd(), -1);
  EXPECT_EQ(mouse_hides, 0);
}

TEST_F(IntroTest, DvdAsksForTheSideBeforeTheIntro) {
  using_dvd = true;
  dialog_answer = 1;

  PlayFirstLaunchIntro(hidden_, seen_);

  EXPECT_EQ(dialogs_shown, 1);
  EXPECT_EQ(movies_played, 1);
  EXPECT_EQ(TheGameState().current_cd(), 1);
}

TEST_F(IntroTest, DvdLeavesTheMouseAsItFoundIt) {
  using_dvd = true;

  PlayFirstLaunchIntro(hidden_, seen_);

  EXPECT_EQ(mouse_hides, 0);
}

}  // namespace
