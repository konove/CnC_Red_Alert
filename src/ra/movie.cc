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

#include <cstdint>
#include <filesystem>
#include <span>
#include <string>

#include "absl/log/log.h"
#include "ra/const.h"
#include "ra/defines.h"
#include "ra/externs.h"
#include "ra/globals.h"
#include "ra/init.h"
#include "ra/interpal.h"
#include "ra/jshell.h"
#include "ra/palette.h"
#include "ra/session.h"
#include "ra/theme.h"
#include "sdllib/gbuffer.h"
#include "sdllib/keyboard.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/ww_win.h"
#include "tech/game_file.h"
#include "tech/game_file_vqa_io.h"
#include "tech/ww_audio.h"
#include "winvq/vqa32/vqaplay.h"

void Play_Movie(const char* name, const ThemeType theme, bool clear_screen) {
  // Both named and enum-based movies come through here, including campaign
  // briefings that would otherwise delay headless save/load checks.
  if (bNoMovies) {
    return;
  }

  DLOG(INFO) << "Play_Movie: " << name;

  // A movie blocks until it finishes, which would stall every other player in
  // a multiplayer session and interrupt editing, so both modes skip it.
  if (MapEditorActive) {
    return;
  }
  if (Session.Type != GAME_NORMAL) {
    return;
  }

  if (name) {
    const auto fullname =
        std::filesystem::path(name).replace_extension(".VQA").string();
    const auto pal_name =
        std::filesystem::path(name).replace_extension(".VQP").string();
    if (!GameFile(fullname).IsAvailable()) {
      DLOG(WARNING) << "Play_Movie: file not found: " << fullname;
      return;
    }
    Anim_Init();

    // Fade audio and video to black before launching the VQA player. The
    // adjust-set-adjust-set sequence below drives the palette to black and
    // then back, which is what produces the fade rather than a hard cut.
    Hide_Mouse();
    Theme.Queue_Song(theme);
    if (PreserveVQAScreen == 0 && !clear_screen) {
      BlackPalette.Set(kFadePaletteMedium);
      VisiblePage.Clear();
      BlackPalette.Adjust(0x08, WhitePalette);
      BlackPalette.Set();
      BlackPalette.Adjust(0xFF);
      BlackPalette.Set();
    }
    PreserveVQAScreen = 0;
    Keyboard->Clear();

    VqaPlayer player;
    GameFileVqaIo movie_io;  // Must outlive the open movie.
    player.SetIo(&movie_io);

    if (IsVQ640) {
      AnimControl.ImageWidth = 640;
      AnimControl.ImageHeight = 400;
      AnimControl.ImageBuf = VQ640.Get_Bytes();
    } else {
      AnimControl.ImageWidth = 320;
      AnimControl.ImageHeight = 200;
      AnimControl.ImageBuf = SysMemPage.Get_Bytes();
    }

    if (!Debug_Quiet && Audio.is_open()) {
      AnimControl.OptionFlags |= VQAOPTF_AUDIO;
    } else {
      AnimControl.OptionFlags &= ~VQAOPTF_AUDIO;
    }

    if (player.Open(fullname.c_str(), &AnimControl) == 0) {
      Brokeout = false;
      if (!IsVQ640) {
        Load_Interpolated_Palettes(pal_name.c_str());
      }
      SysMemPage.Clear();
      InMovie = true;
      player.Play(VQAMODE_RUN);
      player.Close();
      InMovie = false;
      if (!IsVQ640) {
        Free_Interpolated_Palettes();
      }
      IsVQ640 = false;

      // Early exit leaves the palette in an inconsistent state.
      if (Brokeout) {
        clear_screen = true;
        VisiblePage.Clear();
        Brokeout = false;
      }
    } else {
      DLOG(FATAL) << "VQA_Open failed unexpectedly";
    }

    // The VQA player may leave the framebuffer and palette dirty.
    if (clear_screen) {
      VisiblePage.Clear();
      BlackPalette.Adjust(0x08, WhitePalette);
      BlackPalette.Set();
      BlackPalette.Adjust(0xFF);
      BlackPalette.Set();
    }
    Show_Mouse();
  }
}

void Play_Movie(const VQType name, const ThemeType theme,
                const bool clear_screen) {
  if (name != VQ_NONE) {
    if (name == VQ_REDINTRO) {
      IsVQ640 = true;
    }
    Play_Movie(VQName.at(name), theme, clear_screen);
    IsVQ640 = false;
  }
}

int32_t VQ_Call_Back(unsigned char* /*unused*/, int32_t /*unused*/) {
  int key = 0;
  if (Keyboard->Check()) {
    key = Keyboard->Get();
    Keyboard->Clear();
  }
  Check_VQ_Palette_Set();
  if (IsVQ640) {
    VQ640.Blit(SeenBuff);
  } else {
    Interpolate_2X_Scale(&SysMemPage, &SeenBuff, nullptr);
  }
  // ServiceRealTime() is deliberately not invoked here. The VQA player drives
  // audio itself while a movie runs, and the game logic it would service is
  // stopped.

  if ((BreakoutAllowed || Debug_Flag) && key == KN_ESC) {
    Keyboard->Clear();
    Brokeout = true;
    return 1;
  }

  if (!GameInFocus) {
    VQA_PauseAudio();
    while (!GameInFocus) {
      Check_For_Focus_Loss();
    }
  }
  Video_End_Frame();
  return 0;
}

int32_t VQ_Event_Handler(const uint32_t event, void* /*buffer*/,
                         int32_t /*n_bytes*/) {
  // vsync while waiting for frame
  if (event == VQAEVENT_SYNC) {
    Video_End_Frame();
  }
  return 0;
}
