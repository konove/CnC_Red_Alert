/*
**	Command & Conquer(tm)
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

/* $Header:   F:\projects\c&c\vcs\code\ending.cpv   1.5   16 Oct 1995 16:50:30
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : ENDING.H *
 *                                                                                             *
 *                   Programmer : Barry W. Green *
 *                                                                                             *
 *                   Start Date : July 10, 1995 *
 *                                                                                             *
 *                  Last Update : July 10, 1995 [BWG] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "td/ending.h"

#include <cstddef>
#include <span>

#include "base/bytes_of.h"
#ifdef NOT_FOR_WIN95
#include <vector>

#include "base/buffer.h"
#endif

#include <cstdint>
#include <cstdio>

#include "absl/strings/str_format.h"
#include "engine/audio/audio_mixer.h"
#include "engine/file/game_file.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/platform/timer.h"
#include "engine/window/display.h"
#include "engine/window/keyboard.h"
#include "engine/window/ww_mouse.h"
#include "td/conquer.h"
#include "td/defines.h"
#include "td/game_state.h"
#include "td/input.h"
#include "td/interpal.h"
#include "td/jshell.h"
#include "td/palette.h"
#include "td/palettes.h"
#include "td/presentation.h"
#include "td/score.h"
#include "td/screen.h"
#include "td/text.h"
#include "td/winstub.h"
#include "td/world.h"

void GDI_Ending() {
#ifdef DEMO
  Fade_Palette_To(ThePalettes().black_palette(), kFadePaletteMedium, Call_Back);
  Load_Title_Screen("DEMOPIC.PCX", &TheScreen().hidden_view(),
                    ThePalettes().title_palette());
  TheScreen().hidden_view().BlitTo(TheScreen().visible_view());
  Fade_Palette_To(ThePalettes().title_palette(), kFadePaletteMedium, Call_Back);
  TheKeyboard().Clear();
  TheKeyboard().Read();
  Fade_Palette_To(ThePalettes().black_palette(), kFadePaletteMedium, Call_Back);
  TheScreen().visible_page().Clear();

#else
  if (TheWorld().temple_ioned()) {
    Play_Movie("GDIFINB");
  } else {
    Play_Movie("GDIFINA");
  }

  TheWorld().score().Show();

  if (TheWorld().temple_ioned()) {
    Play_Movie("GDIEND2");
  } else {
    Play_Movie("GDIEND1");
  }

  CountDownTimerClass count;
  if (GameFileExists("TRAILER.VQA")) {
    Fade_Palette_To(ThePalettes().black_palette(), kFadePaletteMedium,
                    Call_Back);
    if (const auto file = OpenGameFile("ATTRACT2.CPS")) {
      Load_Uncompress(*file, TheScreen().sys_mem_page().bytes(),
                      TheScreen().sys_mem_page().bytes(),
                      ThePalettes().title_palette());
    }
    TheScreen().sys_mem_page().view().Scale(TheScreen().visible_view(), 0, 0, 0,
                                            0, 320, 199, 640, 398);
    Fade_Palette_To(ThePalettes().title_palette(), kFadePaletteMedium,
                    Call_Back);
    TheKeyboard().Clear();
    count.Set(int64_t{kTimerSecond} * 3);
    while (count.Time()) {
      Call_Back();
    }
    Fade_Palette_To(ThePalettes().black_palette(), kFadePaletteMedium,
                    Call_Back);

    Play_Movie("TRAILER");  // Red Alert teaser.
  }

  Fade_Palette_To(ThePalettes().black_palette(), kFadePaletteMedium, Call_Back);
  if (const auto file = OpenGameFile("ATTRACT2.CPS")) {
    Load_Uncompress(*file, TheScreen().sys_mem_page().bytes(),
                    TheScreen().sys_mem_page().bytes(),
                    ThePalettes().title_palette());
  }
  TheScreen().sys_mem_page().view().Scale(TheScreen().visible_view(), 0, 0, 0,
                                          0, 320, 199, 640, 398);
  Fade_Palette_To(ThePalettes().title_palette(), kFadePaletteMedium, Call_Back);
  TheKeyboard().Clear();
  //	CountDownTimerClass count;
  count.Set(int64_t{kTimerSecond} * 3);
  while (count.Time()) {
    Call_Back();
  }
  Fade_Palette_To(ThePalettes().black_palette(), kFadePaletteMedium, Call_Back);

  Play_Movie("CC2TEASE");
#endif
}

#ifndef DEMO
/***********************************************************************************************
 * Nod_Ending -- play ending movies for Nod players *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS: *
 *                                                                                             *
 * HISTORY: * 7/10/1995 BWG : Created. *
 *=============================================================================================*/
