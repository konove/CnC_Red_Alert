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
#include <string>
#include <tuple>
#include <utility>

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
#include "ra/score.h"
#include "ra/theme.h"
#include "ra/type.h"
#include "sdllib/gbuffer.h"
#include "sdllib/keyboard.h"
#include "sdllib/misc.h"
#include "sdllib/shape.h"
#include "sdllib/wsa.h"
#include "sdllib/ww_audio.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/wwstd.h"
#include "tech/ftimer.h"
#include "tech/rgb.h"

// Scenario filenames for the secret ant missions (Easter egg campaign).
// Index 0 is unused; missions are numbered 1-4.
const char* ant_missions[] = {nullptr, "SCA01EA.INI", "SCA02EA.INI",
                              "SCA03EA.INI", "SCA04EA.INI"};

// Scenario variant suffixes. Each mission can have up to 3 variants (A/B/C)
// representing different map layouts or objectives for the same mission number.
constexpr char kScenarioVariants[] = "ABC";

// Clickable hotspot coordinates for each scenario's mission choices.
// Dimensions: [house: Allied=0/Soviet=1][scenario: 0-13][choice: 0-2]
// Coordinates are in 320x200 logical pixels. {-1, -1} marks unused slots.
struct point {
  int x;
  int y;
} const MapCoords[2][14][3] = {{{{185, 123}, {-1, -1}, {-1, -1}},
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

// Animates the pulsing highlight on clickable map locations.
// Palette entry 254 cycles between dim and bright to draw attention.
// TODO(konove): Seems to be not working.
void Cycle_Call_Back_Delay(int time, PaletteClass& pal) {
  static Timer<SystemTickSource> _ftimer;
  static bool _up = false;
  static int val = 255;

  while (time--) {
    if (_ftimer.Value() > 0) {
      _ftimer.Set(TIMER_SECOND / 6);

      // Oscillate brightness between 0x20 (dim) and 150 (bright).
      constexpr int kStepRate = 20;
      if (_up) {
        val += kStepRate;
        if (val > 150) {
          val = 150;
          _up = false;
        }
      } else {
        val -= kStepRate;
        if (val < 0x20) {
          val = 0x20;
          _up = true;
        }
      }

      // Blend white toward black based on current brightness level.
      pal[254] = GamePalette[WHITE];
      pal[254].Adjust(val, kBlackColor);

      pal.Set();
    }
    Call_Back_Delay(/*time=*/1);
  }
}

// Returns which mission choice (0-2) the mouse is hovering over, or -1 if none.
// Each hotspot is a 12x10 pixel rectangle at the coordinates in MapCoords.
int Mouse_Over_Spot(const int is_soviet, const int scenario) {
  int retval = -1;
  for (int choice = 0;
       choice < 3 && MapCoords[is_soviet][scenario][choice].x != -1; choice++) {
    const int mouse_x = Get_Mouse_X() / 2;
    const int mouse_y = Get_Mouse_Y() / 2;
    if (mouse_x >= MapCoords[is_soviet][scenario][choice].x &&
        mouse_y >= MapCoords[is_soviet][scenario][choice].y &&
        mouse_x <= MapCoords[is_soviet][scenario][choice].x + 11 &&
        mouse_y <= MapCoords[is_soviet][scenario][choice].y + 9) {
      retval = choice;
      break;
    }
  }
  return retval;
}

std::string Map_Selection() {
  if (AntsEnabled) {
    std::string scenario_name = Scen.ScenarioName;
    scenario_name.replace(3, 2, std::format("{:02d}", Scen.Scenario + 1));
    return scenario_name;
  }

  // Build map selection animation filename. Format: MSxY.WSA where
  // MS=Map Selection, x=side (A=Allied, S=Soviet), Y=scenario letter (A-N for
  // scenarios 0-13). WSA = Westwood Studios Animation format.
  std::string file_name = "MSAA.WSA";
  const int is_soviet = PlayerPtr->Class->House == HOUSE_USSR ||
                        PlayerPtr->Class->House == HOUSE_UKRAINE;

  file_name[2] = is_soviet ? 'S' : 'A';
  file_name[3] = Scen.Scenario + 'A';
  PaletteClass map_palette;

  int selection = 0;
  static Timer<SystemTickSource> timer;

  const void* appear1 = MFCD::Retrieve("MAPWIPE2.AUD");
  const void* bleep11 = MFCD::Retrieve("BLEEP11.AUD");
  const void* country4 = MFCD::Retrieve("MAPWIPE5.AUD");
  const void* toney7 = MFCD::Retrieve("TONEY7.AUD");
  const void* bleep17 = MFCD::Retrieve("BLEEP17.AUD");

  const void* scold1 = MFCD::Retrieve("TONEY4.AUD");
  const void* country1 = MFCD::Retrieve("TONEY10.AUD");

  auto* pseudo_seen_buf =
      new GraphicBufferClass(320, 200, static_cast<void*>(nullptr));

  Theme.Queue_Song(THEME_MAP);

  void* anim = Open_Animation(
      file_name.c_str(), /*user_buffer=*/nullptr, /*user_buffer_size=*/0L,
      WSA_OPEN_FROM_MEM | WSA_OPEN_TO_PAGE, map_palette);

  Keyboard->Clear();
  SeenBuff.Clear();
  map_palette.Set(FADE_PALETTE_FAST, Call_Back);

  pseudo_seen_buf->Clear();
  Animate_Frame(anim, *pseudo_seen_buf, /*frame_number=*/1);
  // Initialize palette interpolation as identity mapping (no blending).
  // Each row x maps all 256 entries to color x.
  for (int x = 0; x < 256; x++) {
    memset(&PaletteInterpolationTable[x][0], x, 256);
  }
  Interpolate_2X_Scale(pseudo_seen_buf, &SeenBuff, nullptr);

  // Play the map reveal animation with synchronized sound effects.
  StreamLowImpact = true;
  Play_Sample(appear1, 255, Options.Normalize_Volume(170));
  for (int frame = 1; frame < Get_Animation_Frame_Count(anim); frame++) {
    Animate_Frame(anim, *pseudo_seen_buf, frame);
    Interpolate_2X_Scale(pseudo_seen_buf, &SeenBuff, nullptr);
    Call_Back_Delay(/*time=*/2);
    // Sound effects timed to specific animation frames.
    switch (frame) {
      case 15:
        Play_Sample(bleep11, 255, Options.Normalize_Volume(170));
        break;
      case 29:
        Play_Sample(country4, 255, Options.Normalize_Volume(170));
        break;
      case 50:
        Play_Sample(toney7, 255, Options.Normalize_Volume(170));
        break;
      case 60:
        Play_Sample(bleep17, 255, Options.Normalize_Volume(170));
        break;
      default:
        break;
    }
  }
  StreamLowImpact = false;
  Call_Back();
  Close_Animation(anim);
  Show_Mouse();
  Keyboard->Clear();

  bool mission_selected = false;
  int cursor_frame = 0;
  while (!mission_selected) {
    // Redraw after regaining window focus (surfaces may have been
    // invalidated while the game was in the background).
    if (AllSurfaces.SurfacesRestored) {
      AllSurfaces.SurfacesRestored = false;
      Interpolate_2X_Scale(pseudo_seen_buf, &SeenBuff, nullptr);
    }
    Cycle_Call_Back_Delay(1, map_palette);
    const int choice = Mouse_Over_Spot(is_soviet, Scen.Scenario);
    const bool hovering_over_choice = choice != -1;

    // Cursor animation parameters: normal cursor is static, targeting cursor
    // has 8 frames. Hotspot is top-left for normal, centered for crosshairs.
    const auto [start, count, delay] =
        hovering_over_choice ? std::tuple{21, 8, 4} : std::tuple{0, 1, 0};
    const auto [hotspot_x, hotspot_y] =
        hovering_over_choice ? std::pair{14, 11} : std::pair{0, 0};
    if (timer.IsFinished()) {
      cursor_frame++;
      cursor_frame %= count;
      timer.Set(delay);
      Set_Mouse_Cursor(
          hotspot_x, hotspot_y,
          Extract_Shape(MouseClass::MouseShapes, start + cursor_frame));
    }
    if (Keyboard->Check()) {
      if ((Keyboard->Get() & 0x10FF) == KN_LMOUSE) {
        if (hovering_over_choice) {
          mission_selected = true;
          selection = choice;
          Play_Sample(country1, 255, Options.Normalize_Volume(170));
        } else {
          Play_Sample(scold1, 255, Options.Normalize_Volume(170));
        }
      }
    }
  }

  Hide_Mouse();

  // Restore normal cursor before returning.
  Set_Mouse_Cursor(0, 0, Extract_Shape(MouseClass::MouseShapes, 0));

  Keyboard->Clear();

  Fancy_Text_Print(TXT_STAND_BY, 320, 380,
                   GadgetClass::Get_Color_Scheme(), TBLACK,
                   TPF_CENTER | TPF_6PT_GRAD | TPF_DROPSHADOW);

  // Build the next scenario filename. Format: SCxNNEV.INI where x=campaign,
  // NN=scenario number (01-14), E=side (E=Allied, usually), V=variant (A/B/C).
  // Ant missions (ScenarioName[2]=='A') use a separate filename table.
  std::string scenario_name;
  if (Scen.ScenarioName[2] == 'A') {
    int antnum = Scen.Scenario++;
    if (antnum > 4) {
      antnum = 1;
    }
    scenario_name = ant_missions[antnum];
  } else {
    scenario_name = Scen.ScenarioName;
    scenario_name.replace(3, 2, std::format("{:02d}", Scen.Scenario + 1));
    scenario_name[6] = kScenarioVariants[selection];
  }
  Theme.Fade_Out();

  return scenario_name;
}
