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

/* $Header: /counterstrike/SCORE.CPP 3     3/14/97 12:02a Steve_tall $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : SCORE.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : April 19, 1994 *
 *                                                                                             *
 *                  Last Update : May 3, 1995   [BWG] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * Call_Back_Delay -- Combines Call_Back() and Delay() functions *
 *   Draw_Bar_Graphs -- Draw "Casualties" bar graphs * Draw_InfantryMan -- Draw
 *one guy in score screen, update animation                        *
 *   Draw_Infantrymen -- Draw all the guys on the score screen *
 *   New_Infantry_Anim -- Start up a new animation for one of the infantrymen *
 *   ScoreClass::Count_Up_Print -- Prints a number (up to its max) into a
 *string, cleanly      * ScoreClass::DO_GDI_GRAPH -- Show # of people or
 *buildings killed on GDI score screen      * ScoreClass::Delay -- Pauses
 *waiting for keypress.                                         *
 *   ScoreClass::Presentation -- Main routine to display score screen. *
 *   ScoreClass::Print_Graph_Title -- Prints title on score screen. *
 *   ScoreClass::Print_Minutes -- Print out hours/minutes up to max *
 *   ScoreClass::Pulse_Bar_Graph -- Pulses the bargraph color. *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */
#include "ra/score.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <span>
#include <string_view>
#include <utility>

#include "absl/strings/str_format.h"
#include "base/array.h"
#include "base/buffer.h"
#include "base/numeric.h"
#include "ra/ccptr.h"
#include "ra/config.h"
#include "ra/conquer.h"
#include "ra/defines.h"
#include "ra/display.h"
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
#include "ra/theme.h"
#include "ra/type.h"
#include "ra/vector.h"
#include "ra/ww_audio.h"
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
#include "tech/random.h"
#include "tech/rgb.h"

#define SCORETEXT_X 184
// #define SCORETEXT_Y 8
#define CASUALTY_Y 88
#define BUILDING_X 256
#define BUILDING_Y 128
#define BARGRAPH_X 266
#define MAX_BAR_X 318  // max possible is 319 because of bar's right shadow
#define SIZEGBAR 118
#define HALLFAME_X 11
#define HALLFAME_Y 120

// #define MULTISCOREX 30

// #define TEDIT_FAME 1
#define NUMINFANTRYMEN 10
#define NUMFAMENAMES 7
#define MAX_FAMENAME_LENGTH 11

static struct InfantryAnim {
  int xpos{};
  int ypos{};
  std::span<const std::byte> shapefile;
  std::span<const uint8_t> remap;
  int anim{};
  int stage{};
  char delay{};
  const InfantryTypeClass* Class{};
} InfantryMan[NUMINFANTRYMEN];
static void Draw_InfantryMen();
static void Draw_InfantryMan(int index);
static void New_Infantry_Anim(int index, int anim);

// InfantryAnim::anim holds DoType values as an int so that -1 can mean "gone"
// and the four gun-death variants can be picked arithmetically.
constexpr int kDoGunDeath = static_cast<int>(DO_GUN_DEATH);
static void Draw_Bar_Graphs(int i, int gkilled, int nkilled);
static void Animate_Cursor(int pos, int ypos);
static void Animate_Score_Objs();
static void Cycle_Wait_Click(bool cycle = true);

static std::span<const std::byte> Beepy6;
static bool ControlQ;  // cheat key to skip past score/mapsel screens
static bool StillUpdating;

static const char* ScreenNames[2] = {"ALIBACKH.PCX", "SOVBACKH.PCX"};

