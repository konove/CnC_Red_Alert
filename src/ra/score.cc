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

// Score screens shown at the end of a mission: the single-player rating and
// hall of fame screen (ScoreClass::Presentation) and the multiplayer tally
// (Multi_Score_Presentation).
//
// The screens are drawn at 640x400. Coordinates handed to the ScoreAnimClass
// family and to Count_Up_Print() are 320x200 and get doubled there; direct page
// calls (Print, Blit, Draw_Line, ...) take real pixels.
//
// Originally SCORE.CPP by Joe L. Bostic, started April 19, 1994; the score
// screen routines are by BWG, April-June 1995.

#include "ra/score.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <span>
#include <string_view>

#include "absl/strings/str_format.h"
#include "base/array.h"
#include "base/buffer.h"
#include "base/numeric.h"
#include "ra/ccptr.h"
#include "ra/config.h"
#include "ra/conquer.h"
#include "ra/defines.h"
#include "ra/externs.h"
#include "ra/globals.h"
#include "ra/goptions.h"
#include "ra/graphics_loader.h"
#include "ra/house.h"
#include "ra/inline.h"
#include "ra/interpal.h"
#include "ra/jshell.h"
#include "ra/logic.h"
#include "ra/mapedit.h"
#include "ra/nullmgr.h"
#include "ra/object.h"
#include "ra/palette.h"
#include "ra/scenario.h"
#include "ra/session.h"
#include "ra/shape_draw.h"
#include "ra/text_ids.h"
#include "ra/theme.h"
#include "ra/type.h"
#include "sdllib/drawbuff.h"
#include "sdllib/file_access.h"
#include "sdllib/font.h"
#include "sdllib/gbuffer.h"
#include "sdllib/keyboard.h"
#include "sdllib/shape.h"
#include "sdllib/wsa.h"
#include "sdllib/ww_audio.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/ww_win.h"
#include "sdllib/wwstd.h"
#include "tech/fixed.h"
#include "tech/ftimer.h"
#include "tech/game_file.h"
#include "tech/mix_archive.h"
#include "tech/rgb.h"

// Layout, in 320x200 coordinates.

// Length in pixels of a full Do_GDI_Graph() bar. Frame N of a bar shape file is
// a bar N long; frame SIZEGBAR + 1 is the white flash.
#define SIZEGBAR 118
// Top-left corner of the first hall of fame row.
#define HALLFAME_X 11
#define HALLFAME_Y 120

// Rows in the hall of fame.
#define NUMFAMENAMES 7
// Size of a hall of fame name: ten letters and the terminator. Part of the
// HALLFAME.DAT record layout (see Fame).
#define MAX_FAMENAME_LENGTH 11

// Draws the blinking underline cursor beneath letter `pos` of the hall of fame
// name being typed on 320x200 row `ypos`, erasing the old one if `pos` moved.
static void Animate_Cursor(int pos, int ypos);

// Gives every object in ScoreObjs[] one Update(). Objects may delete themselves
// and free their slot while this runs.
static void Animate_Score_Objs();

// Waits for a key, a click or Ctrl-Q. Input during the first 20 ticks is
// discarded so a stray click left over from the mission cannot dismiss the
// screen. With `cycle` set (and palette scrolling enabled in the options) it
// also colour-cycles ScorePalette entries 233..237 while waiting. In modem
// games it keeps answering the other machine's timing packets so the link
// does not time out.
static void Cycle_Wait_Click(bool cycle = true);

// The count-up tick sound; loaded by Presentation().
static std::span<const std::byte> Beepy6;
// Ctrl-Q cheat key to skip past the score and map selection screens: once
// Call_Back_Delay() has seen it, every later delay (the map selection's
// included) is zero and Cycle_Wait_Click() returns at once.
static bool ControlQ;
// True while some ScorePrintClass is still typing. Animate_Score_Objs() clears
// it and the unfinished printers set it again, so it is valid after any
// Call_Back_Delay().
static bool StillUpdating;

// Score screen backgrounds, indexed by side: 0 Allied, 1 Soviet.
static const char* ScreenNames[2] = {"ALIBACKH.PCX", "SOVBACKH.PCX"};

// One hall of fame row. HALLFAME.DAT is NUMFAMENAMES of these written as raw
// memory, so the struct layout (padding included) is the file format.
struct Fame {
  char name[MAX_FAMENAME_LENGTH];
  int score;  // 0 marks an empty row.
  int level;  // Scenario number the score was earned on.
  int side;   // 0 Allied, 1 Soviet.
};

ScoreAnimClass* ScoreObjs[MAXSCOREOBJS];

ScoreAnimClass::ScoreAnimClass(int x, int y, std::span<const std::byte> data)
    : XPos(x * 2), YPos(y * 2), DataPtr(data) {
  AnimTimer.Set(0);
}

ScoreAnimClass::ScoreAnimClass(int x, int y, std::string_view text)
    : XPos(x * 2), YPos(y * 2), TextData(text) {
  AnimTimer.Set(0);
}

ScoreTimeClass::ScoreTimeClass(int xpos, int ypos,
                               std::span<const std::byte> data, int maxval,
                               int xtimer)
    : ScoreAnimClass(xpos, ypos, data), MaxStage(maxval), TimerReset(xtimer) {}

void ScoreTimeClass::Update() {
  if (AnimTimer.IsFinished()) {
    AnimTimer.Set(TimerReset);
    if (++Stage >= MaxStage) {
      Stage = 0;
    }
    GraphicViewPortClass* oldpage = LogicPage;
    Set_Logic_Page(SeenBuff);
    CC_Draw_Shape(DataPtr, Stage, XPos, YPos, WINDOW_MAIN, SHAPE_WIN_REL, {},
                  {});
    Set_Logic_Page(oldpage);
  }
}

ScoreCredsClass::ScoreCredsClass(int xpos, int ypos,
                                 std::span<const std::byte> data, int maxval,
                                 int xtimer)
    : ScoreAnimClass(xpos, ypos, data),
      MaxStage(maxval),
      TimerReset(xtimer),
      Clock1(MixArchive::RetrieveData("CLOCK1.AUD")) {}

void ScoreCredsClass::Update() {
  if (AnimTimer.IsFinished()) {
    AnimTimer.Set(TimerReset);
    if (++Stage >= MaxStage) {
      Stage = 0;
    }
    GraphicViewPortClass* oldpage = LogicPage;
    Set_Logic_Page(SeenBuff);
    // One tick of sound per frame of the spinning credits symbol.
    Play_Sample(Clock1, 255, Options.Normalize_Volume(130));
    CC_Draw_Shape(DataPtr, Stage, XPos, YPos, WINDOW_MAIN, SHAPE_WIN_REL, {},
                  {});
    Set_Logic_Page(oldpage);
  }
}

