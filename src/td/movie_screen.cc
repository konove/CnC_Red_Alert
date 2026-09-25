#include "td/movie_screen.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <span>

#include "engine/audio/audio_mixer.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/video/vqa/vqa_player.h"
#include "engine/window/display.h"
#include "engine/window/keyboard.h"
#include "td/debug_state.h"
#include "td/game_state.h"
#include "td/input.h"
#include "td/interpal.h"
#include "td/palette.h"
#include "td/screen.h"
#include "td/winstub.h"

namespace {

// Movie palettes are 6 bits a color; brightens one the way the DOS game did,
// then sets it.
void SetMoviePalette(const std::span<const uint8_t> palette) {
  std::array<uint8_t, 768> colors{};
  std::ranges::copy(palette.first(std::min(palette.size(), colors.size())),
                    colors.begin());
  for (uint8_t& color : colors) {
    color &= 63;
  }
  Increase_Palette_Luminance(colors, 15, 15, 15, 63);
  Set_Palette(colors);
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
    key = TheKeyboard().Read();
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