struct Fame {
  char name[MAX_FAMENAME_LENGTH];
  int score;
  int level;
  int side;
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
      CashTurn(MixArchive::RetrieveData("CASHTURN.AUD")),
      Clock1(MixArchive::RetrieveData("CLOCK1.AUD")) {}

void ScoreCredsClass::Update() {
  if (AnimTimer.IsFinished()) {
    AnimTimer.Set(TimerReset);
    if (++Stage >= MaxStage) {
      Stage = 0;
    }
    GraphicViewPortClass* oldpage = LogicPage;
    Set_Logic_Page(SeenBuff);
    Play_Sample(Clock1, 255, Options.Normalize_Volume(130));
    CC_Draw_Shape(DataPtr, Stage, XPos, YPos, WINDOW_MAIN, SHAPE_WIN_REL, {},
                  {});
    Set_Logic_Page(oldpage);
  }
}

ScorePrintClass::ScorePrintClass(int string, int xpos, int ypos,
                                 std::span<const uint8_t> palette,
                                 int background)
    : ScoreAnimClass(xpos, ypos, Text_String(string)),
      Background(background),
      Stage(0),
      PrimaryPalette(palette) {}

ScorePrintClass::ScorePrintClass(std::string_view string, int xpos, int ypos,
                                 std::span<const uint8_t> palette,
                                 int background)
    : ScoreAnimClass(xpos, ypos, string),
      Background(background),
      Stage(0),
      PrimaryPalette(palette) {}

void ScorePrintClass::Update() {
  static char localstr[2] = {0, 0};
  static const uint8_t _whitepal[] = {0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F,
                                      0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F,
                                      0x0F, 0x0F, 0x0F, 0x0F};

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

    const int pos = XPos + (Stage * 12);
    // print the letter properly
    if (Stage) {
      Set_Font_Palette(PrimaryPalette);
      localstr[0] = Text()[base::ToSize(Stage - 1)];
      HidPage.Print(localstr, pos - 12, YPos, kTBlack, kTBlack);
      HidPage.Blit(SeenBuff, pos - 12, YPos - 2, pos - 12, YPos - 2, 14, 16);
    }
    if (base::ToSize(Stage) < Text().size()) {
      localstr[0] = Text()[base::ToSize(Stage)];
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
  static const int _destx[] = {0, 80, 107, 134, 180, 228};
  static const int _destw[] = {6, 20, 30, 40, 60, 80};

  /*
  ** Restore the background for the scaled-up letter
  */
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

/***********************************************************************************************
 * ScoreClass::Presentation -- Main routine to display score screen. *
 *                                                                                             *
 *    This is the main routine that displays the score screen graphics. * It
 *gets called at the end of each scenario and is used to present * the results
 *and a rating of the player's battle.                                         *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/02/1994     : Created. *
 *=============================================================================================*/
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
  //	if (Keyboard != NULL) return;
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
  const int house = (PlayerPtr->Class->House == HOUSE_USSR ||
                     PlayerPtr->Class->House == HOUSE_UKRAINE)
                        ? 1
                        : 0;  // 0 or 1
  char inter_pal[15];
  absl::SNPrintF(inter_pal, sizeof(inter_pal), "SCORPAL1.PAL");

  ControlQ = false;
  FontXSpacing = 0;
  Map.Override_Mouse_Shape(MOUSE_NORMAL);
  Theme.Queue_Song(THEME_SCORE);

  VisiblePage.Clear();
  // SysMemPage.Clear();
  WWMouse->Erase_Mouse(&HidPage, true);
  HiddenPage.Clear();
  // Set_Logic_Page(SysMemPage);
  BlackPalette.Set();

  const auto country4 = MixArchive::RetrieveData("COUNTRY4.AUD");
  const auto sfx4 = MixArchive::RetrieveData("SFX4.AUD");
  Beepy6 = MixArchive::RetrieveData("BEEPY6.AUD");

  /*
  ** Load the background for the score screen
  */

  const int minutes = static_cast<int>(ElapsedTime / kTimerMinute) + 1;

  // Load up the shapes for the Nod score screen
  const auto yellowptr = MixArchive::RetrieveData("BAR3BHR.SHP");
  const auto redptr = MixArchive::RetrieveData("BAR3RHR.SHP");

  /* Change to the six-point font for Text_Print */
  const std::span<const std::byte> oldfont = Set_Font(ScoreFontPtr);
  Call_Back();

  /* --- Now display the background animation --- */
  Hide_Mouse();
  Load_Title_Screen(base::At(ScreenNames, house), &HidPage, ScorePalette);
  Increase_Palette_Luminance(ScorePalette, 30, 30, 30, 63);
  HidPage.Blit(SeenBuff);
  ScorePalette.Set(kFadePaletteFast, Call_Back);
  Play_Sample(country4, 255, Options.Normalize_Volume(150));

  /*
  ** Background's up, so now load various shapes and animations
  */
  const auto timeshape = MixArchive::RetrieveData("TIMEHR.SHP");
  const auto hiscore1shape = MixArchive::RetrieveData("HISC1-HR.SHP");
  const auto hiscore2shape = MixArchive::RetrieveData("HISC2-HR.SHP");
  ScoreObjs[0] = new ScoreTimeClass(238, 2, timeshape, 30, 4);
  ScoreObjs[1] = new ScoreTimeClass(4, 89, hiscore1shape, 10, 4);
  ScoreObjs[2] = new ScoreTimeClass(4, 180, hiscore2shape, 10, 4);

  /* Now display the stuff */
  Set_Logic_Page(SeenBuff);

  Alloc_Object(new ScorePrintClass(TXT_SCORE_TIME,
                                   config::kIsFrench ? 198 : 204, 9, greenpal));
  Alloc_Object(new ScorePrintClass(TXT_SCORE_LEAD, 164, 26, greenpal));
  Alloc_Object(new ScorePrintClass(TXT_SCORE_EFFI, 164, 38, greenpal));
  Alloc_Object(new ScorePrintClass(TXT_SCORE_TOTA, 164, 50, greenpal));
  Play_Sample(sfx4, 255, Options.Normalize_Volume(150));
  Call_Back_Delay(13);

  Keyboard->Clear();

  /*
  **	Determine leadership rating.
  */
  int leadership = 0;
  for (int index = 0; index < Logic.Count(); index++) {
    const ObjectClass* object = Logic[index];
    const HousesType owner = object->Owner();
    if (house &&
        (owner == HOUSE_USSR || owner == HOUSE_BAD || owner == HOUSE_UKRAINE)) {
      leadership++;
    } else {
      if (!house && object->Owner() == HOUSE_GREECE) {
        leadership++;
      }
    }
  }
  int uspoints = 0;

  for (HousesType hous = HOUSE_SPAIN; hous <= HOUSE_BAD; hous++) {
    const HouseClass* hows = HouseClass::As_Pointer(hous);
    if (hous == HOUSE_USSR || hous == HOUSE_BAD || hous == HOUSE_UKRAINE) {
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
  //	if(uspoints < 0) uspoints = 0;
  //	uspoints += 1000; //BG 1000 bonus points for winning mission

  /*
  **	Bias the base score upward according to the difficulty level.
  */
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

  if (!leadership) {
    leadership++;
  }
  leadership = 100 * fixed(leadership, house ? NKilled + NBKilled + leadership
                                             : GKilled + GBKilled + leadership);
  leadership = std::min(150, leadership);

  /*
  **	Determine economy rating.
  */
  int economy =
      100 *
      fixed(static_cast<int>(PlayerPtr->Available_Money()) + 1 +
                PlayerPtr->StolenBuildingsCredits,
            PlayerPtr->HarvestedCredits +
                static_cast<int>(PlayerPtr->Control.InitialCredits) + 1);
  economy = std::min(economy, 150);

  int total = (uspoints * leadership / 100) + (uspoints * economy / 100);
  total = std::clamp(total, -9999, 99999);

  Keyboard->Clear();
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
    // BG		if (Keyboard->Check()) break;
  }
  Count_Up_Print("%3d%%", leadership, leadership, 244, 26);
  Count_Up_Print("%3d%%", economy, economy, 244, 38);

  char buffer[16];
  absl::SNPrintF(buffer, sizeof(buffer), "x %5d", uspoints);
  Alloc_Object(new ScorePrintClass(buffer, 274, 26, greenpal));
  Alloc_Object(new ScorePrintClass(buffer, 274, 38, greenpal));
  Call_Back_Delay(8);
  SeenBuff.Draw_Line(548, 96, 626, 96, kWhite);
  Call_Back_Delay(1);
  SeenBuff.Draw_Line(548, 96, 626, 96, kGreen);

  absl::SNPrintF(buffer, sizeof(buffer), "%5d", total);
  Alloc_Object(new ScorePrintClass(buffer, 286, 50, greenpal));

  // BG	if (!Keyboard->Check()) {
  Call_Back_Delay(60);
  // BG	}

  if (house) {
    Show_Credits(house, greenpal);
  }

  /*BG	if (!Keyboard->Check()) */ Call_Back_Delay(60);

  /*
  ** Show stats on # of units killed
  */
  Set_Logic_Page(SeenBuff);
  Play_Sample(sfx4, 255, Options.Normalize_Volume(150));
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

  /*
  ** Print out stats on buildings destroyed
  */
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
  /*
  ** Hall of fame display and processing
  */
  Play_Sample(sfx4, 255, Options.Normalize_Volume(150));
  Alloc_Object(new ScorePrintClass(TXT_SCORE_TOP, 28, 110, greenpal));
  Call_Back_Delay(9);

  /*
  ** First check for the existence of the file, and if there isn't one,
  ** make a new one filled with blanks.
  */
  if (!file.IsAvailable()) {
    // hall of fame doesn't exist, so blank it out & write it
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

  /*
  ** If the player's score is good enough to bump someone off the list,
  ** remove their data, move everyone down a notch, and set index = where
  ** their info goes
  */
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

  /*
  ** Now display the hall of fame
  */
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
                                       HALLFAME_Y + (i * 8), pal, kBlack));
      if (base::At(hallfame, i).level < 20) {
        absl::SNPrintF(str.subspan(16).data(), str.size() - 16, "%d",
                       base::At(hallfame, i).level);
      } else {
        absl::SNPrintF(str.subspan(16).data(), str.size() - 16, "**");
      }
      Alloc_Object(new ScorePrintClass(str.subspan(16).data(),
                                       HALLFAME_X + (6 * 11),
                                       HALLFAME_Y + (i * 8), pal, kBlack));
      Call_Back_Delay(13);
    }
  }
  // Wait for text printing to complete
  while (StillUpdating) {
    Call_Back_Delay(1);
  }
  /*
  ** If the player's on the hall of fame, have him enter his name now
  */
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