ScorePrintClass::ScorePrintClass(int string, int xpos, int ypos,
                                 std::span<const uint8_t> palette)
    : ScoreAnimClass(xpos, ypos, Text_String(string)),
      Stage(0),
      PrimaryPalette(palette) {}

ScorePrintClass::ScorePrintClass(std::string_view string, int xpos, int ypos,
                                 std::span<const uint8_t> palette)
    : ScoreAnimClass(xpos, ypos, string), Stage(0), PrimaryPalette(palette) {}

void ScorePrintClass::Update() {
  // A one-letter string for Print().
  static char localstr[2] = {0, 0};
  // Font palette with every entry white (colour 15), for the leading letter.
  static const uint8_t _whitepal[] = {0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F,
                                      0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F,
                                      0x0F, 0x0F, 0x0F, 0x0F};

  // Letter Stage - 1 is the last one that still needs its clean reprint. Once
  // that is past the end the text is complete: free the slot and self-destruct.
  if (Stage && base::ToSize(Stage - 1) >= Text().size()) {
    for (auto& ScoreObj : ScoreObjs) {
      if (ScoreObj == this) {
        ScoreObj = nullptr;
      }
    }
    delete this;
    return;
  }

  StillUpdating = true;
  if (AnimTimer.IsFinished()) {
    AnimTimer.Set(1);

    // The score font is 12 pixels per letter at this resolution.
    const int pos = XPos + (Stage * 12);
    // Reprint the previous letter properly. It goes to the hidden page first
    // and is blitted across, 14x16 from two rows up, so the blit also wipes the
    // white smear drawn around it on the last tick.
    if (Stage) {
      Set_Font_Palette(PrimaryPalette);
      localstr[0] = Text().at(base::ToSize(Stage - 1));
      HidPage.Print(localstr, pos - 12, YPos, kTBlack, kTBlack);
      HidPage.Blit(SeenBuff, pos - 12, YPos - 2, pos - 12, YPos - 2, 14, 16);
    }
    // Smear the next letter in white, straight onto the visible page: one row
    // up, one row down and one pixel right of where it will finally sit.
    if (base::ToSize(Stage) < Text().size()) {
      localstr[0] = Text().at(base::ToSize(Stage));
      Set_Font_Palette(_whitepal);
      SeenBuff.Print(localstr, pos, YPos - 1, kTBlack, kTBlack);
      SeenBuff.Print(localstr, pos, YPos + 1, kTBlack, kTBlack);
      SeenBuff.Print(localstr, pos + 1, YPos, kTBlack, kTBlack);
    }
    Stage++;
  }
}

ScoreScaleClass::ScoreScaleClass(std::string_view string, int xpos, int ypos,
                                 std::span<const uint8_t> palette)
    : ScoreAnimClass(xpos, ypos, string), Palette(palette) {}

void ScoreScaleClass::Update() {
  // Left edge and size (320x200) of the zooming letter at each Stage, 5 being
  // the largest and first. Entry 0 is unused: Stage 0 prints the final letter.
  static const int _destx[] = {0, 80, 107, 134, 180, 228};
  static const int _destw[] = {6, 20, 30, 40, 60, 80};

  // The DOS code restored the background under the previous, larger letter here
  // before drawing the next zoom step. The Windows build compiled that out, and
  // since Stage now starts at 0 (see score.h) the zoom branch below never runs.
  if (AnimTimer.IsFinished()) {
    AnimTimer.Set(1);
    if (Stage) {
      Set_Font_Palette(Palette);
      HidPage.Fill_Rect(0, 0, 14, 14, kTBlack);
      HidPage.Print(std::string(Text()).c_str(), 0, 0, kTBlack, kTBlack);
      HidPage.Scale(SeenBuff, 0, 0, base::At(_destx, Stage) * 2, YPos, 10, 12,
                    base::At(_destw, Stage) * 2, base::At(_destw, Stage) * 2,
                    true);
      Stage--;
    } else {
      // Zoom finished: print the letter at its final size, free the slot (which
      // Input_Name() is waiting on) and self-destruct.
      Set_Font_Palette(Palette);
      for (auto& ScoreObj : ScoreObjs) {
        if (ScoreObj == this) {
          ScoreObj = nullptr;
        }
      }
      HidPage.Print(std::string(Text()).c_str(), XPos, YPos, kTBlack, kTBlack);
      HidPage.Blit(SeenBuff, XPos, YPos, XPos, YPos, 12, 12);
      delete this;
      return;
    }
  }
}

// TODO: When every slot is taken this leaks `obj` and still returns 0, which a
// caller cannot tell from "stored in slot 0". Show_Credits() would then delete
// slot 0 (the clock) and Input_Name() would wait forever for it to empty. The
// hall of fame loop in Presentation() can fill the table: rows without a score
// are queued back to back with no delay in between.
int Alloc_Object(ScoreAnimClass* obj) {
  int ret = 0;

  for (int i = ret = 0; i < MAXSCOREOBJS; i++) {
    if (!base::At(ScoreObjs, i)) {
      base::At(ScoreObjs, i) = obj;
      ret = i;
      break;
    }
  }
  return ret;
}

// 16-entry font palettes for Set_Font_Palette(): each maps the gradient score
// font onto one colour ramp of the score screen palette. Blue is the Allied
// colour, red the Soviet one, green the headings and ratings, and yellow the
// "click to continue" prompt.
static const unsigned char bluepal[] = {0xC0, 0xC1, 0xC1, 0xC3, 0xC2, 0xC5,
                                        0xC3, 0xC7, 0xC4, 0xC9, 0xCA, 0xCB,
                                        0xCC, 0xCD, 0xC0, 0xCF};
static const unsigned char greenpal[] = {0x70, 0x71, 0x7C, 0x73, 0x7D, 0x75,
                                         0x7E, 0x77, 0x7F, 0x79, 0x7A, 0x7B,
                                         0x7C, 0x7D, 0x7C, 0x7F};
static const unsigned char redpal[] = {0xD0, 0xD1, 0xD7, 0xD3, 0xD9, 0xD5,
                                       0xDA, 0xD7, 0xDB, 0xD9, 0xDA, 0xDB,
                                       0xDC, 0xDD, 0xD6, 0xDF};
static const unsigned char yellowpal[] = {0x0,  0x0, 0xEC, 0x0, 0xEB, 0x0,
                                          0xEA, 0x0, 0xE9, 0x0, 0x0,  0x0,
                                          0x0,  0x0, 0xED, 0x0};
