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

// Map selection screen for campaign progression.
//
// Between missions, players choose their next objective by clicking on one of
// several highlighted locations on a map of Europe. Each scenario offers 1-3
// possible next missions (branching campaign paths). The Allied and Soviet
// campaigns use different map coordinates.

#include "ra/mapsel.h"

#include <cstring>
#include <format>
#include <span>
#include <string>
#include <string_view>

#include "base/array.h"
#include "ra/ccptr.h"
#include "ra/conquer.h"
#include "ra/defines.h"
#include "ra/dialog.h"
#include "ra/externs.h"
#include "ra/gadget.h"
#include "ra/globals.h"
#include "ra/goptions.h"
#include "ra/house.h"
#include "ra/interpal.h"
#include "ra/jshell.h"
#include "ra/mouse.h"
#include "ra/palette.h"
#include "ra/scenario.h"
#include "ra/text_ids.h"
#include "ra/theme.h"
#include "ra/type.h"
#include "sdllib/gbuffer.h"
#include "sdllib/keyboard.h"
#include "sdllib/shape.h"
#include "sdllib/ww_audio.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/wwstd.h"
#include "tech/ftimer.h"
#include "tech/glow_pulse.h"
#include "tech/mix_archive.h"
#include "tech/rgb.h"
#include "tech/wsa_animation.h"

// The scenario variant behind each hotspot, in the order of kHotspotCorners.
constexpr ScenarioVarType kChoiceVariants[] = {SCEN_VAR_A, SCEN_VAR_B,
                                               SCEN_VAR_C};

// A position on the 320x200 map artwork.
struct Point {
  int x;
  int y;
};

// Top-left corner of the clickable hotspot for each scenario's mission choices.
// Dimensions: [house: Allied=0/Soviet=1][scenario: 0-13][choice: 0-2]
// {-1, -1} marks unused slots.
constexpr Point kHotspotCorners[2][14][3] = {
    {{{185, 123}, {-1, -1}, {-1, -1}},
     {{173, 112}, {-1, -1}, {-1, -1}},
     {{196, 100}, {200, 112}, {-1, -1}},
     {{175, 113}, {-1, -1}, {-1, -1}},
     {{187, 91}, {202, 93}, {206, 105}},
     {{207, 161}, {212, 172}, {-1, -1}},
     {{172, 92}, {-1, -1}, {-1, -1}},
     {{132, 119}, {146, 125}, {-1, -1}},
     {{199, 73}, {205, 86}, {-1, -1}},
     {{236, 114}, {-1, -1}, {-1, -1}},
     {{219, 64}, {225, 76}, {-1, -1}},
     {{256, 69}, {-1, -1}, {-1, -1}},
     {{262, 77}, {-1, -1}, {-1, -1}},
     {{249, 97}, {-1, -1}, {-1, -1}}},
    // Soviet coords
    {{{178, 105}, {-1, -1}, {-1, -1}},
     {{163, 101}, {163, 113}, {-1, -1}},
     {{160, 89}, {-1, -1}, {-1, -1}},
     {{142, 101}, {142, 117}, {-1, -1}},
     {{212, 163}, {-1, -1}, {-1, -1}},
     {{155, 133}, {171, 144}, {-1, -1}},
     {{216, 103}, {-1, -1}, {-1, -1}},
     {{132, 145}, {154, 154}, {-1, -1}},
     {{122, 117}, {-1, -1}, {-1, -1}},
     {{117, 130}, {-1, -1}, {-1, -1}},
     {{99, 107}, {109, 146}, {-1, -1}},
     {{134, 125}, {-1, -1}, {-1, -1}},
     {{32, 156}, {46, 171}, {-1, -1}},
     {{108, 97}, {-1, -1}, {-1, -1}}}};

// Palette index reserved in the map WSA artwork for clickable hotspots.
constexpr int kHotspotPaletteIndex = 254;

// Keeps the highlight on the clickable map locations pulsing in `palette`.
// Call it every pass of the selection loop.
static void PulseHotspots(PaletteClass& palette) {
  static GlowPulse<SystemTickSource> pulse(kTimerSecond / 6);

  if (pulse.Update()) {
    palette.at(kHotspotPaletteIndex) = pulse.Apply(GamePalette.at(kWhite));
    palette.Set();
  }
}

// Returns which mission choice (0-2) the mouse is hovering over, or -1 if none.
// Each hotspot is a 12x10 pixel rectangle whose top-left corner is in
// kHotspotCorners.
static int ChoiceUnderMouse(const bool is_soviet, const int scenario) {
  const int mouse_x = Get_Mouse_X() / 2;
  const int mouse_y = Get_Mouse_Y() / 2;
  int choice = 0;
  for (const Point& corner :
       base::At(base::At(kHotspotCorners, is_soviet), scenario)) {
    if (corner.x == -1) {
      break;
    }
    if (mouse_x >= corner.x && mouse_y >= corner.y &&
        mouse_x <= corner.x + 11 && mouse_y <= corner.y + 9) {
      return choice;
    }
    choice++;
  }
  return -1;
}

