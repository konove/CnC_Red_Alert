#include "td/screens/movie_screen.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <span>

#include "engine/audio/audio_mixer.h"
#include "engine/gfx/palette.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/video/vqa/vqa_player.h"
#include "engine/window/display.h"
#include "engine/window/keyboard.h"
#include "td/assets/interpal.h"
#include "td/assets/palette.h"
#include "td/assets/screen.h"
#include "td/debug/debug_state.h"
#include "td/startup/game_state.h"
#include "td/startup/winstub.h"
#include "td/ui/input.h"

namespace {

// Movie palettes are 6 bits a color; brightens one the way the DOS game did,
// then sets it.
void SetMoviePalette(const std::span<const uint8_t> palette) {
  std::array<uint8_t, engine::gfx::Palette::kFileSize> colors{};
  std::ranges::copy(palette.first(std::min(palette.size(), colors.size())),
                    colors.begin());
  engine::gfx::Palette brightened = engine::gfx::Palette::FromVga(colors);
  engine::gfx::BrightenPalette(brightened, 15, 15, 15);
  Set_Palette(brightened);
}

}  // namespace

bool MovieScreen::OnFrame(const VqaFrameView& frame) {
  if (!frame.palette.empty()) {
    SetMoviePalette(frame.palette);
  }
  PixelView& page = TheScreen().sys_mem_page().view();
  page.CopyFromBuffer((page.width() - frame.width) / 2,
                      (page.height() - frame.height) / 2, frame.width,
                      frame.height, frame.pixels);
  return Present();
}

bool MovieScreen::OnFrameSkipped(const int /*frame_number*/) {
  return Present();
}

void MovieScreen::OnIdle() { engine::window::TheDisplay().EndFrame(); }

bool MovieScreen::Present() {
  int key = 0;
  if (TheKeyboard().Peek()) {
    key = TheKeyboard().Read().key;
    TheKeyboard().Clear();
  }

  Interpolate_2X_Scale(&TheScreen().sys_mem_page(), &TheScreen().visible_view(),
                       nullptr);

  if ((TheGameState().breakout_allowed() || TheDebugState().developer_mode()) &&
      key == engine::window::KN_ESC) {
    TheKeyboard().Clear();
    broken_out_ = true;
    return false;
  }

  // The movie's clock follows its sound, so pausing the sound holds the
  // frames too; Check_For_Focus_Loss() resumes it with the focus.
  if (!TheGameState().in_focus()) {
    engine::audio::TheAudio().SetExtraPaused(true);
    while (!TheGameState().in_focus()) {
      TheKeyboard().Peek();
      Check_For_Focus_Loss();
    }
  }

  engine::window::TheDisplay().EndFrame();
  return true;
}