  /* get rid of all the animating objects */
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
  int minclicks = 20;
  int64_t timingtime = TickCount.Value();
  SerialPacketType sendpacket;
  SerialPacketType receivepacket;
  int packetlen = 0;

  Keyboard->Clear();
  while (minclicks || (!Keyboard->Check() && !ControlQ)) {
    if (Session.Type == GAME_NULL_MODEM || Session.Type == GAME_MODEM) {
      //
      // send a timing packet if enough time has gone by.
      //
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
        // throw packet away
      }

      NullModem.Service();
    }

    Call_Back_Delay(1);
    if (minclicks) {
      minclicks--;
      Keyboard->Clear();
    }

    if (cycle) {
      counter = (counter + 1) % 8;
      if (counter == 0 && Options.IsPaletteScroll) {
        const RGBClass rgb = ScorePalette[233];
        for (int i = 233; i < 237; i++) {
          ScorePalette[i] = ScorePalette[i + 1];
        }
        ScorePalette[237] = rgb;
        ScorePalette.Set();
      }
    }
  }
  Keyboard->Clear();
}

// Not const: plays the score screen animation.
// NOLINTNEXTLINE(readability-make-member-function-const)
void ScoreClass::Do_Nod_Buildings_Graph() {
  const auto power_plant_shape = MixArchive::RetrieveData("POWR.SHP");
  const auto tanya_shape = MixArchive::RetrieveData("E7.SHP");
  const auto fireball_shape = MixArchive::RetrieveData("FBALL1.SHP");
  const InfantryTypeClass* ramboclass =
      &InfantryTypeClass::As_Reference(INFANTRY_TANYA);

  /*
  ** Print the # of buildings on the hidpage so we only need to do it once
  */
  SeenBuff.Blit(HidPage);
  Set_Logic_Page(HidPage);
  Call_Back_Delay(30);
  Set_Font_Palette(redpal);
  HidPage.Print(0, BUILDING_X + 16, BUILDING_Y + 10, kTBlack, kTBlack);
  Set_Font_Palette(bluepal);
  HidPage.Print(0, BUILDING_X + 16, BUILDING_Y + 22, kTBlack, kTBlack);

  /*
  ** Here's the animation/draw loop for blowing up the factory
  */
  for (int i = 0; i < 98; i++) {
    HidPage.Blit(HidPage, BUILDING_X, BUILDING_Y, 0, 0, 320 - BUILDING_X, 48);
    int shapenum = 0;  // no damage
    if (i >= 60) {
      shapenum = Extract_Shape_Count(power_plant_shape) - 2;  // some damage
      if (i == 60) {
        Sound_Effect(VOC_CRUMBLE);
      }
      if (i > 65) {
        shapenum = Extract_Shape_Count(power_plant_shape) - 1;  // mega damage
      }
    }

    /*
    ** Draw the building before Rambo
    */
    if (i < 68) {
      CC_Draw_Shape(power_plant_shape, shapenum, 0, 0, WINDOW_MAIN,
                    SHAPE_GHOST | SHAPE_FADING | SHAPE_WIN_REL,
                    ColorRemaps[PCOLOR_GOLD].RemapTable,
                    DisplayClass::UnitShadow);
    }

    /*
    ** Now draw some fires, if appropriate
    */
    if (i >= 61) {
      const int firecount = Extract_Shape_Count(fireball_shape);
      int shapeindex = (i - 61) / 2;
      if (shapeindex < firecount) {
        CC_Draw_Shape(fireball_shape, shapeindex, 10, 10, WINDOW_MAIN,
                      SHAPE_CENTER | SHAPE_WIN_REL);
      }
      if (i > 64) {
        shapeindex = (i - 64) / 2;
        if (shapeindex < firecount) {
          CC_Draw_Shape(fireball_shape, shapeindex, 50, 30, WINDOW_MAIN,
                        SHAPE_CENTER | SHAPE_WIN_REL);
        }
      }
    }
    /*
    ** Draw the Tanya character running away from the building
    */
    CC_Draw_Shape(
        tanya_shape,
        ramboclass->DoControls[static_cast<int>(DO_WALK)].Frame +
            (ramboclass->DoControls[static_cast<int>(DO_WALK)].Jump * 6) +
            ((i / 2) % ramboclass->DoControls[static_cast<int>(DO_WALK)].Count),
        i + 32, 40, WINDOW_MAIN,
        SHAPE_FADING | SHAPE_CENTER | SHAPE_WIN_REL | SHAPE_GHOST,
        ColorRemaps[PCOLOR_RED].RemapTable, DisplayClass::UnitShadow);
    HidPage.Blit(SeenBuff, 0, 0, BUILDING_X, BUILDING_Y, 320 - BUILDING_X, 48);
    Call_Back_Delay(1);
  }

  const int i = std::max(GBKilled, NBKilled);
  for (int q = 0; q <= i; q++) {
    Set_Font_Palette(redpal);
    Count_Up_Print("%d", q, NBKilled, BUILDING_X + 16, BUILDING_Y + 10);
    Set_Font_Palette(bluepal);
    Count_Up_Print("%d", q, GBKilled, BUILDING_X + 16, BUILDING_Y + 22);
    Play_Sample(Beepy6, 255, Options.Normalize_Volume(150));
    Call_Back_Delay(1);
  }
  Set_Font_Palette(redpal);
  Count_Up_Print("%d", NBKilled, NBKilled, BUILDING_X + 16, BUILDING_Y + 10);
  Set_Font_Palette(bluepal);
  Count_Up_Print("%d", GBKilled, GBKilled, BUILDING_X + 16, BUILDING_Y + 22);
}