void ScoreClass::Presentation() {
  // Text positions, 320x200. Each table has a second entry for a layout that is
  // never selected; see `indx` below.
  static const int _casuax[2] = {144, 150};
  static const int _casuay[2] = {78, 78};
  static const int _gditxy[2] = {90, 90};

  static const int _gditxx[2] = {config::kIsEnglish ? 135 : 130, 150};
  static const int _nodtxx[2] = {config::kIsEnglish ? 135 : 130, 150};
  static const int _nodtxy[2] = {102, 102};
  static const int _bldggy[2] = {138, 138};
  static const int _bldgny[2] = {150, 150};

  GameFile file(kFameFileName);
  struct Fame hallfame[NUMFAMENAMES];
  const int oldfontxspacing = FontXSpacing;
  const int house = IsSovietHouse(PlayerPtr->Class->House) ? 1 : 0;  // 0 or 1

  ControlQ = false;
  FontXSpacing = 0;
  Map.Override_Mouse_Shape(MOUSE_NORMAL);
  Theme.Queue_Song(THEME_SCORE);

  VisiblePage.Clear();
  WWMouse->Erase_Mouse(&HidPage, true);
  HiddenPage.Clear();
  BlackPalette.Set();

  const auto country4 = MixArchive::RetrieveData("COUNTRY4.AUD");
  const auto sfx4 = MixArchive::RetrieveData("SFX4.AUD");
  Beepy6 = MixArchive::RetrieveData("BEEPY6.AUD");

  // The minute in progress counts, so the shortest mission shows as 1 minute.
  const int minutes = static_cast<int>(ElapsedTime / kTimerMinute) + 1;

  // Bar shapes for Do_GDI_Graph(): the player's side in one colour ("yellow",
  // a name from the Tiberian Dawn GDI screen) and the enemy in red.
  const auto yellowptr = MixArchive::RetrieveData("BAR3BHR.SHP");
  const auto redptr = MixArchive::RetrieveData("BAR3RHR.SHP");

  // Change to the score screen font; restored on the way out.
  const std::span<const std::byte> oldfont = Set_Font(ScoreFontPtr);
  ServiceRealTime();

  // Load this side's background onto the hidden page, brighten its palette,
  // and fade it in from black.
  Hide_Mouse();
  Load_Title_Screen(base::At(ScreenNames, house), &HidPage, ScorePalette);
  Increase_Palette_Luminance(ScorePalette, 30, 30, 30, 63);
  HidPage.Blit(SeenBuff);
  ScorePalette.Set(kFadePaletteFast, ServiceRealTime);
  Play_Sample(country4, 255, Options.Normalize_Volume(150));

  // Background's up, so now start the animations that loop for the whole
  // screen: the clock and the two hall of fame ornaments. They take slots 0..2
  // of ScoreObjs[] and are only deleted by the teardown at the end.
  const auto timeshape = MixArchive::RetrieveData("TIMEHR.SHP");
  const auto hiscore1shape = MixArchive::RetrieveData("HISC1-HR.SHP");
  const auto hiscore2shape = MixArchive::RetrieveData("HISC2-HR.SHP");
  ScoreObjs[0] = new ScoreTimeClass(238, 2, timeshape, 30, 4);
  ScoreObjs[1] = new ScoreTimeClass(4, 89, hiscore1shape, 10, 4);
  ScoreObjs[2] = new ScoreTimeClass(4, 180, hiscore2shape, 10, 4);

  // Type out the headings. Each Call_Back_Delay() below is sized to let the
  // text queued before it finish, which is what keeps ScoreObjs[] from
  // overflowing.
  Set_Logic_Page(SeenBuff);

  Alloc_Object(new ScorePrintClass(TXT_SCORE_TIME,
                                   config::kIsFrench ? 198 : 204, 9, greenpal));
  Alloc_Object(new ScorePrintClass(TXT_SCORE_LEAD, 164, 26, greenpal));
  Alloc_Object(new ScorePrintClass(TXT_SCORE_EFFI, 164, 38, greenpal));
  Alloc_Object(new ScorePrintClass(TXT_SCORE_TOTA, 164, 50, greenpal));
  Play_Sample(sfx4, 255, Options.Normalize_Volume(150));
  Call_Back_Delay(13);

  Keyboard->Clear();

  // Determine leadership rating: the share of the player's side that survived.
  // First count what is left on the map...
  int leadership = 0;
  for (int index = 0; index < Logic.Count(); index++) {
    const ObjectClass* object = Logic.at(index);
    const HousesType owner = object->Owner();
    if (house && (IsSovietHouse(owner) || owner == HOUSE_BAD)) {
      leadership++;
    } else {
      if (!house && object->Owner() == HOUSE_GREECE) {
        leadership++;
      }
    }
  }

  // ...then total each side's losses, and the points earned by the player and
  // allies.
  int uspoints = 0;
  for (HousesType hous = HOUSE_SPAIN; hous <= HOUSE_BAD; hous++) {
    const HouseClass* hows = HouseClass::As_Pointer(hous);
    if (IsSovietHouse(hous) || hous == HOUSE_BAD) {
      NKilled += hows->UnitsLost;
      NBKilled += hows->BuildingsLost;
    } else {
      GKilled += hows->UnitsLost;
      GBKilled += hows->BuildingsLost;
    }
    if (PlayerPtr->Is_Ally(hous)) {
      uspoints += hows->PointTotal;
    }
  }

  // Bias the base score upward according to the difficulty level. (BG once
  // tried a flat 1000-point bonus for winning, and flooring the points at zero,
  // here; both were disabled, so a negative base score is possible.)
  switch (PlayerPtr->Difficulty) {
    case DIFF_EASY:
      uspoints += 500;
      break;

    case DIFF_NORMAL:
      uspoints += 1500;
      break;

    case DIFF_HARD:
      uspoints += 3500;
      break;
    default:
      break;
  }

  // Survivors as a percentage of survivors plus losses. Counting at least one
  // survivor keeps the divisor from being zero. The ratio cannot pass 100, so
  // the cap of 150 never applies.
  if (!leadership) {
    leadership++;
  }
  leadership = 100 * fixed(leadership, house ? NKilled + NBKilled + leadership
                                             : GKilled + GBKilled + leadership);
  leadership = std::min(150, leadership);

  // Determine economy rating: money left (plus credits from captured buildings)
  // as a percentage of everything the player had to spend, i.e. starting
  // credits plus harvest. The +1 on each side avoids dividing by zero. Capped
  // at 150%.
  int economy =
      100 *
      fixed(static_cast<int>(PlayerPtr->Available_Money()) + 1 +
                PlayerPtr->StolenBuildingsCredits,
            PlayerPtr->HarvestedCredits +
                static_cast<int>(PlayerPtr->Control.InitialCredits) + 1);
  economy = std::min(economy, 150);

  // Clamped to what fits the five-character "%5d" field.
  int total = (uspoints * leadership / 100) + (uspoints * economy / 100);
  total = std::clamp(total, -9999, 99999);

  Keyboard->Clear();
  // Count both ratings up together, economy starting 30 ticks after
  // leadership: tick i shows i% of the final leadership and (i - 30)% of the
  // final economy, so a full run is 100 + 30 ticks.
  //
  // TODO: The early exit compares i with the ratings themselves rather than
  // with 100, so for ratings under 100 it fires before the counters have
  // reached their values (leadership 50 stops at i == 50 showing 25) and the
  // exact prints after the loop make the numbers jump.
  for (int i = 0; i <= 130; i++) {
    Set_Font_Palette(greenpal);
    const int lead = leadership * i / 100;
    Count_Up_Print("%3d%%", lead, leadership, 244, 26);
    if (i >= 30) {
      const int econo = economy * (i - 30) / 100;
      Count_Up_Print("%3d%%", econo, economy, 244, 38);
    }
    Print_Minutes(minutes);
    Call_Back_Delay(1);
    Play_Sample(Beepy6, 255, Options.Normalize_Volume(100));
    if (i >= 30 && i >= leadership && i - 30 >= economy) {
      break;
    }
  }
  Count_Up_Print("%3d%%", leadership, leadership, 244, 26);
  Count_Up_Print("%3d%%", economy, economy, 244, 38);

  // ScorePrintClass views `buffer` rather than copying it, so the delays that
  // follow must outlast the typing of "x nnnnn" (one letter per tick) before
  // the buffer is reused for the total.
  char buffer[16];
  absl::SNPrintF(buffer, sizeof(buffer), "x %5d", uspoints);
  Alloc_Object(new ScorePrintClass(buffer, 274, 26, greenpal));
  Alloc_Object(new ScorePrintClass(buffer, 274, 38, greenpal));
  Call_Back_Delay(8);
  // Rule off the sum: flash the line white for a tick, then settle on green.
  SeenBuff.Draw_Line(548, 96, 626, 96, kWhite);
  Call_Back_Delay(1);
  SeenBuff.Draw_Line(548, 96, 626, 96, kGreen);

  absl::SNPrintF(buffer, sizeof(buffer), "%5d", total);
  Alloc_Object(new ScorePrintClass(buffer, 286, 50, greenpal));

  Call_Back_Delay(60);

  // The Soviet background has its credits box up here, under the ratings; the
  // Allied one has it at the bottom, so that side shows credits after the
  // graphs.
  if (house) {
    Show_Credits(house, greenpal);
  }

  // The `BG` remnants here and in Do_GDI_Graph(): these pauses used to be
  // skipped once a key was waiting. With that disabled, Ctrl-Q (see
  // Call_Back_Delay) is the only way to hurry the screen along.
  /*BG	if (!Keyboard->Check()) */ Call_Back_Delay(60);

  // Show stats on # of units killed. The player's own side is always the upper
  // of the two rows.
  Set_Logic_Page(SeenBuff);
  Play_Sample(sfx4, 255, Options.Normalize_Volume(150));
  // The original selected the second layout for Soviet players on DOS only;
  // at this resolution both sides share entry 0.
  const int indx = 0;
  Alloc_Object(new ScorePrintClass(TXT_SCORE_CASU, _casuax[indx], _casuay[indx],
                                   greenpal));
  Call_Back_Delay(9);
  if (house) {
    Alloc_Object(
        new ScorePrintClass(TXT_SOVIET, _nodtxx[indx], _gditxy[indx], redpal));
    Alloc_Object(
        new ScorePrintClass(TXT_ALLIES, _gditxx[indx], _nodtxy[indx], bluepal));
  } else {
    Alloc_Object(
        new ScorePrintClass(TXT_ALLIES, _gditxx[indx], _gditxy[indx], bluepal));
    Alloc_Object(
        new ScorePrintClass(TXT_SOVIET, _nodtxx[indx], _nodtxy[indx], redpal));
  }
  Call_Back_Delay(6);

  Set_Font_Palette(redpal);
  Do_GDI_Graph(yellowptr, redptr, GKilled + CKilled, NKilled, 89);

  Set_Logic_Page(SeenBuff);

  // Print out stats on buildings destroyed, laid out like the casualties above.
  Play_Sample(sfx4, 255, Options.Normalize_Volume(150));
  Alloc_Object(new ScorePrintClass(TXT_SCORE_BUIL, 144, 126, greenpal));
  Call_Back_Delay(9);
  if (house) {
    Alloc_Object(
        new ScorePrintClass(TXT_SOVIET, _gditxx[indx], _bldggy[indx], redpal));
    Alloc_Object(
        new ScorePrintClass(TXT_ALLIES, _gditxx[indx], _bldgny[indx], bluepal));
  } else {
    Alloc_Object(
        new ScorePrintClass(TXT_ALLIES, _gditxx[indx], _bldggy[indx], bluepal));
    Alloc_Object(
        new ScorePrintClass(TXT_SOVIET, _gditxx[indx], _bldgny[indx], redpal));
  }
  Call_Back_Delay(7);
  Do_GDI_Graph(yellowptr, redptr, GBKilled + CBKilled, NBKilled, 137);

  // Wait for text printing to complete
  while (StillUpdating) {
    Call_Back_Delay(1);
  }

  Keyboard->Clear();

  if (!house) {
    Show_Credits(house, greenpal);
  }
  // Hall of fame display and processing.
  Play_Sample(sfx4, 255, Options.Normalize_Volume(150));
  Alloc_Object(new ScorePrintClass(TXT_SCORE_TOP, 28, 110, greenpal));
  Call_Back_Delay(9);

  // First check for the existence of the file, and if there isn't one, make a
  // new one filled with blanks.
  if (!file.IsAvailable()) {
    // TODO: Only name[0] and the three ints are set; the rest of each name and
    // the struct padding go to disk as uninitialized stack bytes.
    file.Open(FileAccess::kWrite);

    for (auto& i : hallfame) {
      i.name[0] = static_cast<char>(i.score = i.level = 0);
      i.side = 0;
      file.WriteObject(i);
    }

    file.Close();
  }

  file.Open(FileAccess::kRead);
  for (auto& i : hallfame) {
    file.ReadObject(i);
  }
  file.Close();

  // If the player's score is good enough to bump someone off the list, remove
  // their data, move everyone down a notch, and set index = where their info
  // goes. index == NUMFAMENAMES afterwards means the player did not make the
  // list.
  //
  // TODO: The first test defeats "good enough". When the player does not beat
  // the bottom row, that row's score is zeroed, so any positive total then wins
  // the bottom slot and evicts a better score. With a total of zero or less the
  // row just loses its score on screen (the file is not rewritten). This is the
  // original behaviour; it may have been meant to always show the latest game.
  if (hallfame[NUMFAMENAMES - 1].score >= total) {
    hallfame[NUMFAMENAMES - 1].score = 0;
  }
  int index = 0;
  for (index = 0; index < NUMFAMENAMES; index++) {
    if (total > base::At(hallfame, index).score) {
      if (index < NUMFAMENAMES - 1) {
        for (int i = NUMFAMENAMES - 1; i > index; i--) {
          base::At(hallfame, i) = base::At(hallfame, i - 1);
        }
      }
      base::At(hallfame, index).score = total;
      base::At(hallfame, index).level = Scen.Scenario;
      base::At(base::At(hallfame, index).name, 0) = 0;  // blank out the name
      base::At(hallfame, index).side = house;
      break;
    }
  }

  // Now display the hall of fame. The printers view their strings, so each row
  // gets its own 32-byte slice of `maststr` that stays valid while it types:
  // the score at offset 0 and the mission number at offset 16.
  Set_Logic_Page(SeenBuff);

  char maststr[NUMFAMENAMES * 32];
  std::span<const uint8_t> pal;
  for (int i = 0; i < NUMFAMENAMES; i++) {
    pal = base::At(hallfame, i).side ? redpal : bluepal;
    Alloc_Object(new ScorePrintClass(base::At(hallfame, i).name, HALLFAME_X,
                                     HALLFAME_Y + (i * 8), pal));
    if (base::At(hallfame, i).score) {
      const auto str = std::span(maststr).subspan(base::ToSize(i) * 32, 32);
      absl::SNPrintF(str.data(), str.size(), "%d", base::At(hallfame, i).score);
      Alloc_Object(new ScorePrintClass(str.data(), HALLFAME_X + (6 * 14),
                                       HALLFAME_Y + (i * 8), pal));
      // Scenario numbers from 20 up are expansion missions, not campaign
      // levels; they show as "**".
      if (base::At(hallfame, i).level < 20) {
        absl::SNPrintF(str.subspan(16).data(), str.size() - 16, "%d",
                       base::At(hallfame, i).level);
      } else {
        absl::SNPrintF(str.subspan(16).data(), str.size() - 16, "**");
      }
      Alloc_Object(new ScorePrintClass(str.subspan(16).data(),
                                       HALLFAME_X + (6 * 11),
                                       HALLFAME_Y + (i * 8), pal));
      Call_Back_Delay(13);
    }
  }
  // Wait for text printing to complete
  while (StillUpdating) {
    Call_Back_Delay(1);
  }
  // If the player's on the hall of fame, have him enter his name now and save
  // the table. Otherwise just wait for a click.
  Keyboard->Clear();

  if (index < NUMFAMENAMES) {
    pal = base::At(hallfame, index).side ? redpal : bluepal;
    Input_Name(base::At(hallfame, index).name, HALLFAME_X,
               HALLFAME_Y + (index * 8), pal);

    file.Open(FileAccess::kWrite);
    for (const auto& i : hallfame) {
      file.WriteObject(i);
    }
    file.Close();
  } else {
    Alloc_Object(new ScorePrintClass(TXT_CLICK_CONTINUE, 149, 190, yellowpal));
    ControlQ = false;
    Cycle_Wait_Click();
  }

  Keyboard->Clear();

  // Get rid of all the animating objects: the three looping shapes and any
  // text that Ctrl-Q cut short.
  for (auto& ScoreObj : ScoreObjs) {
    if (ScoreObj) {
      delete ScoreObj;
      ScoreObj = nullptr;
    }
  }
  BlackPalette.Set(kFadePaletteFast, nullptr);
  VisiblePage.Clear();
  Show_Mouse();

  Theme.Queue_Song(THEME_NONE);

  BlackPalette.Set(kFadePaletteFast, nullptr);
  VisiblePage.Clear();
  GamePalette.Set();

  Set_Font(oldfont);
  FontXSpacing = oldfontxspacing;
  ControlQ = false;
}

