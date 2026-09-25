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

/* $Header:   F:\projects\c&c\vcs\code\intro.cpv   1.6   16 Oct 1995 16:50:18
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : INTRO.H *
 *                                                                                             *
 *                   Programmer : Barry W. Green *
 *                                                                                             *
 *                   Start Date : May 8, 1995 *
 *                                                                                             *
 *                  Last Update : May 8, 1995  [BWG] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "td/intro.h"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <optional>
#include <span>
#include <utility>

#include "base/bytes_of.h"
#include "engine/audio/audio_mixer.h"
#include "engine/file/game_file.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/wsa_animation.h"
#include "engine/platform/timer.h"
#include "engine/video/game_file_vqa_io.h"
#include "engine/video/mixer_vqa_audio.h"
#include "engine/video/vqa/vqa_player.h"
#include "engine/window/display.h"
#include "engine/window/keyboard.h"
#include "engine/window/ww_mouse.h"
#include "td/conquer.h"
#include "td/debug_state.h"
#include "td/defines.h"
#include "td/game_state.h"
#include "td/input.h"
#include "td/interpal.h"
#include "td/jshell.h"
#include "td/movie_screen.h"
#include "td/palette.h"
#include "td/palettes.h"
#include "td/presentation.h"
#include "td/score.h"
#include "td/screen.h"
#include "td/special.h"
#include "td/world.h"

#ifndef DEMO

// Opens a movie without playing it, to be shown on screen and heard on audio
// (nullptr for none). The io object, the screen and the audio device must
// outlive the player. Returns nullopt if the movie cannot be opened.
static std::optional<VqaPlayer> Open_Movie(GameFileVqaIo& io,
                                           MovieScreen& screen,
                                           MixerVqaAudio* audio,
                                           const char* name) {
  auto player = VqaPlayer::Open(io, name, screen, audio);
  if (!player.has_value()) {
    return std::nullopt;
  }
  return std::move(*player);
}

/***********************************************************************************************
 * Choose_Side -- play the introduction movies, select house *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS: *
 *                                                                                             *
 * HISTORY: * 5/08/1995 BWG : Created. *
 *=============================================================================================*/