/***************************************************************************
 * DO_GDI_GRAPH -- Show # of people or buildings killed on GDI score screen*
 *                                                                         *
 *                                                                         *
 *                                                                         *
 * INPUT:   yellowptr, redptr = pointers to shape file for graphs          *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   05/03/1995 BWG : Created.                                             *
 *=========================================================================*/

void ScoreClass::Do_GDI_Graph(std::span<const std::byte> yellowptr,
                              std::span<const std::byte> redptr, int gkilled,
                              int nkilled, int ypos) {
  const int xpos = 174;
  const int house = (PlayerPtr->Class->House == HOUSE_USSR ||
                     PlayerPtr->Class->House == HOUSE_UKRAINE)
                        ? 1
                        : 0;  // 0 or 1
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

  gdikilled = gdikilled * SIZEGBAR / maxval;
  nodkilled = nodkilled * SIZEGBAR / maxval;
  if (maxval < 20) {
    gdikilled = gkilled * 5;
    nodkilled = nkilled * 5;
  }

  maxval = std::max(gdikilled, nodkilled);
  if (!maxval) {
    maxval = 1;
  }

  // Draw the white-flash shape on the hidpage
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
    // BG		if (!Keyboard->Check()) {
    Play_Sample(Beepy6, 255, Options.Normalize_Volume(150));
    Call_Back_Delay(2);
    // BG		}
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
    // BG		if (!Keyboard->Check()) {
    Play_Sample(Beepy6, 255, Options.Normalize_Volume(150));
    Call_Back_Delay(2);
    // BG		}
  }

  //	if (Keyboard::Check()) Keyboard::Clear();

  /*
  ** Make sure accurate count is printed at end
  */
  CC_Draw_Shape(redptr, nodkilled, xpos * 2, (ypos + 12) * 2, WINDOW_MAIN,
                SHAPE_WIN_REL, {}, {});
  Count_Up_Print("%d", nkilled, nkilled, 297, ypos + 14);
  /*BG	if (!Keyboard->Check()) */ Call_Back_Delay(40);
}