void Cycle_Wait_Click(bool cycle) {
  int counter = 0;
  // Ticks left during which input is thrown away.
  int minclicks = 20;
  int64_t timingtime = TickCount.Value();
  SerialPacketType sendpacket{
      .Command = SERIAL_SCORE_SCREEN, .Name = {}, .ID = 0, .ScenarioInfo = {}};
  SerialPacketType receivepacket{
      .Command = SERIAL_LAST_COMMAND, .Name = {}, .ID = 0, .ScenarioInfo = {}};
  int packetlen = 0;

  Keyboard->Clear();
  while (minclicks || (!Keyboard->Check() && !ControlQ)) {
    if (Session.Type == GAME_NULL_MODEM || Session.Type == GAME_MODEM) {
      // Send a timing packet if enough time has gone by, so the other machine
      // keeps measuring the link while both sit on this screen.
      if (TickCount.Value() - timingtime > PACKET_TIMING_TIMEOUT) {
        base::FillBytes(base::ObjectBytes(sendpacket), 0,
                        sizeof(SerialPacketType));
        sendpacket.Command = SERIAL_SCORE_SCREEN;
        sendpacket.ScenarioInfo.ResponseTime = NullModem.Response_Time();
        sendpacket.ID = static_cast<unsigned char>(Session.ModemType);

        NullModem.Send_Message(base::ObjectBytes(sendpacket),
                               sizeof(sendpacket), 0);
        timingtime = TickCount.Value();
      }

      if (NullModem.Get_Message(base::ObjectBytes(receivepacket), &packetlen) >
          0) {
        // Throw the packet away: the peer's timing packets only need to be
        // drained from the queue.
      }

      NullModem.Service();
    }

    Call_Back_Delay(1);
    if (minclicks) {
      minclicks--;
      Keyboard->Clear();
    }

    // Every 8th tick, rotate palette entries 233..237 one place down.
    if (cycle) {
      counter = (counter + 1) % 8;
      if (counter == 0 && Options.IsPaletteScroll) {
        const RGBClass rgb = ScorePalette.at(233);
        for (int i = 233; i < 237; i++) {
          ScorePalette.at(i) = ScorePalette.at(i + 1);
        }
        ScorePalette.at(237) = rgb;
        ScorePalette.Set();
      }
    }
  }
  Keyboard->Clear();
}