void Nod_Ending() {
  static const unsigned char _tanpal[] = {0x0,  0xED, 0xED, 0x2C, 0x2C, 0xFB,
                                          0xFB, 0xFD, 0xFD, 0x0,  0x0,  0x0,
                                          0x0,  0x0,  0x52, 0x0};

  char fname[12];
#ifdef NOT_FOR_WIN95
  std::vector<uint8_t> satpic(64000);
#endif  // NOT_FOR_WIN95

  TheWorld().score().Show();

  Presentation show;
  TheScreen().visible_view().Clear();
  TheScreen().hidden_view().Clear();
  show.page().view().Clear();

  std::span<std::byte> localpal;
  if (const auto file = OpenGameFile("SATSEL.PAL")) {
    localpal = Load_Alloc_Data(*file);
  }
  if (const auto file = OpenGameFile("SATSEL.CPS")) {
    Load_Uncompress(*file, TheScreen().sys_mem_page().bytes(),
                    TheScreen().sys_mem_page().bytes(), {});
  }
#ifdef NOT_FOR_WIN95
  base::CopyBytes(std::as_writable_bytes(std::span(satpic)),
                  std::as_bytes(TheScreen().hidden_view().bytes()),
                  satpic.size());
#else
  TheScreen().sys_mem_page().view().BlitTo(show.page().view());
#endif  // NOT_FOR_WIN95
  // Read from the file: MixArchive::RetrieveData() only serves cached archives.
  const auto kanefinl_file = OpenGameFile("KANEFINL.AUD");
  const std::vector<std::byte> kanefinl =
      kanefinl_file ? kanefinl_file->ReadBytes(kanefinl_file->Size())
                    : std::vector<std::byte>{};
  const auto loopie6m_file = OpenGameFile("LOOPIE6M.AUD");
  const std::vector<std::byte> loopie6m =
      loopie6m_file ? loopie6m_file->ReadBytes(loopie6m_file->Size())
                    : std::vector<std::byte>{};

  Play_Movie("NODFINAL", THEME_NONE, false);

  Hide_Mouse();
  engine::window::TheDisplay().EndFrame();
  Set_Palette(base::UnsignedBytes(localpal));
#ifdef NOT_FOR_WIN95
  base::CopyBytes(std::as_writable_bytes(TheScreen().visible_view().bytes()),
                  std::as_writable_bytes(std::span(satpic)), satpic.size());
#endif  // NOT_FOR_WIN95
  Show_Mouse();

  Increase_Palette_Luminance(base::UnsignedBytes(localpal), 30, 30, 30, 63);
  Interpolate_2X_Scale(&show.page(), &TheScreen().visible_view(),
                       "SATSELIN.PAL");

  TheKeyboard().Clear();
  engine::audio::TheAudio().Play(kanefinl, 255, 128);
  engine::audio::TheAudio().Play(loopie6m, 255, 128);

  bool mouseshown = false;
  bool done = false;
  int selection = 1;
  bool printedtext = false;
  while (!done) {
    if (!printedtext && !engine::audio::TheAudio().IsPlaying(kanefinl.data())) {
      printedtext = true;
      Alloc_Object(new ScorePrintClass(show, Text_String(TXT_SEL_TARGET), 0,
                                       180, _tanpal));
      mouseshown = true;
      Show_Mouse();
    }
    Call_Back_Delay(show, 1);
    if (!TheKeyboard().Peek()) {
      if (!engine::audio::TheAudio().IsPlaying(loopie6m.data())) {
        engine::audio::TheAudio().Play(loopie6m, 255, 128);
      }
    } else {
      if (engine::audio::TheAudio().IsPlaying(kanefinl.data())) {
        TheKeyboard().Clear();
      } else {
        const engine::window::InputEvent event = TheKeyboard().Read();
        if (event.IsPress(engine::window::MouseButton::kLeft)) {
          const int mousex = event.x;
          const int mousey = event.y;
          if (mousey >= 44 && mousey <= 354) {
            done = true;
            if (mousex < 320 && mousey < 200) {
              selection = 2;
            }
            if (mousex < 320 && mousey >= 200) {
              selection = 3;
            }
            if (mousex >= 320 && mousey >= 200) {
              selection = 4;
            }
          }
        }
      }
    }
  }
  if (mouseshown) {
    Hide_Mouse();
  }
#ifdef NOT_FOR_WIN95
  satpic.clear();
#endif  // NOT_FOR_WIN95

  /* get rid of all the animating objects */
  for (auto& ScoreObj : ScoreObjs) {
    if (ScoreObj) {
      delete ScoreObj;
      ScoreObj = nullptr;
    }
  }
  // erase the "choose a target" text
  TheScreen().visible_view().FillRect(0, 360, 638, 398, 0);
  show.text_page().view().FillRect(0, 360, 638, 398, 0);

  Hide_Mouse();
  TheKeyboard().Clear();

  engine::audio::TheAudio().Stop(kanefinl.data());
  engine::audio::TheAudio().Stop(loopie6m.data());

  absl::SNPrintF(fname, sizeof(fname), "NODEND%d", selection);
  TheGameState().preserve_movie_screen() = true;
  Play_Movie(fname);

  CountDownTimerClass count;
  if (GameFileExists("TRAILER.VQA")) {
    Fade_Palette_To(ThePalettes().black_palette(), kFadePaletteMedium,
                    Call_Back);
    if (const auto file = OpenGameFile("ATTRACT2.CPS")) {
      Load_Uncompress(*file, TheScreen().sys_mem_page().bytes(),
                      TheScreen().sys_mem_page().bytes(),
                      ThePalettes().title_palette());
    }
    TheScreen().sys_mem_page().view().Scale(TheScreen().visible_view(), 0, 0, 0,
                                            0, 320, 199, 640, 398);
    Fade_Palette_To(ThePalettes().title_palette(), kFadePaletteMedium,
                    Call_Back);
    TheKeyboard().Clear();
    count.Set(int64_t{kTimerSecond} * 3);
    while (count.Time()) {
      Call_Back();
    }
    Fade_Palette_To(ThePalettes().black_palette(), kFadePaletteMedium,
                    Call_Back);

    Play_Movie("TRAILER");  // Red Alert teaser.
  }

  Fade_Palette_To(ThePalettes().black_palette(), kFadePaletteMedium, Call_Back);
  if (const auto file = OpenGameFile("ATTRACT2.CPS")) {
    Load_Uncompress(*file, TheScreen().sys_mem_page().bytes(),
                    TheScreen().sys_mem_page().bytes(),
                    ThePalettes().title_palette());
  }
  TheScreen().sys_mem_page().view().Scale(TheScreen().visible_view(), 0, 0, 0,
                                          0, 320, 199, 640, 398);
  Fade_Palette_To(ThePalettes().title_palette(), kFadePaletteMedium, Call_Back);
  TheKeyboard().Clear();
  //	CountDownTimerClass count;
  count.Set(int64_t{kTimerSecond} * 3);
  while (count.Time()) {
    Call_Back();
  }
  Fade_Palette_To(ThePalettes().black_palette(), kFadePaletteMedium, Call_Back);

  Play_Movie("CC2TEASE");

  delete[] base::CharBytes(std::span(localpal)).data();
}
#endif