// Not const: plays the score screen animation.
// NOLINTNEXTLINE(readability-make-member-function-const)
void ScoreClass::Do_Nod_Casualties_Graph() {
  const auto e1ptr = MixArchive::RetrieveData("E1.SHP");

  int gdikilled = GKilled;
  int nodkilled = NKilled;
  int maxval = std::max(gdikilled, nodkilled);

  if (!maxval) {
    maxval = 1;
  }
  if (gdikilled > MAX_BAR_X - BARGRAPH_X ||
      nodkilled > MAX_BAR_X - BARGRAPH_X) {
    gdikilled = gdikilled * (MAX_BAR_X - BARGRAPH_X) / maxval;
    nodkilled = nodkilled * (MAX_BAR_X - BARGRAPH_X) / maxval;
  }

  maxval = std::max(gdikilled, nodkilled);
  if (!maxval) {
    maxval = 1;
  }

  /*
  ** Initialize a bunch of objects for the infantrymen who pose for the bar
  ** graphs of casualties.
  */
  const int r = NUMINFANTRYMEN / 2;
  for (int i = 0; i < NUMINFANTRYMEN / 2; i++) {
    base::At(InfantryMan, i + 0).xpos = base::At(InfantryMan, i + r).xpos =
        (i * 10) + 7;
    base::At(InfantryMan, i + 0).ypos = 11;
    base::At(InfantryMan, i + r).ypos = 21;
    base::At(InfantryMan, i + 0).shapefile =
        base::At(InfantryMan, i + r).shapefile = e1ptr;
    base::At(InfantryMan, i + 0).remap = ColorRemaps[PCOLOR_RED].RemapTable;
    base::At(InfantryMan, i + r).remap = ColorRemaps[PCOLOR_BLUE].RemapTable;
    base::At(InfantryMan, i + 0).anim = base::At(InfantryMan, i + r).anim = 0;
    base::At(InfantryMan, i + 0).stage = base::At(InfantryMan, i + r).stage = 0;
    base::At(InfantryMan, i + 0).delay = base::At(InfantryMan, i + r).delay =
        static_cast<char>(local_rng.Next() % 32);
    base::At(InfantryMan, i + 0).Class = base::At(InfantryMan, i + r).Class =
        &InfantryTypeClass::As_Reference(INFANTRY_E1);
  }

  /*
  ** Draw the infantrymen and pause briefly before running the graph
  */
  Draw_InfantryMen();
  HidPage.Blit(SeenBuff, 0, 0, BARGRAPH_X, CASUALTY_Y, 320 - BARGRAPH_X, 34);
  Call_Back_Delay(40);

  for (int i = 1; i <= maxval; i++) {
    // Draw & update infantrymen 3 times for every tick on the graph (i)
    for (int index = 0; index < 3; index++) {
      Draw_InfantryMen();
      Draw_Bar_Graphs(i, nodkilled, gdikilled);
      HidPage.Blit(SeenBuff, 0, 0, BARGRAPH_X, CASUALTY_Y, 320 - BARGRAPH_X,
                   34);

      Set_Font_Palette(redpal);
      Count_Up_Print("%d", i * NKilled / maxval, NKilled, SCORETEXT_X + 64,
                     CASUALTY_Y + 2);
      Set_Font_Palette(bluepal);
      Count_Up_Print("%d", i * GKilled / maxval, GKilled, SCORETEXT_X + 64,
                     CASUALTY_Y + 14);
      /*BG			if (!Keyboard->Check()) */ Call_Back_Delay(3);
    }
    Play_Sample(Beepy6, 255, Options.Normalize_Volume(150));
  }
  // BG	if (Keyboard->Check()) Keyboard->Clear();

  /*
  ** Make sure accurate count is printed at end
  */
  Set_Font_Palette(redpal);
  Count_Up_Print("%d", NKilled, NKilled, SCORETEXT_X + 64, CASUALTY_Y + 2);
  Set_Font_Palette(bluepal);
  Count_Up_Print("%d", GKilled, GKilled, SCORETEXT_X + 64, CASUALTY_Y + 14);

  /*
  ** Finish up death animations, if there are any active
  */
  int k = 1;
  while (k) {
    for (int i = k = 0; i < NUMINFANTRYMEN; i++) {
      if (base::At(InfantryMan, i).anim >= kDoGunDeath) {
        k = 1;
      }
    }
    if (k) {
      Draw_InfantryMen();
    }
    Draw_Bar_Graphs(maxval, nodkilled, gdikilled);
    HidPage.Blit(SeenBuff, 0, 0, BARGRAPH_X, CASUALTY_Y, 320 - BARGRAPH_X, 34);
    Call_Back_Delay(1);
  }
}

void ScoreClass::Show_Credits(int house, std::span<const uint8_t> pal) {
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
  const int minval = static_cast<int>(PlayerPtr->Available_Money() / 100);

  /*
  ** Print out total credits left at end of scenario
  */
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

  delete base::At(ScoreObjs, credobj);
  base::At(ScoreObjs, credobj) = nullptr;
}