void ScoreClass::Do_GDI_Graph(std::span<const std::byte> yellowptr,
                              std::span<const std::byte> redptr, int gkilled,
                              int nkilled, int ypos) {
  // Left edge of both bars, 320x200.
  const int xpos = 174;
  const int house = IsSovietHouse(PlayerPtr->Class->House) ? 1 : 0;  // 0 or 1
  // A Soviet player's own losses go on top, so swap the counts and the bar
  // colours. From here on "gdi"/"g" means the top bar and "nod"/"n" the bottom.
  if (house) {
    const int temp = gkilled;
    gkilled = nkilled;
    nkilled = temp;
    const auto tempptr = yellowptr;
    yellowptr = redptr;
    redptr = tempptr;
  }
  int gdikilled = gkilled;
  int nodkilled = nkilled;

  int maxval = std::max(gdikilled, nodkilled);
  if (!maxval) {
    maxval = 1;
  }

  // Convert the counts to bar lengths. Normally the larger count fills the
  // graph and the other is in proportion; below 20 a fixed 5 pixels per kill is
  // used instead so that a handful of losses doesn't draw a full-width bar.
  gdikilled = gdikilled * SIZEGBAR / maxval;
  nodkilled = nodkilled * SIZEGBAR / maxval;
  if (maxval < 20) {
    gdikilled = gkilled * 5;
    nodkilled = nkilled * 5;
  }

  // From here maxval is the longer bar's length, used to turn a bar position
  // back into a count.
  //
  // TODO: That conversion suits bars growing side by side, as in Tiberian
  // Dawn's Nod casualty graph, where it was copied from. Here each bar stops at
  // its own length, so the shorter bar's counter only gets to
  // count * shorter / longer (50 against 100 stops at 25) and then jumps to the
  // real count. Dividing by the bar's own length would fix it.
  maxval = std::max(gdikilled, nodkilled);
  if (!maxval) {
    maxval = 1;
  }

  // Draw the white-flash shape on the hidpage. It is blitted over the last
  // step of each bar, cut to that bar's length plus 3 pixels of end cap, and
  // then replaced by the final coloured frame.
  Set_Logic_Page(HidPage);
  HidPage.Fill_Rect(0, 0, 248, 18, kTBlack);
  CC_Draw_Shape(redptr, 119, 0, 0, WINDOW_MAIN, SHAPE_WIN_REL, {}, {});
  Set_Logic_Page(SeenBuff);
  Set_Font_Palette(house ? redpal : bluepal);

  for (int i = 1; i <= gdikilled; i++) {
    if (i != gdikilled) {
      CC_Draw_Shape(yellowptr, i, xpos * 2, ypos * 2, WINDOW_MAIN,
                    SHAPE_WIN_REL, {}, {});
    } else {
      HidPage.Blit(SeenBuff, 0, 0, xpos * 2, ypos * 2, (3 + gdikilled) * 2, 16);
    }

    Count_Up_Print("%d", i * gkilled / maxval, gkilled, 297, ypos + 2);
    Play_Sample(Beepy6, 255, Options.Normalize_Volume(150));
    Call_Back_Delay(2);
  }
  CC_Draw_Shape(yellowptr, gdikilled, xpos * 2, ypos * 2, WINDOW_MAIN,
                SHAPE_WIN_REL, {}, {});
  Count_Up_Print("%d", gkilled, gkilled, 297, ypos + 2);
  /*BG	if (!Keyboard->Check()) */ Call_Back_Delay(40);

  Set_Font_Palette(house ? bluepal : redpal);
  for (int i = 1; i <= nodkilled; i++) {
    if (i != nodkilled) {
      CC_Draw_Shape(redptr, i, xpos * 2, (ypos + 12) * 2, WINDOW_MAIN,
                    SHAPE_WIN_REL, {}, {});
    } else {
      HidPage.Blit(SeenBuff, 0, 0, xpos * 2, (ypos + 12) * 2,
                   (3 + nodkilled) * 2, 16);
    }

    Count_Up_Print("%d", i * nkilled / maxval, nkilled, 297, ypos + 14);
    Play_Sample(Beepy6, 255, Options.Normalize_Volume(150));
    Call_Back_Delay(2);
  }

  // Make sure accurate count is printed at end; integer division in the loop
  // can leave the counter short.
  CC_Draw_Shape(redptr, nodkilled, xpos * 2, (ypos + 12) * 2, WINDOW_MAIN,
                SHAPE_WIN_REL, {}, {});
  Count_Up_Print("%d", nkilled, nkilled, 297, ypos + 14);
  /*BG	if (!Keyboard->Check()) */ Call_Back_Delay(40);
}