static void PlayMapSound(const std::string_view file_name) {
  PlaySample(MixArchive::RetrieveData(file_name), 255,
             Options.Normalize_Volume(170));
}

// Plays the animation that draws the map, leaving its last frame on screen and
// the animation's colors in `palette`.
static void PlayMapReveal(const std::string& animation_name,
                          PaletteClass& palette) {
  // Sound effects timed to specific animation frames.
  struct SoundCue {
    int frame;
    std::string_view file_name;
  };
  static constexpr SoundCue kSoundCues[] = {{15, "BLEEP11.AUD"},
                                            {29, "MAPWIPE5.AUD"},
                                            {50, "TONEY7.AUD"},
                                            {60, "BLEEP17.AUD"}};

  // The artwork is drawn at this size and scaled up to the screen.
  GraphicBufferClass page(320, 200);
  page.Clear();
  WsaAnimation animation(animation_name, palette);

  Keyboard->Clear();
  SeenBuff.Clear();
  palette.Set(kFadePaletteFast, ServiceRealTime);

  animation.DrawFrame(page, 1);
  Interpolate_2X_Scale(&page, &SeenBuff, {});

  PlayMapSound("MAPWIPE2.AUD");
  // Ctrl-Q, the score screen's skip key, plays the rest without the waits.
  bool skip = false;
  for (int frame = 1; frame < animation.frame_count(); frame++) {
    animation.DrawFrame(page, frame);
    Interpolate_2X_Scale(&page, &SeenBuff, {});
    skip = skip || (KeyboardClass::Down(KN_LCTRL) && KeyboardClass::Down(KN_Q));
    ServiceRealTimeFor(skip ? 0 : 2);
    for (const SoundCue& cue : kSoundCues) {
      if (cue.frame == frame) {
        PlayMapSound(cue.file_name);
      }
    }
  }
  ServiceRealTime();
}

// Waits for the player to click one of the hotspots on the map, which pulse in
// `palette` meanwhile, and returns that choice (0-2).
static int WaitForMissionChoice(PaletteClass& palette, const bool is_soviet) {
  Timer<SystemTickSource> cursor_timer;
  int cursor_frame = 0;
  while (true) {
    PulseHotspots(palette);
    ServiceRealTimeFor(1);

    // MouseClass animates the pointer only while the game map runs, so step
    // through the crosshair's frames here.
    const int choice = ChoiceUnderMouse(is_soviet, Scen.Scenario);
    const MouseClass::MouseStruct& cursor =
        MouseClass::Control(choice != -1 ? MOUSE_CAN_ATTACK : MOUSE_NORMAL);
    if (cursor_timer.IsFinished()) {
      cursor_frame = (cursor_frame + 1) % cursor.FrameCount;
      cursor_timer.Set(cursor.FrameRate);
      Set_Mouse_Cursor(cursor.X, cursor.Y,
                       Extract_Shape(MouseClass::MouseShapes,
                                     cursor.StartFrame + cursor_frame));
    }

    if (Keyboard->Check() && KeyCode(Keyboard->Get()) == KN_LMOUSE) {
      if (choice != -1) {
        PlayMapSound("TONEY10.AUD");
        return choice;
      }
      PlayMapSound("TONEY4.AUD");
    }
  }
}

ScenarioVarType ChooseMissionVariant() {
  const bool is_soviet = IsSovietHouse(PlayerPtr->Class->House);

  // The animation is MSxY.WSA: x is the side (A=Allied, S=Soviet) and Y the
  // scenario letter (A-N for scenarios 0-13).
  const std::string animation_name =
      std::format("MS{}{}.WSA", is_soviet ? 'S' : 'A',
                  static_cast<char>('A' + Scen.Scenario));
  PaletteClass map_palette;

  Theme.Queue_Song(THEME_MAP);
  PlayMapReveal(animation_name, map_palette);
  Show_Mouse();
  Keyboard->Clear();

  const int choice = WaitForMissionChoice(map_palette, is_soviet);

  Hide_Mouse();

  // Restore normal cursor before returning.
  Set_Mouse_Cursor(0, 0, Extract_Shape(MouseClass::MouseShapes, 0));

  Keyboard->Clear();

  Fancy_Text_Print(TXT_STAND_BY, 320, 380, GadgetClass::Get_Color_Scheme(),
                   kTBlack, TPF_CENTER | TPF_6PT_GRAD | TPF_DROPSHADOW);
  Theme.Fade_Out();

  return base::At(kChoiceVariants, choice);
}
