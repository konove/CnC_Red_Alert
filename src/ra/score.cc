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
#include <cstdio>
#include <cstring>

#include "base/types.h"
#include "ra/ccfile.h"
#include "ra/ccptr.h"
#include "ra/compat.h"
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
#include "sdllib/font.h"
#include "sdllib/keyboard.h"
#include "sdllib/shape.h"
#include "sdllib/wsa.h"
#include "sdllib/ww_audio.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/ww_win.h"
#include "tech/fixed.h"
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

struct InfantryAnim {
  int xpos;
  int ypos;
  const void* shapefile;
  const void* remap;
  int anim;
  int stage;
  char delay;
  const InfantryTypeClass* Class;
} InfantryMan[NUMINFANTRYMEN];
void Draw_InfantryMen();
void Draw_InfantryMan(int index);
void New_Infantry_Anim(int index, int anim);
void Draw_Bar_Graphs(int i, int gkilled, int nkilled);
void Animate_Cursor(int pos, int ypos);
void Animate_Score_Objs();
void Cycle_Wait_Click(bool cycle = true);

void Disable_Uncompressed_Shapes();
void Enable_Uncompressed_Shapes();

const void* Beepy6;
int ControlQ;  // cheat key to skip past score/mapsel screens
bool StillUpdating;

const char* ScreenNames[2] = {"ALIBACKH.PCX", "SOVBACKH.PCX"};

struct Fame {
  char name[MAX_FAMENAME_LENGTH];
  int score;
  int level;
  int side;
};

ScoreAnimClass* ScoreObjs[MAXSCOREOBJS];

ScoreAnimClass::ScoreAnimClass(int x, int y, const void* data) {
  XPos = x * 2;
  YPos = y * 2;
  AnimTimer.Set(0);
  DataPtr = data;
}

ScoreTimeClass::ScoreTimeClass(int xpos, int ypos, const void* data, int maxval,
                               int xtimer)
    : ScoreAnimClass(xpos, ypos, data) {
  Stage = 0;
  MaxStage = maxval;
  TimerReset = xtimer;
}

void ScoreTimeClass::Update() {
  GraphicViewPortClass* oldpage;
  if (AnimTimer.IsFinished()) {
    AnimTimer.Set(TimerReset);
    if (++Stage >= MaxStage) {
      Stage = 0;
    }
    oldpage = LogicPage;
    Set_Logic_Page(SeenBuff);
    CC_Draw_Shape(DataPtr, Stage, XPos, YPos, WINDOW_MAIN, SHAPE_WIN_REL,
                  nullptr, nullptr);
    Set_Logic_Page(oldpage);
  }
}

ScoreCredsClass::ScoreCredsClass(int xpos, int ypos, const void* data,
                                 int maxval, int xtimer)
    : ScoreAnimClass(xpos, ypos, data) {
  Stage = 0;
  MaxStage = maxval;
  TimerReset = xtimer;
  Clock1 = MFCD::Retrieve("CLOCK1.AUD");
  CashTurn = MFCD::Retrieve("CASHTURN.AUD");
}

void ScoreCredsClass::Update() {
  GraphicViewPortClass* oldpage;
  if (AnimTimer.IsFinished()) {
    AnimTimer.Set(TimerReset);
    if (++Stage >= MaxStage) {
      Stage = 0;
    }
    oldpage = LogicPage;
    Set_Logic_Page(SeenBuff);
    Play_Sample(Clock1, 255, Options.Normalize_Volume(130));
    CC_Draw_Shape(DataPtr, Stage, XPos, YPos, WINDOW_MAIN, SHAPE_WIN_REL,
                  nullptr, nullptr);
    Set_Logic_Page(oldpage);
  }
}

ScorePrintClass::ScorePrintClass(int string, int xpos, int ypos,
                                 const void* palette, int background)
    : ScoreAnimClass(xpos, ypos, Text_String(string)) {
  Background = background;
  PrimaryPalette = palette;
  Stage = 0;
}

ScorePrintClass::ScorePrintClass(const void* string, int xpos, int ypos,
                                 const void* palette, int background)
    : ScoreAnimClass(xpos, ypos, string) {
  Background = background;
  PrimaryPalette = palette;
  Stage = 0;
}

void ScorePrintClass::Update() {
  static char localstr[2] = {0, 0};
  static char _whitepal[] = {0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F,
                             0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F};

  if (Stage && ((char*)DataPtr)[Stage - 1] == 0) {
    for (int i = 0; i < MAXSCOREOBJS; i++) {
      if (ScoreObjs[i] == this) {
        ScoreObjs[i] = nullptr;
      }
    }
    delete this;
    return;
  }

  StillUpdating = true;
  if (AnimTimer.IsFinished()) {
    AnimTimer.Set(1);

    int pos = XPos + Stage * 12;
    // print the letter properly
    if (Stage) {
      Set_Font_Palette(PrimaryPalette);
      localstr[0] = ((char*)DataPtr)[Stage - 1];
      HidPage.Print(localstr, pos - 12, YPos, TBLACK, TBLACK);
      HidPage.Blit(SeenBuff, pos - 12, YPos - 2, pos - 12, YPos - 2, 14, 16);
    }
    if (((char*)DataPtr)[Stage]) {
      localstr[0] = ((char*)DataPtr)[Stage];
      Set_Font_Palette(_whitepal);
      SeenBuff.Print(localstr, pos, YPos - 1, TBLACK, TBLACK);
      SeenBuff.Print(localstr, pos, YPos + 1, TBLACK, TBLACK);
      SeenBuff.Print(localstr, pos + 1, YPos, TBLACK, TBLACK);
    }
    Stage++;
  }
}

