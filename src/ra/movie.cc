/*
**	Command & Conquer Red Alert(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// File: Plays VQA movies through the VQA player, scaling them to the screen.

#include "ra/movie.h"

#include <absl/log/check.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <span>

#include "absl/log/log.h"
#include "engine/audio/audio_mixer.h"
#include "engine/file/game_file.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/video/game_file_vqa_io.h"
#include "engine/video/mixer_vqa_audio.h"
#include "engine/video/vqa/vqa_player.h"
#include "engine/window/display.h"
#include "engine/window/keyboard.h"
#include "engine/window/ww_mouse.h"
#include "ra/const.h"
#include "ra/debug_state.h"
#include "ra/defines.h"
#include "ra/game_state.h"
#include "ra/input.h"
#include "ra/interpal.h"
#include "ra/palette.h"
#include "ra/palettes.h"
#include "ra/screen.h"
#include "ra/session.h"
#include "ra/theme.h"
#include "ra/winstub.h"

namespace {

// Where a movie's frames go: the frame is copied, centered, into the page it
// is scaled from - the 640x400 vq640 page for the logo, else the 320x200
// system memory page - and presented. Esc stops the movie where breaking out
// is allowed; losing the window's focus pauses it.
class MovieScreen final : public VqaClient {
 public:
  bool OnFrame(const VqaFrameView& frame) override {
    if (!frame.palette.empty()) {
      SetPalette(frame.palette);
    }
    PixelView& page = Page().view();
    page.CopyFromBuffer((page.width() - frame.width) / 2,
                        (page.height() - frame.height) / 2, frame.width,
                        frame.height, frame.pixels);
    return Present();
  }

  // A dropped frame still presents and reads the keyboard, so Esc works
  // while playback catches up.
  bool OnFrameSkipped(int /*frame_number*/) override { return Present(); }

  // Too early for the next frame: wait for the display's next frame.
  void OnIdle() override { engine::window::TheDisplay().EndFrame(); }

  // Whether the player pressed Esc to stop the movie.
  [[nodiscard]] bool broken_out() const { return broken_out_; }

 private:
  static PixelBuffer& Page() {
    return TheScreen().is_vq640() ? TheScreen().vq640()
                                  : TheScreen().sys_mem_page();
  }

  // Movie palettes are 6 bits a color; brighten them the way the DOS game
  // did, then set them.
  static void SetPalette(const std::span<const uint8_t> palette) {
    std::array<uint8_t, 768> colors{};
    std::ranges::copy(palette.first(std::min(palette.size(), colors.size())),
                      colors.begin());
    for (uint8_t& color : colors) {
      color &= 63;
    }
    Increase_Palette_Luminance(colors, 15, 15, 15, 63);
    Set_Palette(colors);
  }

  // Shows the page and services the keyboard and the window's focus. Returns
  // false when the player pressed Esc to stop the movie.
  bool Present() {
    int key = 0;
    if (TheKeyboard().Peek()) {
      key = TheKeyboard().Read();
      TheKeyboard().Clear();
    }
    if (TheScreen().is_vq640()) {
      TheScreen().vq640().view().BlitTo(TheScreen().visible_view());
    } else {
      Interpolate_2X_Scale(&TheScreen().sys_mem_page(),
                           &TheScreen().visible_view(), nullptr);
    }
    // ServiceRealTime() is deliberately not invoked here. The VQA player
    // drives audio itself while a movie runs, and the game logic it would
    // service is stopped.

    if ((TheGameState().breakout_allowed() ||
         TheDebugState().developer_mode()) &&
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
        Check_For_Focus_Loss();
      }
    }
    engine::window::TheDisplay().EndFrame();
    return true;
  }

  bool broken_out_ = false;
};

}  // namespace

void Play_Movie(const char* name, const ThemeType theme, bool clear_screen) {
  // Both named and enum-based movies come through here, including campaign
  // briefings that would otherwise delay headless save/load checks.
  if (bNoMovies) {
    return;
  }

  DLOG(INFO) << "Play_Movie: " << name;

  // A movie blocks until it finishes, which would stall every other player in
  // a multiplayer session and interrupt editing, so both modes skip it.
  if (TheDebugState().map_editor_active()) {
    return;
  }
  if (TheSession().Type != GAME_NORMAL) {
    return;
  }

  if (name) {
    const auto fullname =
        std::filesystem::path(name).replace_extension(".VQA").string();
    if (!GameFileExists(fullname)) {
      DLOG(WARNING) << "Play_Movie: file not found: " << fullname;
      return;
    }

    // Fade audio and video to black before launching the VQA player. The
    // adjust-set-adjust-set sequence below drives the palette to black and
    // then back, which is what produces the fade rather than a hard cut.
    Hide_Mouse();
    TheTheme().Queue_Song(theme);
    if (!clear_screen) {
      ThePalettes().black_palette().Set(kFadePaletteMedium);
      TheScreen().visible_page().view().Clear();
      ThePalettes().black_palette().Adjust(0x08, ThePalettes().white_palette());
      ThePalettes().black_palette().Set();
      ThePalettes().black_palette().Adjust(0xFF);
      ThePalettes().black_palette().Set();
    }
    TheKeyboard().Clear();

    // The file, the screen and the sound device must outlive the player.
    GameFileVqaIo movie_io;
    MovieScreen screen;
    MixerVqaAudio movie_audio(engine::audio::TheAudio());
    const bool with_sound =
        !TheDebugState().quiet() && engine::audio::TheAudio().is_open();

    if (auto player = VqaPlayer::Open(movie_io, fullname, screen,
                                      with_sound ? &movie_audio : nullptr)) {
      TheScreen().sys_mem_page().view().Clear();
      TheGameState().in_movie() = true;
      player->Run();
      TheGameState().in_movie() = false;
      TheScreen().set_is_vq640(false);

      // Early exit leaves the palette in an inconsistent state.
      if (screen.broken_out()) {
        clear_screen = true;
        TheScreen().visible_page().view().Clear();
      }
    } else {
      DLOG(FATAL) << "VqaPlayer::Open(" << fullname
                  << ") failed: " << static_cast<int>(player.error());
    }

    // The VQA player may leave the framebuffer and palette dirty.
    if (clear_screen) {
      TheScreen().visible_page().view().Clear();
      ThePalettes().black_palette().Adjust(0x08, ThePalettes().white_palette());
      ThePalettes().black_palette().Set();
      ThePalettes().black_palette().Adjust(0xFF);
      ThePalettes().black_palette().Set();
    }
    Show_Mouse();
  }
}

void Play_Movie(const VQType name, const ThemeType theme,
                const bool clear_screen) {
  if (name != VQ_NONE) {
    if (name == VQ_REDINTRO) {
      TheScreen().set_is_vq640(true);
    }
    Play_Movie(VQName.at(name), theme, clear_screen);
    TheScreen().set_is_vq640(false);
  }
}
