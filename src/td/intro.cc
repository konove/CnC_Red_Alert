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
#include <span>

#include "port/bytes_of.h"
#include "sdllib/font.h"
#include "sdllib/gbuffer.h"
#include "sdllib/keyboard.h"
#include "sdllib/misc.h"
#include "sdllib/timer.h"
#include "sdllib/ww_mouse.h"
#include "td/conquer.h"
#include "td/defines.h"
#include "td/externs.h"
#include "td/globals.h"
#include "td/interpal.h"
#include "td/jshell.h"
#include "td/palette.h"
#include "td/score.h"
#include "td/special.h"
#include "td/textblit.h"
#include "tech/game_file.h"
#include "tech/game_file_vqa_io.h"
#include "tech/wsa_animation.h"
#include "tech/ww_audio.h"
#include "winvq/vqa32/vqaplay.h"

#ifndef DEMO

// Opens a movie on the given player without playing it. The io object must
// stay alive until the player is closed. Returns true if the movie opened.
static bool Open_Movie(VqaPlayer& player, GameFileVqaIo& io, const char* name) {
  if (!Debug_Quiet && Audio.is_open()) {
    AnimControl.OptionFlags |= VQAOPTF_AUDIO;
  } else {
    AnimControl.OptionFlags &= ~VQAOPTF_AUDIO;
  }

  player.SetIo(&io);
  return player.Open(name, &AnimControl) == 0;
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

  VqaPlayer gdibrief_player;
  VqaPlayer nodbrief_player;
  GameFileVqaIo gdibrief_io;
  GameFileVqaIo nodbrief_io;  // Must outlive the open players.
  bool gdibrief = false;
  bool nodbrief = false;  // Movie opened successfully?
  std::span<const std::byte> speech;
  bool speechplaying = false;
  const int oldfontxspacing = FontXSpacing;
  int setpalette = 0;

  TextPrintBuffer =
      new GraphicBufferClass(SeenBuff.Get_Width(), SeenBuff.Get_Height(), {});
  TextPrintBuffer->Clear();
  BlitList.Clear();
  PseudoSeenBuff = new GraphicBufferClass(320, 200, {});
  int frame = 0;
  int endframe = 255;
  bool lettersdone = false;

  Hide_Mouse();
  /* Change to the six-point font for Text_Print */
  const std::span<const std::byte> oldfont = Set_Font(ScoreFontPtr);

  Call_Back();

  GameFile f("STRUGGLE.AUD");
  const auto staticaud = Load_Alloc_Data(f);
  f.Open("GDI_SLCT.AUD");
  const auto speechg = Load_Alloc_Data(f);
  f.Open("NOD_SLCT.AUD");
  const auto speechn = Load_Alloc_Data(f);

  //	staticaud = MixArchive::RetrieveData("STRUGGLE.AUD");
  //	speechg = MixArchive::RetrieveData("GDI_SLCT.AUD");
  //	speechn = MixArchive::RetrieveData("NOD_SLCT.AUD");

  if (Special.IsFromInstall) {
    {
      VisiblePage.Clear();
      PreserveVQAScreen = true;
      Play_Movie("INTRO2", THEME_NONE, false);
    }
    BreakoutAllowed = true;
  }

  WsaAnimation anim("CHOOSE.WSA", Palette);
  Call_Back();

  InterpolationPaletteChanged = true;
  InterpolationPalette = Palette;
  Read_Interpolation_Palette("SIDES.PAL");

  nodbrief = Open_Movie(nodbrief_player, nodbrief_io, "NOD1PRE.VQA");
  const int gdi_start_palette = Load_Interpolated_Palettes("NOD1PRE.VQP");
  Call_Back();
  gdibrief = Open_Movie(gdibrief_player, gdibrief_io, "GDI1.VQA");
  Load_Interpolated_Palettes("GDI1.VQP", true);

  WWMouse->Erase_Mouse(&HidPage, true);
  HiddenPage.Clear();
  PseudoSeenBuff->Clear();
  SysMemPage.Clear();
  // if (!Special.IsFromInstall) {
  VisiblePage.Clear();
  Set_Palette(Palette);
  //} else {
  // setpalette = 1;
  //}

  int statichandle = Audio.Play(staticaud, 255, 64);
  CountDownTimerClass sample_timer;
  sample_timer.Set(0x3f);
  Alloc_Object(new ScorePrintClass(TXT_GDI_NAME, 0, 180, yellowpal));
#ifdef FRENCH
  Alloc_Object(new ScorePrintClass(TXT_GDI_NAME2, 0, 187, yellowpal));
#endif
  Alloc_Object(new ScorePrintClass(TXT_NOD_NAME, 180, 180, redpal));

#ifdef GERMAN
  Alloc_Object(new ScorePrintClass(TXT_SEL_TRANS, 57, 190, _graypal));
#else
#ifdef FRENCH
  Alloc_Object(new ScorePrintClass(TXT_SEL_TRANS, 103, 194, _graypal));
#else
  Alloc_Object(new ScorePrintClass(TXT_SEL_TRANS, 103, 190, _graypal));
#endif
#endif
  Keyboard::Clear();

  while (Get_Mouse_State()) {
    Show_Mouse();
  }

  while (endframe != frame ||
         (speechplaying && Audio.IsPlaying(speech.data()))) {
    anim.DrawFrame(SysMemPage, frame++);
    if (setpalette) {
      Wait_Vert_Blank();
      Set_Palette(Palette);
      setpalette = 0;
    }
    SysMemPage.Blit(*PseudoSeenBuff, 0, 22, 0, 22, 320, 156);

    /*
    ** If the sample has stopped or is about to then restart it
    */
    if (!Audio.IsPlaying(staticaud.data()) || !sample_timer.Time()) {
      Audio.Stop(statichandle);
      statichandle = Audio.Play(staticaud, 255, 64);
      sample_timer.Set(0x3f);
    }
    Call_Back_Delay(3);  // delay only if haven't clicked

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
    if ((Keyboard::Check() && endframe == 255) &&
        KeyCode(Keyboard::Get()) == KN_LMOUSE &&
        (ActiveKeyboard->MouseQY > 96 && ActiveKeyboard->MouseQY < 300)) {
      if (ActiveKeyboard->MouseQX > 36 && ActiveKeyboard->MouseQX < 296) {
        // Chose GDI
        Whom = HOUSE_GOOD;
        ScenPlayer = SCEN_PLAYER_GDI;
        endframe = 0;
        Audio.Play(speechg);
        speechplaying = true;
        speech = speechg;

      } else if (ActiveKeyboard->MouseQX > 320 &&
                 ActiveKeyboard->MouseQX < 600) {
        // Chose Nod
        endframe = 14;
        Whom = HOUSE_BAD;
        ScenPlayer = SCEN_PLAYER_NOD;
        Audio.Play(speechn);
        speechplaying = true;
        speech = speechn;
      }
    }
  }

  Hide_Mouse();
  anim.Close();

  // erase the "choose side" text
  PseudoSeenBuff->Fill_Rect(0, 180, 319, 199, 0);
  SeenBuff.Fill_Rect(0, 180 * 2, 319 * 2, 199 * 2, 0);
  Interpolate_2X_Scale(PseudoSeenBuff, &SeenBuff, "SIDES.PAL");
  SysMemPage.Clear();

  Keyboard::Clear();

  /*
  ** Skip the briefings if we're in special mode.
  */
  if (Special.IsJurassic && AreThingiesEnabled) {
    if (nodbrief) {
      nodbrief_player.Close();
      nodbrief = false;
    }
    if (gdibrief) {
      gdibrief_player.Close();
      gdibrief = false;
    }
  }

  /* play the scenario 1 briefing movie */
  if (Whom == HOUSE_GOOD) {
    if (nodbrief) {
      nodbrief_player.Close();
    }
    if (gdibrief) {
      PaletteCounter = gdi_start_palette;
      gdibrief_player.Play(VQAMODE_RUN);
      gdibrief_player.Close();
    }
  } else {
    if (gdibrief) {
      gdibrief_player.Close();
    }
    if (nodbrief) {
      nodbrief_player.Play(VQAMODE_RUN);
      nodbrief_player.Close();
    }
  }

  Free_Interpolated_Palettes();
  /* get rid of all the animating objects */
  for (auto& ScoreObj : ScoreObjs) {
    if (ScoreObj) {
      delete ScoreObj;
      ScoreObj = nullptr;
    }
  }

  if (Whom == HOUSE_GOOD) {
    /*
    ** Make sure the screen's fully clear after the movie plays
    */
    VisiblePage.Clear();
    std::ranges::fill(BlackPalette, 0x01);
    Set_Palette(BlackPalette);
    std::ranges::fill(BlackPalette, 0x00);
  } else {
    PreserveVQAScreen = true;
  }
  Audio.Stop(statichandle);
  delete[] port::CharBytes(std::span(staticaud)).data();
  delete[] port::CharBytes(std::span(speechg)).data();
  delete[] port::CharBytes(std::span(speechn)).data();

  Set_Font(oldfont);
  FontXSpacing = oldfontxspacing;

  delete PseudoSeenBuff;
  PseudoSeenBuff = nullptr;
  delete TextPrintBuffer;
  TextPrintBuffer = nullptr;
  BlitList.Clear();
}
#endif