/***************************************************************************
 * SCORECLASS::PRINT_MINUTES -- Print out hours/minutes up to max          *
 *                                                                         *
 *    Same as count-up-print, but for the time                             *
 *                                                                         *
 * INPUT:   current minute count and maximum                               *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   04/13/1995 BWG : Created.                                             *
 *=========================================================================*/
void ScoreClass::Print_Minutes(int minutes) {
  char str[20];
  if (minutes >= 60) {
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

/***********************************************************************************************
 * ScoreClass::Count_Up_Print -- Prints a number (up to its max) into a string,
 *cleanly.       *
 *                                                                                             *
 *    This routine prints out a number (like 70) or its maximum number, into a
 *string,   onto  * the screen, on a clean section of the screen, and blits it
 *forward to the seenpage so you* can print without flashing and can print over
 *something (to count up %'s).               *
 *                                                                                             *
 * INPUT:   str = string to print into * percent = # to print * max = # to print
 *if percent > max                                                * xpos = x
 *pixel coord                                                             * ypos
 *= y pixel coord                                                             *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 04/07/1995 BWG : Created. *
 *=============================================================================================*/
void ScoreClass::Count_Up_Print(const char* str, int percent, int maxval,
                                int xpos, int ypos) {
  char destbuf[64];

  Format_Runtime_Text(destbuf, sizeof(destbuf), str,
                      percent <= maxval ? percent : maxval);
  SeenBuff.Print(destbuf, xpos * 2, ypos * 2, kTBlack, kBlack);
}

/***********************************************************************************************
 * ScoreClass::Input_Name -- Gets the name from the keyboard *
 *                                                                                             *
 *      This routine handles keyboard input, and does a nifty zooming letter
 *effect too.       *
 *                                                                                             *
 * INPUT:   str = string to put user's typing into * xpos = x pixel coord * ypos
 *= y pixel coord                                                             *
 *            pal  = text remapping palette to print using *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/15/1995 BWG : Created. *
 *=============================================================================================*/
void ScoreClass::Input_Name(std::span<char> str, int xpos, int ypos,
                            std::span<const uint8_t> pal) {
  int key = 0;
  int index = 0;

  const auto keystrok = MixArchive::RetrieveData("KEYSTROK.AUD");

  /*
  ** Ready the hidpage so it can restore background under zoomed letters
  */
  SeenBuff.Blit(HidPage);

  /*
  ** Put a copy of the high score area on a spare area of the hidpage, so
  ** we can use it to restore the letter's background instead of filling
  ** with black.
  */
  HidPage.Blit(HidPage, 0, 200, 0, 0, 200, 200);

  do {
    Call_Back();
    Animate_Score_Objs();
    Animate_Cursor(index, ypos);
    if (Keyboard->Check()) {
      key = KeyboardClass::To_ASCII(Keyboard->Get()) & 0xFF;
      Call_Back();

      if (index == MAX_FAMENAME_LENGTH - 2) {
        while (Keyboard->Check()) {
          Keyboard->Get();
        }
      }

      /*
      ** If they hit 'backspace' when they're on the last letter,
      ** turn it into a space instead.
      */
      if ((key == KA_BACKSPACE && index == MAX_FAMENAME_LENGTH - 2) &&
          (str[base::ToSize(index)] && str[base::ToSize(index)] != 32)) {
        key = 32;
      }

      if (key == KA_BACKSPACE) {  // if (key == KN_BACKSPACE) {
        if (index) {
          str[base::ToSize(--index)] = 0;

          const int xposindex6 = (xpos + (index * 6)) * 2;
          HidPage.Blit(SeenBuff, xposindex6, (ypos - 100) * 2, xposindex6,
                       ypos * 2, 12, 12);
          HidPage.Blit(HidPage, xposindex6, (ypos - 100) * 2, xposindex6,
                       ypos * 2, 12, 12);
        }

      } else if (key != KA_RETURN) {  // else if (key != KN_RETURN &&
                                      // key!=KN_KEYPAD_RETURN) {
        int ascii = key;              // ascii = KN_To_KA(key);
        if (ascii >= 'a' && ascii <= 'z') {
          ascii -= 'a' - 'A';
        }
        if ((ascii >= '!' && ascii <= KA_TILDA) || ascii == ' ') {
          HidPage.Blit(SeenBuff, (xpos + (index * 6)) * 2, (ypos - 100) * 2,
                       (xpos + (index * 6)) * 2, ypos * 2, 12, 12);
          HidPage.Blit(HidPage, (xpos + (index * 6)) * 2, (ypos - 100) * 2,
                       (xpos + (index * 6)) * 2, ypos * 2, 12, 12);
          str[base::ToSize(index)] = static_cast<char>(ascii);
          str[base::ToSize(index + 1)] = 0;

          Play_Sample(keystrok, 255, Options.Normalize_Volume(150));
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
  } while (
      key !=
      KA_RETURN);  //	} while(key != KN_RETURN && key!=KN_KEYPAD_RETURN);
}

void Animate_Cursor(int pos, int ypos) {
  static int _lastpos = 0;
  static int _state;
  static Timer<SystemTickSource> _timer;

  ypos += 6;  // move cursor to bottom of letter

  ypos *= 2;

  // If they moved the cursor, erase old one and force state=0, to make green
  // draw right away
  if (pos != _lastpos) {
    HidPage.Blit(SeenBuff, (HALLFAME_X + (_lastpos * 6)) * 2, ypos - 200,
                 (HALLFAME_X + (_lastpos * 6)) * 2, ypos, 12, 2);
    _lastpos = pos;
    _state = 0;
  }
  SeenBuff.Draw_Line((HALLFAME_X + (pos * 6)) * 2, ypos,
                     (HALLFAME_X + (pos * 6) + 5) * 2, ypos,
                     _state ? kLtBlue : kTBlack);
  /*
  ** Toggle the color of the cursor, green or black, if it's time to do so.
  */
  if (_timer.IsFinished()) {
    _state = _state == 0 ? 1 : 0;
    _timer.Set(5);
  }
}

/***************************************************************************
 * Draw_InfantryMen -- Draw all the guys on the score screen               *
 *                                                                         *
 *                                                                         *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   04/13/1995 BWG : Created.                                             *
 *=========================================================================*/
void Draw_InfantryMen() {

  // Only draw the infantrymen if we're playing USSR... Allies wouldn't execute
  //	people like that.

  /*
  ** First restore the background
  */
  HidPage.Blit(HidPage, BARGRAPH_X, CASUALTY_Y, 0, 0, 320 - BARGRAPH_X, 34);
  Set_Logic_Page(HidPage);

  /*
  ** Then draw all the infantrymen on the clean hidpage
  */
  for (int k = 0; k < NUMINFANTRYMEN; k++) {
    Draw_InfantryMan(k);
  }
  /*
  ** They'll all be blitted over to the seenpage after the graphs are drawn
  */
}

/***************************************************************************
 * Draw_InfantryMan -- Draw one guy in score screen, update animation      *
 *                                                                         *
 *    This routine draws one of the infantrymen in the "Casualties" area   *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   04/13/1995 BWG : Created.                                             *
 *=========================================================================*/
void Draw_InfantryMan(int index) {

  /* If the infantryman's dead, just abort this function */
  if (base::At(InfantryMan, index).anim == -1) {
    return;
  }

  const int stage =
      base::At(InfantryMan, index).stage +
      base::At(InfantryMan, index)
          .Class->DoControls[base::ToSize(base::At(InfantryMan, index).anim)]
          .Frame;

  CC_Draw_Shape(base::At(InfantryMan, index).shapefile, stage,
                base::At(InfantryMan, index).xpos,
                base::At(InfantryMan, index).ypos, WINDOW_MAIN,
                SHAPE_FADING | SHAPE_CENTER | SHAPE_WIN_REL | SHAPE_GHOST,
                base::At(InfantryMan, index).remap, DisplayClass::UnitShadow);
  /*
  ** see if it's time to run a new anim
  */
  if (--base::At(InfantryMan, index).delay <= 0) {
    base::At(InfantryMan, index).delay = 3;
    if (std::cmp_greater_equal(
            ++base::At(InfantryMan, index).stage,
            base::At(InfantryMan, index)
                .Class
                ->DoControls[base::ToSize(base::At(InfantryMan, index).anim)]
                .Count)) {
      /*
      ** was he playing a death anim? If so, and it's done, erase him
      */
      if (base::At(InfantryMan, index).anim >= kDoGunDeath) {
        base::At(InfantryMan, index).anim = -1;
      } else {
        New_Infantry_Anim(index, static_cast<int>(DO_STAND_READY));
      }
    }
  }
}

/***************************************************************************
 * New_Infantry_Anim -- Start up a new animation for one of the infantrymen*
 *                                                                         *
 *                                                                         *
 *                                                                         *
 * INPUT:   index: which of the 30 infantrymen to affect                   *
 *          anim:  which animation sequence to start him into              *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   04/13/1995 BWG : Created.                                             *
 *=========================================================================*/
void New_Infantry_Anim(int index, int anim) {
  base::At(InfantryMan, index).anim = anim;
  base::At(InfantryMan, index).stage = 0;
  if (anim >= kDoGunDeath) {
    base::At(InfantryMan, index).delay = 1;  // start right away
  } else {
    base::At(InfantryMan, index).delay =
        static_cast<char>(local_rng.Next() % 16);
  }
}

// Draws one frame of the two "Casualties" bar graphs on the score screen. The
// caller ramps `i` upward over many calls, so each call extends the bars a
// little further until they reach the final casualty counts. As a bar grows
// past one of the displayed infantrymen, that man is sent into a death
// animation.
//
// The two graphs share the InfantryMan[] array: its first half holds the men
// for the top (`gkilled`) graph, its second half the men for the bottom
// (`nkilled`) graph.
//
// i       - How far the graphs have filled so far (the caller's tick count).
// gkilled - Forces killed on the top side, pre-clamped to fit the graph width.
// nkilled - Forces killed on the bottom side, pre-clamped to fit the graph
// width.
//
// HISTORY:
//   04/13/1995 BWG : Created.
//   07/02/1996 BWG : Removed references to civilians.
void Draw_Bar_Graphs(int i, int gkilled, int nkilled) {
  // Top bar. Widths are doubled because the graph is drawn at hi-res (the
  // coordinates above are in the original 320-wide space).
  if (gkilled) {
    LogicPage->Fill_Rect(0, 0 + 8, 0 + (std::min(i, gkilled) * 2), 0 + 10,
                         kRed);
    LogicPage->Draw_Line(0 + 2, 0 + 12, (0 + std::min(i, gkilled) + 1) * 2,
                         0 + 12, kTBlack);
    LogicPage->Draw_Line((0 + std::min(i, gkilled) + 1) * 2, 0 + 10,
                         (0 + std::min(i, gkilled) + 1) * 2, 0 + 10, kTBlack);
    if (i <= gkilled) {
      // Each displayed infantryman stands for 11 ticks of the graph, so i / 11
      // is the man the bar has just reached. Kill him off unless he is already
      // dead or dying.
      const int anim = base::At(InfantryMan, i / 11).anim;
      if (anim != -1 && anim < kDoGunDeath) {
        if (i / 11) {
          // Cosmetic death animations use the non-sync RNG so they cannot
          // perturb game logic; pick one of the 4 gun-death variants at random.
          New_Infantry_Anim(i / 11, kDoGunDeath + (local_rng.Next() % 4));
        } else {
          New_Infantry_Anim(i / 11, kDoGunDeath);
        }
      }
    }
  }
  // Bottom bar. Same logic as the top bar, drawn 24 rows lower and indexing the
  // second half of InfantryMan[] (hence the NUMINFANTRYMEN / 2 offset).
  if (nkilled) {
    LogicPage->Fill_Rect(0, 0 + 32, 0 + (std::min(i, nkilled) * 2), 0 + 34,
                         kLtCyan);
    LogicPage->Draw_Line(0 + 2, 0 + 36, (0 + std::min(i, nkilled) + 1) * 2,
                         0 + 36, kTBlack);
    LogicPage->Draw_Line((0 + std::min(i, nkilled) + 1) * 2, 0 + 34,
                         (0 + std::min(i, nkilled) + 1) * 2, 0 + 34, kTBlack);
    if (i <= nkilled) {
      const int anim =
          base::At(InfantryMan, (NUMINFANTRYMEN / 2) + (i / 11)).anim;
      if (anim != -1 && anim < kDoGunDeath) {
        if (i / 11) {
          New_Infantry_Anim((NUMINFANTRYMEN / 2) + (i / 11),
                            kDoGunDeath + (local_rng.Next() % 4));
        } else {
          New_Infantry_Anim((NUMINFANTRYMEN / 2) + (i / 11), kDoGunDeath);
        }
      }
    }
  }
}

/***************************************************************************
 * Call_Back_Delay -- Combines Call_Back() and Delay() functions           *
 *                                                                         *
 *    This is just to cut down on code size and typing a little.           *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   04/13/1995 BWG : Created.                                             *
 *=========================================================================*/
void Call_Back_Delay(int time) {
  time = std::clamp(time, 0, 60);
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
  StreamLowImpact = true;
  do {
    if (callbackcd.IsFinished()) {
      Call_Back();
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

static char* Int_Print(int a) {
  static char str[10];

  absl::SNPrintF(str, sizeof(str), "%d", a);
  return str;
}

/***********************************************************************************************
 * Multi_Score_Presentation -- Multiplayer routine to display score screen. *
 *                                                                                             *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/11/1995  BWG: Created. *
 *=============================================================================================*/

void Multi_Score_Presentation() {
  unsigned char remap[16];
  auto* pseudoseenbuff = new GraphicBufferClass(320, 200, std::span<uint8_t>{});

  const int oldfontxspacing = FontXSpacing;

  FontXSpacing = 0;
  Map.Override_Mouse_Shape(MOUSE_NORMAL);
  //	Theme.Queue_Song(THEME_WIN);

  BlackPalette.Set();
  SeenBuff.Clear();
  HidPage.Clear();
  Hide_Mouse();
  void* anim =
      Open_Animation("MLTIPLYR.WSA", {}, 0L,
                     WSA_OPEN_FROM_MEM | WSA_OPEN_TO_PAGE, ScorePalette);
  /*
  ** Display the background animation
  */
  pseudoseenbuff->Clear();
  Animate_Frame(anim, *pseudoseenbuff, 1);
  for (int x = 0; x < 256; x++) {
    std::ranges::fill(base::At(PaletteInterpolationTable, x),
                      static_cast<uint8_t>(x));
  }
  Interpolate_2X_Scale(pseudoseenbuff, &SeenBuff, {});
  ScorePalette.Set(kFadePaletteFast, Call_Back);

  int frame = 1;
  while (frame < Get_Animation_Frame_Count(anim)) {
    Animate_Frame(anim, *pseudoseenbuff, frame++);
    Interpolate_2X_Scale(pseudoseenbuff, &SeenBuff, {});
    Call_Back_Delay(2);
  }
  Close_Animation(anim);

  /* Change to the six-point font for Text_Print */
  const std::span<const std::byte> oldfont = Set_Font(ScoreFontPtr);
  Call_Back();

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

  /*
  ** Move all the scores over a notch if there's more games than can be
  ** shown (which is known by Session.CurGame == MAX_MULTI_GAMES-1);
  */
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
      const PlayerColorType color = i.Color;
      remap[8] = ColorRemaps[color].FontRemap[11];
      remap[6] = ColorRemaps[color].FontRemap[12];
      remap[4] = ColorRemaps[color].FontRemap[13];
      remap[2] = ColorRemaps[color].FontRemap[14];
      remap[14] = ColorRemaps[color].FontRemap[15];

      Alloc_Object(new ScorePrintClass(i.Name, 15, y, remap));
      Call_Back_Delay(20);

      Alloc_Object(new ScorePrintClass(Int_Print(i.Wins), 118, y, remap));
      Call_Back_Delay(6);

      for (int k = 0; k <= std::min(Session.CurGame, MAX_MULTI_GAMES - 2);
           k++) {
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

  /* get rid of all the animating objects */
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
  ChangingGun = nullptr;
}