ScoreScaleClass::ScoreScaleClass(const void* string, int xpos, int ypos,
                                 const unsigned char palette[])
    : ScoreAnimClass(xpos, ypos, string) {
  Palette = &palette[0];
  Stage = 0;
}

void ScoreScaleClass::Update() {
  static int _destx[] = {0, 80, 107, 134, 180, 228};
  static int _destw[] = {6, 20, 30, 40, 60, 80};

  /*
  ** Restore the background for the scaled-up letter
  */
  if (AnimTimer.IsFinished()) {
    AnimTimer.Set(1);
    if (Stage) {
      Set_Font_Palette(Palette);
      HidPage.Fill_Rect(0, 0, 14, 14, TBLACK);
      HidPage.Print((char*)DataPtr, 0, 0, TBLACK, TBLACK);
      HidPage.Scale(SeenBuff, 0, 0, _destx[Stage] * 2, YPos, 10, 12,
                    _destw[Stage] * 2, _destw[Stage] * 2, true);
      Stage--;
    } else {
      Set_Font_Palette(Palette);
      for (int i = 0; i < MAXSCOREOBJS; i++) {
        if (ScoreObjs[i] == this) {
          ScoreObjs[i] = nullptr;
        }
      }
      HidPage.Print((char*)DataPtr, XPos, YPos, TBLACK, TBLACK);
      HidPage.Blit(SeenBuff, XPos, YPos, XPos, YPos, 12, 12);
      delete this;
      return;
    }
  }
}