void ScoreClass::Show_Credits(int house, std::span<const uint8_t> pal) {
  // 320x200 positions indexed by `house`: the spinning credits shape (_creds),
  // the counting number (_credp) and the caption (_credt). German text is
  // longer, so it starts further left and sits differently.
  static const int _credsx[2] = {276, 276};
  static const int _credsy[2] = {173, 58};
  static const int _credpx[2] = {228, 236};
  static const int _credpy[2] = {config::kIsGerman ? 181 : 189 - 12, 74};
  static const int _credtx[2] = {config::kIsGerman ? 162 : 182,
                                 config::kIsGerman ? 162 : 182};
  static const int _credty[2] = {config::kIsGerman ? 173 : 179 - 12, 62};

  const auto credshape =
      MixArchive::RetrieveData(house ? "CREDSUHR.SHP" : "CREDSAHR.SHP");

  Alloc_Object(new ScorePrintClass(TXT_SCORE_ENDCRED, base::At(_credtx, house),
                                   base::At(_credty, house), pal));
  Call_Back_Delay(15);

  const int credobj = Alloc_Object(new ScoreCredsClass(
      base::At(_credsx, house), base::At(_credsy, house), credshape, 32, 2));
  // Smallest step: bounds the count-up to about 100 steps however rich the
  // player is.
  const int minval = static_cast<int>(PlayerPtr->Available_Money() / 100);

  // Print out total credits left at end of scenario. The counter starts below
  // zero, which holds the display at 0 for a moment, and the step grows with
  // the distance still to go so that large sums don't take forever.
  int i = -50;

  do {
    int add = 5;
    if (PlayerPtr->Available_Money() - i > 100) {
      add += 15;
    }
    if (PlayerPtr->Available_Money() - i > 500) {
      add += 30;
    }
    if (PlayerPtr->Available_Money() - i > 1000) {
      add = static_cast<int>(add + (PlayerPtr->Available_Money() / 40));
    }
    add = std::max(add, minval);
    i += add;

    i = std::max(i, 0);

    Set_Font_Palette(pal);
    Count_Up_Print("%d", i, static_cast<int>(PlayerPtr->Available_Money()),
                   base::At(_credpx, house), base::At(_credpy, house));
    Call_Back_Delay(2);
  } while (i < PlayerPtr->Available_Money());

  // The credits animation loops forever, so stop it by hand.
  delete base::At(ScoreObjs, credobj);
  base::At(ScoreObjs, credobj) = nullptr;
}