void Choose_Side() {
  static const unsigned char yellowpal[] = {0x0,  0xC9, 0xBA, 0x93, 0x61, 0xEE,
                                            0xee, 0x0,  0x0,  0x0,  0x0,  0x0,
                                            0x0,  0x0,  0x0,  0x0};
  static const unsigned char redpal[] = {0x0,  0xa8, 0xd9, 0xda, 0xe1, 0xd4,
                                         0xDA, 0x0,  0xE1, 0x0,  0x0,  0x0,
                                         0x0,  0x0,  0xD4, 0x0};
  static const unsigned char _graypal[] = {0x0,  0x17, 0x10, 0x12, 0x14, 0x1c,
                                           0x12, 0x1c, 0x14, 0x0,  0x0,  0x0,
                                           0x0,  0x0,  0x1C, 0x0};

  // The io objects, the screen and the audio device must outlive the open
  // players.
  MixerVqaAudio movie_audio(engine::audio::TheAudio());
  MixerVqaAudio* const audio =
      !TheDebugState().quiet() && engine::audio::TheAudio().is_open()
          ? &movie_audio
          : nullptr;
  MovieScreen movie_screen;
  GameFileVqaIo gdibrief_io;
  GameFileVqaIo nodbrief_io;
  std::optional<VqaPlayer> gdibrief;
  std::optional<VqaPlayer> nodbrief;
  std::span<const std::byte> speech;
  bool speechplaying = false;
  int setpalette = 0;

  Presentation show;
  int frame = 0;
  int endframe = 255;
  bool lettersdone = false;

  Hide_Mouse();

  Call_Back();

  std::span<std::byte> staticaud;
  if (const auto file = OpenGameFile("STRUGGLE.AUD")) {
    staticaud = Load_Alloc_Data(*file);
  }
  std::span<std::byte> speechg;
  if (const auto file = OpenGameFile("GDI_SLCT.AUD")) {
    speechg = Load_Alloc_Data(*file);
  }
  std::span<std::byte> speechn;
  if (const auto file = OpenGameFile("NOD_SLCT.AUD")) {
    speechn = Load_Alloc_Data(*file);
  }

  //	staticaud = MixArchive::RetrieveData("STRUGGLE.AUD");
  //	speechg = MixArchive::RetrieveData("GDI_SLCT.AUD");
  //	speechn = MixArchive::RetrieveData("NOD_SLCT.AUD");

  if (TheSpecial().IsFromInstall) {
    {
      TheScreen().visible_page().view().Clear();
      TheGameState().preserve_movie_screen() = true;
      Play_Movie("INTRO2", THEME_NONE, false);
    }
    TheGameState().breakout_allowed() = true;
  }

  WsaAnimation anim("CHOOSE.WSA", ThePalettes().title_palette());
  Call_Back();

  nodbrief = Open_Movie(nodbrief_io, movie_screen, audio, "NOD1PRE.VQA");
  Call_Back();
  gdibrief = Open_Movie(gdibrief_io, movie_screen, audio, "GDI1.VQA");

  TheMouse()->Erase_Mouse(&TheScreen().hidden_view(), true);
  TheScreen().hidden_page().view().Clear();
  show.page().view().Clear();
  TheScreen().sys_mem_page().view().Clear();
  // if (!Special.IsFromInstall) {
  TheScreen().visible_page().view().Clear();
  Set_Palette(ThePalettes().title_palette());
  //} else {
  // setpalette = 1;
  //}

  int statichandle = engine::audio::TheAudio().Play(staticaud, 255, 64);
  CountDownTimerClass sample_timer;
  sample_timer.Set(0x3f);
  Alloc_Object(new ScorePrintClass(show, TXT_GDI_NAME, 0, 180, yellowpal));
#ifdef FRENCH
  Alloc_Object(new ScorePrintClass(show, TXT_GDI_NAME2, 0, 187, yellowpal));
#endif
  Alloc_Object(new ScorePrintClass(show, TXT_NOD_NAME, 180, 180, redpal));

#ifdef GERMAN
  Alloc_Object(new ScorePrintClass(show, TXT_SEL_TRANS, 57, 190, _graypal));
#else
#ifdef FRENCH
  Alloc_Object(new ScorePrintClass(show, TXT_SEL_TRANS, 103, 194, _graypal));
#else
  Alloc_Object(new ScorePrintClass(show, TXT_SEL_TRANS, 103, 190, _graypal));
#endif
#endif
  TheKeyboard().Clear();

  while (Get_Mouse_State()) {
    Show_Mouse();
  }

  while (
      endframe != frame ||
      (speechplaying && engine::audio::TheAudio().IsPlaying(speech.data()))) {
    anim.DrawFrame(TheScreen().sys_mem_page().view(), frame++);
    if (setpalette) {
      engine::window::TheDisplay().EndFrame();
      Set_Palette(ThePalettes().title_palette());
      setpalette = 0;
    }
    TheScreen().sys_mem_page().view().BlitTo(show.page().view(), 0, 22, 0, 22,
                                             320, 156);

    /*
    ** If the sample has stopped or is about to then restart it
    */
    if (!engine::audio::TheAudio().IsPlaying(staticaud.data()) ||
        !sample_timer.Time()) {
      engine::audio::TheAudio().Stop(statichandle);
      statichandle = engine::audio::TheAudio().Play(staticaud, 255, 64);
      sample_timer.Set(0x3f);
    }
    Call_Back_Delay(show, 3);  // delay only if haven't clicked

    /* keep the mouse hidden until the letters are thru printing */
    if (!lettersdone) {
      lettersdone = true;
      for (auto& ScoreObj : ScoreObjs) {
        if (ScoreObj) {
          lettersdone = false;
        }
      }
      if (lettersdone) {
        Show_Mouse();
      }
    }
    if (frame >= anim.frame_count()) {
      frame = 0;
    }
    if ((TheKeyboard().Peek() && endframe == 255) &&
        engine::window::KeyCode(TheKeyboard().Read()) ==
            engine::window::KN_LMOUSE &&
        (TheKeyboard().click_y() > 96 && TheKeyboard().click_y() < 300)) {
      if (TheKeyboard().click_x() > 36 && TheKeyboard().click_x() < 296) {
        // Chose GDI
        TheWorld().whom() = HOUSE_GOOD;
        TheWorld().scen_player() = SCEN_PLAYER_GDI;
        endframe = 0;
        engine::audio::TheAudio().Play(speechg);
        speechplaying = true;
        speech = speechg;

      } else if (TheKeyboard().click_x() > 320 &&
                 TheKeyboard().click_x() < 600) {
        // Chose Nod
        endframe = 14;
        TheWorld().whom() = HOUSE_BAD;
        TheWorld().scen_player() = SCEN_PLAYER_NOD;
        engine::audio::TheAudio().Play(speechn);
        speechplaying = true;
        speech = speechn;
      }
    }
  }

  Hide_Mouse();
  anim.Close();

  // erase the "choose side" text
  show.page().view().FillRect(0, 180, 319, 199, 0);
  TheScreen().visible_view().FillRect(0, 180 * 2, 319 * 2, 199 * 2, 0);
  Interpolate_2X_Scale(&show.page(), &TheScreen().visible_view(), "SIDES.PAL");
  TheScreen().sys_mem_page().view().Clear();

  TheKeyboard().Clear();

  /*
  ** Skip the briefings if we're in special mode.
  */
  if (TheSpecial().IsJurassic && TheGameState().thingies_enabled()) {
    nodbrief.reset();
    gdibrief.reset();
  }

  /* play the scenario 1 briefing movie, closing the other side's */
  std::optional<VqaPlayer>& briefing =
      TheWorld().whom() == HOUSE_GOOD ? gdibrief : nodbrief;
  std::optional<VqaPlayer>& other =
      TheWorld().whom() == HOUSE_GOOD ? nodbrief : gdibrief;
  other.reset();
  if (briefing.has_value()) {
    briefing->Run();
    briefing.reset();
  }

  /* get rid of all the animating objects */
  for (auto& ScoreObj : ScoreObjs) {
    if (ScoreObj) {
      delete ScoreObj;
      ScoreObj = nullptr;
    }
  }

  if (TheWorld().whom() == HOUSE_GOOD) {
    /*
    ** Make sure the screen's fully clear after the movie plays
    */
    TheScreen().visible_page().view().Clear();
    std::ranges::fill(ThePalettes().black_palette(), 0x01);
    Set_Palette(ThePalettes().black_palette());
    std::ranges::fill(ThePalettes().black_palette(), 0x00);
  } else {
    TheGameState().preserve_movie_screen() = true;
  }
  engine::audio::TheAudio().Stop(statichandle);
  delete[] base::CharBytes(std::span(staticaud)).data();
  delete[] base::CharBytes(std::span(speechg)).data();
  delete[] base::CharBytes(std::span(speechn)).data();
}
#endif
