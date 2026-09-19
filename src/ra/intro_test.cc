// Tests for the first-launch introduction. The dialog, the movie player, the
// mouse and the title page are link-time stubs that record what was asked of
// them.

#include "ra/intro.h"

#include <cstddef>
#include <cstdint>
#include <vector>

#include "gtest/gtest.h"
#include "ra/defines.h"
#include "ra/externs.h"
#include "ra/globals.h"
#include "ra/init.h"
#include "ra/movie.h"
#include "ra/mplayer.h"
#include "ra/msgbox.h"
#include "ra/palette.h"
#include "sdllib/gbuffer.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/ww_win.h"

namespace {

bool using_dvd = false;
int dialog_answer = 0;  // Index of the button the stub dialog "presses".
int dialogs_shown = 0;
int movies_played = 0;
int mouse_hides = 0;  // Hide_Mouse() calls not yet matched by Show_Mouse().

}  // namespace

// The globals intro.cc reaches for.
int CurrentCD = -1;
PaletteClass BlackPalette;
PaletteClass CCPalette;
PaletteClass GamePalette;
GraphicViewPortClass HidPage;
GraphicViewPortClass SeenBuff;

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

// ww_win.cc, pulled in through gbuffer, dispatches events to the app.
void SDL_Event_Handler(SDL_Event* /*event*/) {}

namespace {

class IntroTest : public testing::Test {
 protected:
  void SetUp() override {
    HidPage.Attach(&hidden_, 0, 0, kWidth, kHeight);
    SeenBuff.Attach(&seen_, 0, 0, kWidth, kHeight);
    CurrentCD = -1;
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
  GraphicBufferClass hidden_{kWidth, kHeight, hidden_pixels_};
  GraphicBufferClass seen_{kWidth, kHeight, seen_pixels_};
};

TEST_F(IntroTest, CdInstallPlaysTheIntroWithoutAsking) {
  using_dvd = false;

  PlayFirstLaunchIntro();

  EXPECT_EQ(dialogs_shown, 0);
  EXPECT_EQ(movies_played, 1);
  EXPECT_EQ(CurrentCD, -1);
  EXPECT_EQ(mouse_hides, 0);
}

TEST_F(IntroTest, DvdAsksForTheSideBeforeTheIntro) {
  using_dvd = true;
  dialog_answer = 1;

  PlayFirstLaunchIntro();

  EXPECT_EQ(dialogs_shown, 1);
  EXPECT_EQ(movies_played, 1);
  EXPECT_EQ(CurrentCD, 1);
}

TEST_F(IntroTest, DvdLeavesTheMouseAsItFoundIt) {
  using_dvd = true;

  PlayFirstLaunchIntro();

  EXPECT_EQ(mouse_hides, 0);
}

}  // namespace