void ScoreClass::Print_Minutes(int minutes) {
  char str[20];
  if (minutes >= 60) {
    // The hours field is a single digit.
    if (minutes / 60 > 9) {
      minutes = (9 * 60) + 59;
    }
    Format_Runtime_Text(str, sizeof(str), Text_String(TXT_SCORE_TIMEFORMAT1),
                        minutes / 60, minutes % 60);
  } else {
    Format_Runtime_Text(str, sizeof(str), Text_String(TXT_SCORE_TIMEFORMAT2),
                        minutes);
  }
  SeenBuff.Print(str, 550, 18, kTBlack, kTBlack);
}

void ScoreClass::Count_Up_Print(const char* str, int percent, int maxval,
                                int xpos, int ypos) {
  char destbuf[64];

  Format_Runtime_Text(destbuf, sizeof(destbuf), str,
                      percent <= maxval ? percent : maxval);
  SeenBuff.Print(destbuf, xpos * 2, ypos * 2, kTBlack, kBlack);
}

void ScoreClass::Input_Name(std::span<char> str, int xpos, int ypos,
                            std::span<const uint8_t> pal) {
  int key = 0;
  // Cursor position within `str`. It stops at the last letter,
  // MAX_FAMENAME_LENGTH - 2, which further typing overwrites.
  int index = 0;

  const auto keystrok = MixArchive::RetrieveData("KEYSTROK.AUD");

  // Ready the hidpage so it can restore background under zoomed letters.
  SeenBuff.Blit(HidPage);

  // Put a copy of the high score area on a spare area of the hidpage, so we can
  // use it to restore the letter's background instead of filling with black.
  // The copy sits 200 pixels (100 in 320x200 terms) above the original, which
  // is where the `ypos - 100` and Animate_Cursor()'s `ypos - 200` come from.
  HidPage.Blit(HidPage, 0, 200, 0, 0, 200, 200);

  do {
    ServiceRealTime();
    Animate_Score_Objs();
    Animate_Cursor(index, ypos);
    if (Keyboard->Check()) {
      // Keep the character and drop the modifier bits.
      key = KeyboardClass::To_ASCII(Keyboard->Get()) & 0xFF;
      ServiceRealTime();

      // On the last letter, flush the type-ahead so that key repeat doesn't
      // keep overwriting it.
      if (index == MAX_FAMENAME_LENGTH - 2) {
        while (Keyboard->Check()) {
          Keyboard->Get();
        }
      }

      // If they hit 'backspace' when they're on the last letter, turn it into a
      // space instead. The cursor never moves past the last letter, so a plain
      // backspace there would delete the letter before it and leave the last
      // one standing.
      if ((key == KA_BACKSPACE && index == MAX_FAMENAME_LENGTH - 2) &&
          (base::At(str, base::ToSize(index)) &&
           base::At(str, base::ToSize(index)) != 32)) {
        key = 32;
      }

      if (key == KA_BACKSPACE) {
        if (index) {
          base::At(str, base::ToSize(--index)) = 0;

          // Erase the letter on both pages from the saved background copy.

          const int xposindex6 = (xpos + (index * 6)) * 2;
          HidPage.Blit(SeenBuff, xposindex6, (ypos - 100) * 2, xposindex6,
                       ypos * 2, 12, 12);
          HidPage.Blit(HidPage, xposindex6, (ypos - 100) * 2, xposindex6,
                       ypos * 2, 12, 12);
        }

      } else if (key != KA_RETURN) {
        // Names are upper case only.
        int ascii = key;
        if (ascii >= 'a' && ascii <= 'z') {
          ascii -= 'a' - 'A';
        }
        if ((ascii >= '!' && ascii <= KA_TILDA) || ascii == ' ') {
          HidPage.Blit(SeenBuff, (xpos + (index * 6)) * 2, (ypos - 100) * 2,
                       (xpos + (index * 6)) * 2, ypos * 2, 12, 12);
          HidPage.Blit(HidPage, (xpos + (index * 6)) * 2, (ypos - 100) * 2,
                       (xpos + (index * 6)) * 2, ypos * 2, 12, 12);
          base::At(str, base::ToSize(index)) = static_cast<char>(ascii);
          base::At(str, base::ToSize(index + 1)) = 0;

          Play_Sample(keystrok, 255, Options.Normalize_Volume(150));
          // Echo the letter and wait until its animation has finished and
          // freed its slot, so that letters appear strictly one at a time.
          const int objindex = Alloc_Object(
              new ScoreScaleClass(str.subspan(base::ToSize(index)).data(),
                                  xpos + (index * 6), ypos, pal));
          while (base::At(ScoreObjs, objindex)) {
            Call_Back_Delay(1);
          }

          if (index < MAX_FAMENAME_LENGTH - 2) {
            index++;
          }
        }
      }
    }
  } while (key != KA_RETURN);
}

void Animate_Cursor(int pos, int ypos) {
  static int _lastpos = 0;
  // 0 draws the cursor transparent (hidden), 1 draws it blue.
  static int _state;
  static Timer<SystemTickSource> _timer;

  ypos += 6;  // move cursor to bottom of letter

  ypos *= 2;

  // If they moved the cursor, erase the old one from Input_Name()'s saved
  // background copy, 200 pixels up the hidden page, and restart the blink.
  if (pos != _lastpos) {
    HidPage.Blit(SeenBuff, (HALLFAME_X + (_lastpos * 6)) * 2, ypos - 200,
                 (HALLFAME_X + (_lastpos * 6)) * 2, ypos, 12, 2);
    _lastpos = pos;
    _state = 0;
  }
  SeenBuff.Draw_Line((HALLFAME_X + (pos * 6)) * 2, ypos,
                     (HALLFAME_X + (pos * 6) + 5) * 2, ypos,
                     _state ? kLtBlue : kTBlack);
  // Toggle the color of the cursor, blue or hidden, if it's time to do so:
  // every 5 ticks.
  if (_timer.IsFinished()) {
    _state = _state == 0 ? 1 : 0;
    _timer.Set(5);
  }
}