int Alloc_Object(ScoreAnimClass* obj) {
  int i, ret;

  for (i = ret = 0; i < MAXSCOREOBJS; i++) {
    if (!ScoreObjs[i]) {
      ScoreObjs[i] = obj;
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
static const unsigned char _bluepal[] = {0xC0, 0xC1, 0xC1, 0xC3, 0xC2, 0xC5,
                                         0xC3, 0xC7, 0xC4, 0xC9, 0xCA, 0xCB,
                                         0xCC, 0xCD, 0xC0, 0xCF};
static const unsigned char _greenpal[] = {0x70, 0x71, 0x7C, 0x73, 0x7D, 0x75,
                                          0x7E, 0x77, 0x7F, 0x79, 0x7A, 0x7B,
                                          0x7C, 0x7D, 0x7C, 0x7F};
static const unsigned char _redpal[] = {0xD0, 0xD1, 0xD7, 0xD3, 0xD9, 0xD5,
                                        0xDA, 0xD7, 0xDB, 0xD9, 0xDA, 0xDB,
                                        0xDC, 0xDD, 0xD6, 0xDF};
static const unsigned char _yellowpal[] = {0x0,  0x0, 0xEC, 0x0, 0xEB, 0x0,
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

  /*
  ** Fix for the score screen crash due to uncompressed shape buffer overflow.
  */
  Disable_Uncompressed_Shapes();
  int i;
  const void* yellowptr;
  const void* redptr;
  CCFileClass file(kFameFileName);
  struct Fame hallfame[NUMFAMENAMES];
  const void* oldfont;
  int oldfontxspacing = FontXSpacing;
  int house = PlayerPtr->Class->House == HOUSE_USSR ||
              PlayerPtr->Class->House == HOUSE_UKRAINE;  // 0 or 1
  char inter_pal[15];
  sprintf(inter_pal, "SCORPAL1.PAL");

  ControlQ = 0;
  FontXSpacing = 0;
  Map.Override_Mouse_Shape(MOUSE_NORMAL);
  Theme.Queue_Song(THEME_SCORE);

  VisiblePage.Clear();
  // SysMemPage.Clear();
  WWMouse->Erase_Mouse(&HidPage, true);
  HiddenPage.Clear();
  // Set_Logic_Page(SysMemPage);
  BlackPalette.Set();

  const void* country4 = MFCD::Retrieve("COUNTRY4.AUD");
  const void* sfx4 = MFCD::Retrieve("SFX4.AUD");
  Beepy6 = MFCD::Retrieve("BEEPY6.AUD");

  /*
  ** Load the background for the score screen
  */

  unsigned minutes =
      static_cast<unsigned>(ElapsedTime / (long)kTimerMinute) + 1;

  // Load up the shapes for the Nod score screen
  yellowptr = MFCD::Retrieve("BAR3BHR.SHP");
  redptr = MFCD::Retrieve("BAR3RHR.SHP");

  /* Change to the six-point font for Text_Print */
  oldfont = Set_Font(ScoreFontPtr);
  Call_Back();

  /* --- Now display the background animation --- */
  Hide_Mouse();
  Load_Title_Screen(ScreenNames[house], &HidPage, ScorePalette);
  Increase_Palette_Luminance(ScorePalette, 30, 30, 30, 63);
  HidPage.Blit(SeenBuff);
  ScorePalette.Set(kFadePaletteFast, Call_Back);
  Play_Sample(country4, 255, Options.Normalize_Volume(150));

  /*
  ** Background's up, so now load various shapes and animations
  */
  const void* timeshape = MFCD::Retrieve("TIMEHR.SHP");
  const void* hiscore1shape = MFCD::Retrieve("HISC1-HR.SHP");
  const void* hiscore2shape = MFCD::Retrieve("HISC2-HR.SHP");
  ScoreObjs[0] = new ScoreTimeClass(238, 2, timeshape, 30, 4);
  ScoreObjs[1] = new ScoreTimeClass(4, 89, hiscore1shape, 10, 4);
  ScoreObjs[2] = new ScoreTimeClass(4, 180, hiscore2shape, 10, 4);

  /* Now display the stuff */
  Set_Logic_Page(SeenBuff);

  Alloc_Object(new ScorePrintClass(
      TXT_SCORE_TIME, config::kIsFrench ? 198 : 204, 9, _greenpal));
  Alloc_Object(new ScorePrintClass(TXT_SCORE_LEAD, 164, 26, _greenpal));
  Alloc_Object(new ScorePrintClass(TXT_SCORE_EFFI, 164, 38, _greenpal));
  Alloc_Object(new ScorePrintClass(TXT_SCORE_TOTA, 164, 50, _greenpal));
  Play_Sample(sfx4, 255, Options.Normalize_Volume(150));
  Call_Back_Delay(13);

  Keyboard->Clear();

  /*
  **	Determine leadership rating.
  */
  int leadership = 0;
  for (int index = 0; index < Logic.Count(); index++) {
    ObjectClass* object = Logic[index];
    HousesType owner = object->Owner();
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
    HouseClass* hows = HouseClass::As_Pointer(hous);
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
      fixed(static_cast<unsigned>(PlayerPtr->Available_Money()) + 1 +
                PlayerPtr->StolenBuildingsCredits,
            PlayerPtr->HarvestedCredits +
                static_cast<unsigned>(PlayerPtr->Control.InitialCredits) + 1);
  economy = std::min(economy, 150);

  int total = uspoints * leadership / 100 + uspoints * economy / 100;
  total = std::clamp(total, -9999, 99999);

  Keyboard->Clear();
  for (i = 0; i <= 130; i++) {
    Set_Font_Palette(_greenpal);
    int lead = leadership * i / 100;
    Count_Up_Print("%3d%%", lead, leadership, 244, 26);
    if (i >= 30) {
      int econo = economy * (i - 30) / 100;
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
  sprintf(buffer, "x %5d", uspoints);
  Alloc_Object(new ScorePrintClass(buffer, 274, 26, _greenpal));
  Alloc_Object(new ScorePrintClass(buffer, 274, 38, _greenpal));
  Call_Back_Delay(8);
  SeenBuff.Draw_Line(548, 96, 626, 96, WHITE);
  Call_Back_Delay(1);
  SeenBuff.Draw_Line(548, 96, 626, 96, GREEN);

  sprintf(buffer, "%5d", total);
  Alloc_Object(new ScorePrintClass(buffer, 286, 50, _greenpal));

  // BG	if (!Keyboard->Check()) {
  Call_Back_Delay(60);
  // BG	}

  if (house) {
    Show_Credits(house, _greenpal);
  }

  /*BG	if (!Keyboard->Check()) */ Call_Back_Delay(60);

  /*
  ** Show stats on # of units killed
  */
  Set_Logic_Page(SeenBuff);
  Play_Sample(sfx4, 255, Options.Normalize_Volume(150));
  int indx = 0;
  Alloc_Object(new ScorePrintClass(TXT_SCORE_CASU, _casuax[indx], _casuay[indx],
                                   _greenpal));
  Call_Back_Delay(9);
  if (house) {
    Alloc_Object(
        new ScorePrintClass(TXT_SOVIET, _nodtxx[indx], _gditxy[indx], _redpal));
    Alloc_Object(new ScorePrintClass(TXT_ALLIES, _gditxx[indx], _nodtxy[indx],
                                     _bluepal));
  } else {
    Alloc_Object(new ScorePrintClass(TXT_ALLIES, _gditxx[indx], _gditxy[indx],
                                     _bluepal));
    Alloc_Object(
        new ScorePrintClass(TXT_SOVIET, _nodtxx[indx], _nodtxy[indx], _redpal));
  }
  Call_Back_Delay(6);

  Set_Font_Palette(_redpal);
  Do_GDI_Graph(yellowptr, redptr, GKilled + CKilled, NKilled, 89);

  Set_Logic_Page(SeenBuff);

  /*
  ** Print out stats on buildings destroyed
  */
  Play_Sample(sfx4, 255, Options.Normalize_Volume(150));
  Alloc_Object(new ScorePrintClass(TXT_SCORE_BUIL, 144, 126, _greenpal));
  Call_Back_Delay(9);
  if (house) {
    Alloc_Object(
        new ScorePrintClass(TXT_SOVIET, _gditxx[indx], _bldggy[indx], _redpal));
    Alloc_Object(new ScorePrintClass(TXT_ALLIES, _gditxx[indx], _bldgny[indx],
                                     _bluepal));
  } else {
    Alloc_Object(new ScorePrintClass(TXT_ALLIES, _gditxx[indx], _bldggy[indx],
                                     _bluepal));
    Alloc_Object(
        new ScorePrintClass(TXT_SOVIET, _gditxx[indx], _bldgny[indx], _redpal));
  }
  Call_Back_Delay(7);
  Do_GDI_Graph(yellowptr, redptr, GBKilled + CBKilled, NBKilled, 137);

  // Wait for text printing to complete
  while (StillUpdating) {
    Call_Back_Delay(1);
  }

  Keyboard->Clear();

  if (!house) {
    Show_Credits(house, _greenpal);
  }
  /*
  ** Hall of fame display and processing
  */
  Play_Sample(sfx4, 255, Options.Normalize_Volume(150));
  Alloc_Object(new ScorePrintClass(TXT_SCORE_TOP, 28, 110, _greenpal));
  Call_Back_Delay(9);

  /*
  ** First check for the existence of the file, and if there isn't one,
  ** make a new one filled with blanks.
  */
  if (!file.Is_Available()) {
    // hall of fame doesn't exist, so blank it out & write it
    file.Open(FileAccess::kWrite);

    for (i = 0; i < NUMFAMENAMES; i++) {
      hallfame[i].name[0] = static_cast<char>(hallfame[i].score = hallfame[i].level = 0);
      hallfame[i].side = 0;
      file.Write(&hallfame[i], sizeof(struct Fame));
    }

    file.Close();
  }

  file.Open(FileAccess::kRead);
  for (i = 0; i < NUMFAMENAMES; i++) {
    file.Read(&hallfame[i], sizeof(struct Fame));
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
  int index;
  for (index = 0; index < NUMFAMENAMES; index++) {
    if (total > hallfame[index].score) {
      if (index < NUMFAMENAMES - 1) {
        for (i = NUMFAMENAMES - 1; i > index; i--) {
          hallfame[i] = hallfame[i - 1];
        }
      }
      hallfame[index].score = total;
      hallfame[index].level = Scen.Scenario;
      hallfame[index].name[0] = 0;  // blank out the name
      hallfame[index].side = house;
      break;
    }
  }

  /*
  ** Now display the hall of fame
  */
  Set_Logic_Page(SeenBuff);

  char maststr[NUMFAMENAMES * 32];
  const unsigned char* pal;
  for (i = 0; i < NUMFAMENAMES; i++) {
    pal = hallfame[i].side ? _redpal : _bluepal;
    Alloc_Object(new ScorePrintClass(hallfame[i].name, HALLFAME_X,
                                     HALLFAME_Y + i * 8, pal));
    if (hallfame[i].score) {
      char* str = maststr + static_cast<base::ssize>(i) * 32;
      sprintf(str, "%d", hallfame[i].score);
      Alloc_Object(new ScorePrintClass(str, HALLFAME_X + 6 * 14,
                                       HALLFAME_Y + i * 8, pal, BLACK));
      if (hallfame[i].level < 20) {
        sprintf(str + 16, "%d", hallfame[i].level);
      } else {
        sprintf(str + 16, "**");
      }
      Alloc_Object(new ScorePrintClass(str + 16, HALLFAME_X + 6 * 11,
                                       HALLFAME_Y + i * 8, pal, BLACK));
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
    pal = hallfame[index].side ? _redpal : _bluepal;
    Input_Name(hallfame[index].name, HALLFAME_X, HALLFAME_Y + index * 8, pal);

    file.Open(FileAccess::kWrite);
    for (i = 0; i < NUMFAMENAMES; i++) {
      file.Write(&hallfame[i], sizeof(struct Fame));
    }
    file.Close();
  } else {
    Alloc_Object(new ScorePrintClass(TXT_CLICK_CONTINUE, 149, 190, _yellowpal));
    ControlQ = false;
    Cycle_Wait_Click();
  }

  Keyboard->Clear();

  /* get rid of all the animating objects */
  for (i = 0; i < MAXSCOREOBJS; i++) {
    if (ScoreObjs[i]) {
      delete ScoreObjs[i];
      ScoreObjs[i] = nullptr;
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
  ControlQ = 0;

  /*
  ** Fix for the score screen crash due to uncompressed shape buffer overflow.
  */
  Enable_Uncompressed_Shapes();
}

void Cycle_Wait_Click(bool cycle) {
  int counter = 0;
  int minclicks = 20;
  int64_t timingtime = TickCount.Value();
  SerialPacketType sendpacket;
  SerialPacketType receivepacket;
  int packetlen;

  Keyboard->Clear();
  while (minclicks || (!Keyboard->Check() && !ControlQ)) {
    if (Session.Type == GAME_NULL_MODEM || Session.Type == GAME_MODEM) {
      //
      // send a timing packet if enough time has gone by.
      //
      if (TickCount.Value() - timingtime > PACKET_TIMING_TIMEOUT) {
        memset(&sendpacket, 0, sizeof(SerialPacketType));
        sendpacket.Command = SERIAL_SCORE_SCREEN;
        sendpacket.ScenarioInfo.ResponseTime = static_cast<int>(NullModem.Response_Time());
        sendpacket.ID = Session.ModemType;

        NullModem.Send_Message(&sendpacket, sizeof(sendpacket), 0);
        timingtime = TickCount.Value();
      }

      if (NullModem.Get_Message(&receivepacket, &packetlen) > 0) {
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
      counter = ++counter & 7;
      if (counter == 0 && Options.IsPaletteScroll) {
        RGBClass rgb = ScorePalette[233];
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

void ScoreClass::Do_Nod_Buildings_Graph() {
  const auto power_plant_shape = MFCD::RetrieveData("POWR.SHP");
  const auto tanya_shape = MFCD::RetrieveData("E7.SHP");
  const auto fireball_shape = MFCD::RetrieveData("FBALL1.SHP");
  const InfantryTypeClass* ramboclass =
      &InfantryTypeClass::As_Reference(INFANTRY_TANYA);

  /*
  ** Print the # of buildings on the hidpage so we only need to do it once
  */
  SeenBuff.Blit(HidPage);
  Set_Logic_Page(HidPage);
  Call_Back_Delay(30);
  Set_Font_Palette(_redpal);
  HidPage.Print(0, BUILDING_X + 16, BUILDING_Y + 10, TBLACK, TBLACK);
  Set_Font_Palette(_bluepal);
  HidPage.Print(0, BUILDING_X + 16, BUILDING_Y + 22, TBLACK, TBLACK);

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
      int firecount = Extract_Shape_Count(fireball_shape);
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
    CC_Draw_Shape(tanya_shape,
                  ramboclass->DoControls[DO_WALK].Frame +
                      ramboclass->DoControls[DO_WALK].Jump * 6 +
                      (static_cast<unsigned>(i) >> 1) %
                          ramboclass->DoControls[DO_WALK].Count,
                  i + 32, 40, WINDOW_MAIN,
                  SHAPE_FADING | SHAPE_CENTER | SHAPE_WIN_REL | SHAPE_GHOST,
                  ColorRemaps[PCOLOR_RED].RemapTable, DisplayClass::UnitShadow);
    HidPage.Blit(SeenBuff, 0, 0, BUILDING_X, BUILDING_Y, 320 - BUILDING_X, 48);
    Call_Back_Delay(1);
  }

  int i = std::max(GBKilled, NBKilled);
  for (int q = 0; q <= i; q++) {
    Set_Font_Palette(_redpal);
    Count_Up_Print("%d", q, NBKilled, BUILDING_X + 16, BUILDING_Y + 10);
    Set_Font_Palette(_bluepal);
    Count_Up_Print("%d", q, GBKilled, BUILDING_X + 16, BUILDING_Y + 22);
    Play_Sample(Beepy6, 255, Options.Normalize_Volume(150));
    Call_Back_Delay(1);
  }
  Set_Font_Palette(_redpal);
  Count_Up_Print("%d", NBKilled, NBKilled, BUILDING_X + 16, BUILDING_Y + 10);
  Set_Font_Palette(_bluepal);
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

void ScoreClass::Do_GDI_Graph(const void* yellowptr, const void* redptr,
                              int gkilled, int nkilled, int ypos) {
  int i, maxval;
  int xpos = 174;
  int house = PlayerPtr->Class->House == HOUSE_USSR ||
              PlayerPtr->Class->House == HOUSE_UKRAINE;  // 0 or 1
  if (house) {
    int temp = gkilled;
    gkilled = nkilled;
    nkilled = temp;
    const void* tempptr = yellowptr;
    yellowptr = redptr;
    redptr = tempptr;
  }
  int gdikilled = gkilled, nodkilled = nkilled;

  maxval = std::max(gdikilled, nodkilled);
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
  HidPage.Fill_Rect(0, 0, 248, 18, TBLACK);
  CC_Draw_Shape(redptr, 119, 0, 0, WINDOW_MAIN, SHAPE_WIN_REL, nullptr,
                nullptr);
  Set_Logic_Page(SeenBuff);
  Set_Font_Palette(house ? _redpal : _bluepal);

  for (i = 1; i <= gdikilled; i++) {
    if (i != gdikilled) {
      CC_Draw_Shape(yellowptr, i, xpos * 2, ypos * 2, WINDOW_MAIN,
                    SHAPE_WIN_REL, nullptr, nullptr);
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
                SHAPE_WIN_REL, nullptr, nullptr);
  Count_Up_Print("%d", gkilled, gkilled, 297, ypos + 2);
  /*BG	if (!Keyboard->Check()) */ Call_Back_Delay(40);

  Set_Font_Palette(house ? _bluepal : _redpal);
  for (i = 1; i <= nodkilled; i++) {
    if (i != nodkilled) {
      CC_Draw_Shape(redptr, i, xpos * 2, (ypos + 12) * 2, WINDOW_MAIN,
                    SHAPE_WIN_REL, nullptr, nullptr);
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
                SHAPE_WIN_REL, nullptr, nullptr);
  Count_Up_Print("%d", nkilled, nkilled, 297, ypos + 14);
  /*BG	if (!Keyboard->Check()) */ Call_Back_Delay(40);
}

void ScoreClass::Do_Nod_Casualties_Graph() {
  int i, gdikilled, nodkilled, maxval;

  const void* e1ptr = MFCD::Retrieve("E1.SHP");

  gdikilled = GKilled;
  nodkilled = NKilled;
  maxval = std::max(gdikilled, nodkilled);

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
  int r = NUMINFANTRYMEN / 2;
  for (i = 0; i < NUMINFANTRYMEN / 2; i++) {
    InfantryMan[i + 0].xpos = InfantryMan[i + r].xpos = i * 10 + 7;
    InfantryMan[i + 0].ypos = 11;
    InfantryMan[i + r].ypos = 21;
    InfantryMan[i + 0].shapefile = InfantryMan[i + r].shapefile = e1ptr;
    InfantryMan[i + 0].remap = ColorRemaps[PCOLOR_RED].RemapTable;
    InfantryMan[i + r].remap = ColorRemaps[PCOLOR_BLUE].RemapTable;
    InfantryMan[i + 0].anim = InfantryMan[i + r].anim = 0;
    InfantryMan[i + 0].stage = InfantryMan[i + r].stage = 0;
    InfantryMan[i + 0].delay = InfantryMan[i + r].delay =
        static_cast<char>(local_rng.Next() & 0x1F);
    InfantryMan[i + 0].Class = InfantryMan[i + r].Class =
        &InfantryTypeClass::As_Reference(INFANTRY_E1);
  }

  /*
  ** Draw the infantrymen and pause briefly before running the graph
  */
  Draw_InfantryMen();
  HidPage.Blit(SeenBuff, 0, 0, BARGRAPH_X, CASUALTY_Y, 320 - BARGRAPH_X, 34);
  Call_Back_Delay(40);

  for (i = 1; i <= maxval; i++) {
    // Draw & update infantrymen 3 times for every tick on the graph (i)
    for (int index = 0; index < 3; index++) {
      Draw_InfantryMen();
      Draw_Bar_Graphs(i, nodkilled, gdikilled);
      HidPage.Blit(SeenBuff, 0, 0, BARGRAPH_X, CASUALTY_Y, 320 - BARGRAPH_X,
                   34);

      Set_Font_Palette(_redpal);
      Count_Up_Print("%d", i * NKilled / maxval, NKilled, SCORETEXT_X + 64,
                     CASUALTY_Y + 2);
      Set_Font_Palette(_bluepal);
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
  Set_Font_Palette(_redpal);
  Count_Up_Print("%d", NKilled, NKilled, SCORETEXT_X + 64, CASUALTY_Y + 2);
  Set_Font_Palette(_bluepal);
  Count_Up_Print("%d", GKilled, GKilled, SCORETEXT_X + 64, CASUALTY_Y + 14);

  /*
  ** Finish up death animations, if there are any active
  */
  int k = 1;
  while (k) {
    for (i = k = 0; i < NUMINFANTRYMEN; i++) {
      if (InfantryMan[i].anim >= DO_GUN_DEATH) {
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

void ScoreClass::Show_Credits(int house, const unsigned char pal[]) {
  static int _credsx[2] = {276, 276};
  static int _credsy[2] = {173, 58};
  static int _credpx[2] = {228, 236};
  static int _credpy[2] = {config::kIsGerman ? 181 : 189 - 12, 74};
  static int _credtx[2] = {config::kIsGerman ? 162 : 182,
                           config::kIsGerman ? 162 : 182};
  static int _credty[2] = {config::kIsGerman ? 173 : 179 - 12, 62};

  int credobj, i;
  int minval, add;

  const void* credshape =
      MFCD::Retrieve(house ? "CREDSUHR.SHP" : "CREDSAHR.SHP");

  Alloc_Object(new ScorePrintClass(TXT_SCORE_ENDCRED, _credtx[house],
                                   _credty[house], pal));
  Call_Back_Delay(15);

  credobj = Alloc_Object(
      new ScoreCredsClass(_credsx[house], _credsy[house], credshape, 32, 2));
  minval = static_cast<int>(PlayerPtr->Available_Money() / 100);

  /*
  ** Print out total credits left at end of scenario
  */
  i = -50;

  do {
    add = 5;
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
    Count_Up_Print("%d", i, static_cast<int>(PlayerPtr->Available_Money()), _credpx[house],
                   _credpy[house]);
    Call_Back_Delay(2);
  } while (i < PlayerPtr->Available_Money());

  delete ScoreObjs[credobj];
  ScoreObjs[credobj] = nullptr;
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
      minutes = 9 * 60 + 59;
    }
    Format_Runtime_Text(str, sizeof(str), Text_String(TXT_SCORE_TIMEFORMAT1),
                        minutes / 60, minutes % 60);
  } else {
    Format_Runtime_Text(str, sizeof(str), Text_String(TXT_SCORE_TIMEFORMAT2),
                        minutes);
  }
  SeenBuff.Print(str, 550, 18, TBLACK, TBLACK);
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
  SeenBuff.Print(destbuf, xpos * 2, ypos * 2, TBLACK, BLACK);
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
void ScoreClass::Input_Name(char str[], int xpos, int ypos,
                            const unsigned char pal[]) {
  int key = 0;
  int ascii, index = 0;

  const void* keystrok = MFCD::Retrieve("KEYSTROK.AUD");

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
      key = Keyboard->To_ASCII(Keyboard->Get()) & 0xFF;
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
      if (key == KA_BACKSPACE && index == MAX_FAMENAME_LENGTH - 2) {
        if (str[index] && str[index] != 32) {
          key = 32;
        }
      }
      if (key == KA_BACKSPACE) {  // if (key == KN_BACKSPACE) {
        if (index) {
          str[--index] = 0;

          int xposindex6 = (xpos + index * 6) * 2;
          HidPage.Blit(SeenBuff, xposindex6, (ypos - 100) * 2, xposindex6,
                       ypos * 2, 12, 12);
          HidPage.Blit(HidPage, xposindex6, (ypos - 100) * 2, xposindex6,
                       ypos * 2, 12, 12);
        }

      } else if (key != KA_RETURN) {  // else if (key != KN_RETURN &&
                                      // key!=KN_KEYPAD_RETURN) {
        ascii = key;                  // ascii = KN_To_KA(key);
        if (ascii >= 'a' && ascii <= 'z') {
          ascii -= 'a' - 'A';
        }
        if ((ascii >= '!' && ascii <= KA_TILDA) || ascii == ' ') {
          HidPage.Blit(SeenBuff, (xpos + index * 6) * 2, (ypos - 100) * 2,
                       (xpos + index * 6) * 2, ypos * 2, 12, 12);
          HidPage.Blit(HidPage, (xpos + index * 6) * 2, (ypos - 100) * 2,
                       (xpos + index * 6) * 2, ypos * 2, 12, 12);
          str[index] = static_cast<char>(ascii);
          str[index + 1] = 0;

          int objindex;
          Play_Sample(keystrok, 255, Options.Normalize_Volume(150));
          objindex = Alloc_Object(
              new ScoreScaleClass(str + index, xpos + index * 6, ypos, pal));
          while (ScoreObjs[objindex]) {
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
  static int _lastpos = 0, _state;
  static Timer<SystemTickSource> _timer;

  ypos += 6;  // move cursor to bottom of letter

  ypos *= 2;

  // If they moved the cursor, erase old one and force state=0, to make green
  // draw right away
  if (pos != _lastpos) {
    HidPage.Blit(SeenBuff, (HALLFAME_X + _lastpos * 6) * 2, ypos - 200,
                 (HALLFAME_X + _lastpos * 6) * 2, ypos, 12, 2);
    _lastpos = pos;
    _state = 0;
  }
  SeenBuff.Draw_Line((HALLFAME_X + pos * 6) * 2, ypos,
                     (HALLFAME_X + pos * 6 + 5) * 2, ypos,
                     _state ? LTBLUE : TBLACK);
  /*
  ** Toggle the color of the cursor, green or black, if it's time to do so.
  */
  if (_timer.IsFinished()) {
    _state ^= 1;
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
  int k;

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
  for (k = 0; k < NUMINFANTRYMEN; k++) {
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
  int stage;

  /* If the infantryman's dead, just abort this function */
  if (InfantryMan[index].anim == -1) {
    return;
  }

  stage = InfantryMan[index].stage +
          InfantryMan[index].Class->DoControls[InfantryMan[index].anim].Frame;

  CC_Draw_Shape(InfantryMan[index].shapefile, stage, InfantryMan[index].xpos,
                InfantryMan[index].ypos, WINDOW_MAIN,
                SHAPE_FADING | SHAPE_CENTER | SHAPE_WIN_REL | SHAPE_GHOST,
                InfantryMan[index].remap, DisplayClass::UnitShadow);
  /*
  ** see if it's time to run a new anim
  */
  if (--InfantryMan[index].delay <= 0) {
    InfantryMan[index].delay = 3;
    if (++InfantryMan[index].stage >=
        InfantryMan[index].Class->DoControls[InfantryMan[index].anim].Count) {
      /*
      ** was he playing a death anim? If so, and it's done, erase him
      */
      if (InfantryMan[index].anim >= DO_GUN_DEATH) {
        InfantryMan[index].anim = -1;
      } else {
        New_Infantry_Anim(index, DO_STAND_READY);
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
  InfantryMan[index].anim = anim;
  InfantryMan[index].stage = 0;
  if (anim >= DO_GUN_DEATH) {
    InfantryMan[index].delay = 1;  // start right away
  } else {
    InfantryMan[index].delay = static_cast<char>(local_rng.Next() & 15);
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
    LogicPage->Fill_Rect(0, 0 + 8, 0 + std::min(i, gkilled) * 2, 0 + 10, RED);
    LogicPage->Draw_Line(0 + 2, 0 + 12, (0 + std::min(i, gkilled) + 1) * 2,
                         0 + 12, TBLACK);
    LogicPage->Draw_Line((0 + std::min(i, gkilled) + 1) * 2, 0 + 10,
                         (0 + std::min(i, gkilled) + 1) * 2, 0 + 10, TBLACK);
    if (i <= gkilled) {
      // Each displayed infantryman stands for 11 ticks of the graph, so i / 11
      // is the man the bar has just reached. Kill him off unless he is already
      // dead or dying.
      int anim = InfantryMan[i / 11].anim;
      if (anim != -1 && anim < DO_GUN_DEATH) {
        if (i / 11) {
          // Cosmetic death animations use the non-sync RNG so they cannot
          // perturb game logic; pick one of the 4 gun-death variants at random.
          New_Infantry_Anim(i / 11, DO_GUN_DEATH + (local_rng.Next() & 3));
        } else {
          New_Infantry_Anim(i / 11, DO_GUN_DEATH);
        }
      }
    }
  }
  // Bottom bar. Same logic as the top bar, drawn 24 rows lower and indexing the
  // second half of InfantryMan[] (hence the NUMINFANTRYMEN / 2 offset).
  if (nkilled) {
    LogicPage->Fill_Rect(0, 0 + 32, 0 + std::min(i, nkilled) * 2, 0 + 34,
                         LTCYAN);
    LogicPage->Draw_Line(0 + 2, 0 + 36, (0 + std::min(i, nkilled) + 1) * 2,
                         0 + 36, TBLACK);
    LogicPage->Draw_Line((0 + std::min(i, nkilled) + 1) * 2, 0 + 34,
                         (0 + std::min(i, nkilled) + 1) * 2, 0 + 34, TBLACK);
    if (i <= nkilled) {
      int anim = InfantryMan[NUMINFANTRYMEN / 2 + i / 11].anim;
      if (anim != -1 && anim < DO_GUN_DEATH) {
        if (i / 11) {
          New_Infantry_Anim(NUMINFANTRYMEN / 2 + i / 11,
                            DO_GUN_DEATH + (local_rng.Next() & 3));
        } else {
          New_Infantry_Anim(NUMINFANTRYMEN / 2 + i / 11, DO_GUN_DEATH);
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

  if (!ControlQ) {
    if (Keyboard->Down(KN_LCTRL) && Keyboard->Down(KN_Q)) {
      ControlQ = 1;
      Keyboard->Clear();
    }
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
      if (SoundType) {
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
  for (int i = 0; i < MAXSCOREOBJS; i++) {
    if (ScoreObjs[i]) {
      ScoreObjs[i]->Update();
    }
  }
}

char* Int_Print(int a) {
  static char str[10];

  sprintf(str, "%d", a);
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
  char remap[16];
  GraphicBufferClass* pseudoseenbuff =
      new GraphicBufferClass(320, 200, static_cast<void*>(nullptr));

  int i, k;
  const void* oldfont;
  int oldfontxspacing = FontXSpacing;

  FontXSpacing = 0;
  Map.Override_Mouse_Shape(MOUSE_NORMAL);
  //	Theme.Queue_Song(THEME_WIN);

  BlackPalette.Set();
  SeenBuff.Clear();
  HidPage.Clear();
  Hide_Mouse();
  void* anim =
      Open_Animation("MLTIPLYR.WSA", nullptr, 0L,
                     WSA_OPEN_FROM_MEM | WSA_OPEN_TO_PAGE, ScorePalette);
  /*
  ** Display the background animation
  */
  pseudoseenbuff->Clear();
  Animate_Frame(anim, *pseudoseenbuff, 1);
  for (int x = 0; x < 256; x++) {
    memset(&PaletteInterpolationTable[x][0], x, 256);
  }
  Interpolate_2X_Scale(pseudoseenbuff, &SeenBuff, nullptr);
  ScorePalette.Set(kFadePaletteFast, Call_Back);

  int frame = 1;
  while (frame < Get_Animation_Frame_Count(anim)) {
    Animate_Frame(anim, *pseudoseenbuff, frame++);
    Interpolate_2X_Scale(pseudoseenbuff, &SeenBuff, nullptr);
    Call_Back_Delay(2);
  }
  Close_Animation(anim);

  /* Change to the six-point font for Text_Print */
  oldfont = Set_Font(ScoreFontPtr);
  Call_Back();

  Set_Logic_Page(SeenBuff);

  Alloc_Object(new ScorePrintClass(TXT_SCORE_TOP, config::kIsFrench ? 113 : 130,
                                   13, _greenpal));
  Call_Back_Delay(5);
  Alloc_Object(new ScorePrintClass(TXT_COMMANDER, 27, 31, _greenpal));
  Call_Back_Delay(10);
  Alloc_Object(new ScorePrintClass(TXT_BATTLES_WON,
                                   config::kIsFrench   ? 113
                                   : config::kIsGerman ? 118
                                                       : 126,
                                   31, _greenpal));
  Call_Back_Delay(13);
  Alloc_Object(new ScorePrintClass(TXT_KILLS_COLON, 249, 31, _greenpal));
  Call_Back_Delay(6);

  /*
  ** Move all the scores over a notch if there's more games than can be
  ** shown (which is known by Session.CurGame == MAX_MULTI_GAMES-1);
  */
  if (Session.CurGame == MAX_MULTI_GAMES - 1) {
    for (i = 0; i < MAX_MULTI_NAMES; i++) {
      for (k = 0; k < MAX_MULTI_GAMES - 1; k++) {
        Session.Score[i].Kills[k] = Session.Score[i].Kills[k + 1];
      }
    }
  }

  int y = 41;
  for (i = 0; i < MAX_MULTI_NAMES; i++) {
    if (strlen(Session.Score[i].Name)) {
      int color = Session.Score[i].Color;
      remap[8] = ColorRemaps[color].FontRemap[11];
      remap[6] = ColorRemaps[color].FontRemap[12];
      remap[4] = ColorRemaps[color].FontRemap[13];
      remap[2] = ColorRemaps[color].FontRemap[14];
      remap[14] = ColorRemaps[color].FontRemap[15];

      Alloc_Object(new ScorePrintClass(Session.Score[i].Name, 15, y, remap));
      Call_Back_Delay(20);

      Alloc_Object(
          new ScorePrintClass(Int_Print(Session.Score[i].Wins), 118, y, remap));
      Call_Back_Delay(6);

      for (k = 0; k <= std::min(Session.CurGame, MAX_MULTI_GAMES - 2); k++) {
        if (Session.Score[i].Kills[k] >= 0) {
          Alloc_Object(new ScorePrintClass(Int_Print(Session.Score[i].Kills[k]),
                                           225 + 24 * k, y, remap));
          Call_Back_Delay(6);
        }
      }
      y += 12;
    }
  }

  Alloc_Object(new ScorePrintClass(
      TXT_CLICK_CONTINUE, config::kIsEnglish ? 109 : 95, 190, _yellowpal));
  Cycle_Wait_Click(false);

  /* get rid of all the animating objects */
  for (i = 0; i < MAXSCOREOBJS; i++) {
    if (ScoreObjs[i]) {
      delete ScoreObjs[i];
      ScoreObjs[i] = nullptr;
    }
  }

  Theme.Queue_Song(THEME_NONE);

  BlackPalette.Set(kFadePaletteFast, nullptr);
  SeenBuff.Clear();
  GamePalette.Set();
  Set_Font(oldfont);
  FontXSpacing = oldfontxspacing;
  ControlQ = 0;
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