void Call_Back_Delay(int time) {
  time = std::clamp(time, 0, 60);
  // Paces the full ServiceRealTime(): at most four times a second.
  Timer<SystemTickSource> callbackcd{0};

  if ((!ControlQ) &&
      (KeyboardClass::Down(KN_LCTRL) && KeyboardClass::Down(KN_Q))) {
    ControlQ = true;
    Keyboard->Clear();
  }

  if (ControlQ) {
    time = 0;
  }

  const Timer<SystemTickSource> cd{time};
  // Tell the music streamer to do as little work per call as it can while this
  // loop spins. In between the full services, only keep the sound fed and the
  // frame presented.
  StreamLowImpact = true;
  do {
    if (callbackcd.IsFinished()) {
      ServiceRealTime();
      callbackcd.Set(kTimerSecond / 4);
    } else {
      if (SoundType != SFX_NONE) {
        Sound_Callback();
      }
      Video_End_Frame();
    }
    Animate_Score_Objs();
  } while (cd.HasTimeLeft());
  StreamLowImpact = false;
}

void Animate_Score_Objs() {
  StillUpdating = false;
  for (auto& ScoreObj : ScoreObjs) {
    if (ScoreObj) {
      ScoreObj->Update();
    }
  }
}

// Formats `a` into a static buffer that the next call overwrites. Callers hand
// the result to a ScorePrintClass, which only views it, so they must delay
// long enough for it to finish typing before calling again.
static char* Int_Print(int a) {
  static char str[10];

  absl::SNPrintF(str, sizeof(str), "%d", a);
  return str;
}

void Multi_Score_Presentation() {
  unsigned char remap[16];
  // The background animation is 320x200; each frame is decoded here and then
  // doubled onto the visible page.
  GraphicBufferClass pseudoseenbuff(320, 200);

  const int oldfontxspacing = FontXSpacing;

  FontXSpacing = 0;
  Map.Override_Mouse_Shape(MOUSE_NORMAL);

  BlackPalette.Set();
  SeenBuff.Clear();
  HidPage.Clear();
  Hide_Mouse();
  void* anim = Open_Animation(
      "MLTIPLYR.WSA", WSA_OPEN_FROM_MEM | WSA_OPEN_TO_PAGE, ScorePalette);
  // Display the background animation. The first frame goes up under a black
  // palette and is faded in; the remaining frames then play at two ticks each.
  pseudoseenbuff.Clear();
  Animate_Frame(anim, pseudoseenbuff, 1);
  Interpolate_2X_Scale(&pseudoseenbuff, &SeenBuff, {});
  ScorePalette.Set(kFadePaletteFast, ServiceRealTime);

  int frame = 1;
  while (frame < Get_Animation_Frame_Count(anim)) {
    Animate_Frame(anim, pseudoseenbuff, frame++);
    Interpolate_2X_Scale(&pseudoseenbuff, &SeenBuff, {});
    Call_Back_Delay(2);
  }
  Close_Animation(anim);

  // Change to the score screen font; restored on the way out.
  const std::span<const std::byte> oldfont = Set_Font(ScoreFontPtr);
  ServiceRealTime();

  Set_Logic_Page(SeenBuff);

  Alloc_Object(new ScorePrintClass(TXT_SCORE_TOP, config::kIsFrench ? 113 : 130,
                                   13, greenpal));
  Call_Back_Delay(5);
  Alloc_Object(new ScorePrintClass(TXT_COMMANDER, 27, 31, greenpal));
  Call_Back_Delay(10);
  Alloc_Object(new ScorePrintClass(
      TXT_BATTLES_WON,
      [] {
        if (config::kIsFrench) {
          return 113;
        }
        if (config::kIsGerman) {
          return 118;
        }
        return 126;
      }(),
      31, greenpal));
  Call_Back_Delay(13);
  Alloc_Object(new ScorePrintClass(TXT_KILLS_COLON, 249, 31, greenpal));
  Call_Back_Delay(6);

  // Move all the scores over a notch if there's more games than can be shown
  // (which is known by Session.CurGame == MAX_MULTI_GAMES-1), dropping the
  // oldest game's kills.
  if (Session.CurGame == MAX_MULTI_GAMES - 1) {
    for (auto& i : Session.Score) {
      for (int k = 0; k < MAX_MULTI_GAMES - 1; k++) {
        base::At(i.Kills, k) = base::At(i.Kills, k + 1);
      }
    }
  }

  int y = 41;
  for (auto& i : Session.Score) {
    if (!std::string_view(i.Name).empty()) {
      // Build a font palette in the player's colour from the brightest five
      // steps of that colour's font ramp. Only the entries the score font
      // uses are set; the rest of `remap` stays uninitialized.
      const PlayerColorType color = i.Color;
      remap[8] = ColorRemaps.at(color).FontRemap[11];
      remap[6] = ColorRemaps.at(color).FontRemap[12];
      remap[4] = ColorRemaps.at(color).FontRemap[13];
      remap[2] = ColorRemaps.at(color).FontRemap[14];
      remap[14] = ColorRemaps.at(color).FontRemap[15];

      Alloc_Object(new ScorePrintClass(i.Name, 15, y, remap));
      Call_Back_Delay(20);

      Alloc_Object(new ScorePrintClass(Int_Print(i.Wins), 118, y, remap));
      Call_Back_Delay(6);

      for (int k = 0; k <= std::min(Session.CurGame, MAX_MULTI_GAMES - 2);
           k++) {
        // A negative kill count marks a game the player was not in.
        if (base::At(i.Kills, k) >= 0) {
          Alloc_Object(new ScorePrintClass(Int_Print(base::At(i.Kills, k)),
                                           225 + (24 * k), y, remap));
          Call_Back_Delay(6);
        }
      }
      y += 12;
    }
  }

  Alloc_Object(new ScorePrintClass(
      TXT_CLICK_CONTINUE, config::kIsEnglish ? 109 : 95, 190, yellowpal));
  Cycle_Wait_Click(false);

  // Get rid of any text that Ctrl-Q cut short.
  for (auto& ScoreObj : ScoreObjs) {
    if (ScoreObj) {
      delete ScoreObj;
      ScoreObj = nullptr;
    }
  }

  Theme.Queue_Song(THEME_NONE);

  BlackPalette.Set(kFadePaletteFast, nullptr);
  SeenBuff.Clear();
  GamePalette.Set();
  Set_Font(oldfont);
  FontXSpacing = oldfontxspacing;
  ControlQ = false;
  Show_Mouse();
}

void ScoreClass::Init() {
  Score = 0;
  NKilled = 0;
  GKilled = 0;
  CKilled = 0;
  NBKilled = 0;
  GBKilled = 0;
  CBKilled = 0;
  NHarvested = 0;
  GHarvested = 0;
  CHarvested = 0;
  ElapsedTime = 0;
  RealTime.Reset();
}
