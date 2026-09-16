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

/* $Header: /CounterStrike/SCORE.H 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : SCORE.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : April 19, 1994 *
 *                                                                                             *
 *                  Last Update : April 19, 1994   [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_RA_SCORE_H_
#define CNC_RED_ALERT_RA_SCORE_H_

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "absl/base/attributes.h"
#include "ra/jshell.h"
#include "sdllib/gbuffer.h"
#include "sdllib/wwstd.h"
#include "tech/ftimer.h"

class ScoreClass {
 public:
  // Field-wise saved-game state; read and write share this field list.
  template <class Archive>
  void Serialize(Archive& ar);

  ScoreClass() = default;

  int Score = 0;
  int NKilled = 0;
  int GKilled = 0;
  int CKilled = 0;
  int NBKilled = 0;
  int GBKilled = 0;
  int CBKilled = 0;
  int NHarvested = 0;
  int GHarvested = 0;
  int CHarvested = 0;
  int64_t ElapsedTime = 0;
  Stopwatch<SystemTickSource> RealTime;

  void Init();
  void Presentation();

  /*
  **	File I/O.
  */

 private:
  unsigned char* ChangingGun = nullptr;

  void ScoreDelay(int ticks);
  void Pulse_Bar_Graph();
  void Print_Graph_Title(int, int);
  static void Print_Minutes(int minutes);
  static void Count_Up_Print(const char* str, int percent, int max, int xpos,
                             int ypos);
  static void Show_Credits(int house, std::span<const uint8_t> pal);
  static void Do_GDI_Graph(std::span<const std::byte> yellowptr,
                           std::span<const std::byte> redptr, int gkilled,
                           int nkilled, int ypos);
  void Do_Nod_Casualties_Graph();
  void Do_Nod_Buildings_Graph();
  static void Input_Name(std::span<char> str, int xpos, int ypos,
                         std::span<const uint8_t> pal);
};

class ScoreAnimClass {
 public:
  ScoreAnimClass(int x, int y,
                 std::span<const std::byte> data ABSL_ATTRIBUTE_LIFETIME_BOUND);
  ScoreAnimClass(int x, int y,
                 std::string_view text ABSL_ATTRIBUTE_LIFETIME_BOUND);
  int XPos;
  int YPos;
  Timer<SystemTickSource> AnimTimer;
  std::span<const std::byte> DataPtr;
  std::string_view TextData;
  // The animation's text, for the text-style animations.
  [[nodiscard]] std::string_view Text() const { return TextData; }
  virtual void Update() {}
  virtual ~ScoreAnimClass() = default;
  ScoreAnimClass(const ScoreAnimClass&) = delete;
  ScoreAnimClass& operator=(const ScoreAnimClass&) = delete;
  ScoreAnimClass(ScoreAnimClass&&) = delete;
  ScoreAnimClass& operator=(ScoreAnimClass&&) = delete;
};

class ScoreCredsClass : public ScoreAnimClass {
 public:
  int Stage{0};
  int MaxStage;
  int TimerReset;
  std::span<const std::byte> CashTurn;
  std::span<const std::byte> Clock1;

  void Update() override;
  ScoreCredsClass(int xpos, int ypos, std::span<const std::byte> data, int max,
                  int timer);
  ~ScoreCredsClass() override {
    CashTurn = {};
    Clock1 = {};
  }
  ScoreCredsClass(const ScoreCredsClass&) = delete;
  ScoreCredsClass& operator=(const ScoreCredsClass&) = delete;
  ScoreCredsClass(ScoreCredsClass&&) = delete;
  ScoreCredsClass& operator=(ScoreCredsClass&&) = delete;
};

class ScoreTimeClass : public ScoreAnimClass {
 public:
  int Stage{0};
  int MaxStage;
  int TimerReset;
  void Update() override;
  ScoreTimeClass(int xpos, int ypos, std::span<const std::byte> data, int max,
                 int timer);
  ~ScoreTimeClass() override = default;
  ScoreTimeClass(const ScoreTimeClass&) = delete;
  ScoreTimeClass& operator=(const ScoreTimeClass&) = delete;
  ScoreTimeClass(ScoreTimeClass&&) = delete;
  ScoreTimeClass& operator=(ScoreTimeClass&&) = delete;
};

class ScorePrintClass : public ScoreAnimClass {
 public:
  int Background;
  int Stage;
  std::span<const uint8_t> PrimaryPalette;
  void Update() override;
  ScorePrintClass(std::string_view string, int xpos, int ypos,
                  std::span<const uint8_t> palette
                      ABSL_ATTRIBUTE_LIFETIME_BOUND,
                  int background = kTBlack);
  ScorePrintClass(int string, int xpos, int ypos,
                  std::span<const uint8_t> palette
                      ABSL_ATTRIBUTE_LIFETIME_BOUND,
                  int background = kTBlack);
  ~ScorePrintClass() override { PrimaryPalette = {}; }
  ScorePrintClass(const ScorePrintClass&) = delete;
  ScorePrintClass& operator=(const ScorePrintClass&) = delete;
  ScorePrintClass(ScorePrintClass&&) = delete;
  ScorePrintClass& operator=(ScorePrintClass&&) = delete;
};

class ScoreScaleClass : public ScoreAnimClass {
 public:
  int Stage{0};
  std::span<const uint8_t> Palette;
  void Update() override;
  ScoreScaleClass(std::string_view string, int xpos, int ypos,
                  std::span<const uint8_t> pal ABSL_ATTRIBUTE_LIFETIME_BOUND);
  ~ScoreScaleClass() override { Palette = {}; }
  ScoreScaleClass(const ScoreScaleClass&) = delete;
  ScoreScaleClass& operator=(const ScoreScaleClass&) = delete;
  ScoreScaleClass(ScoreScaleClass&&) = delete;
  ScoreScaleClass& operator=(ScoreScaleClass&&) = delete;
};

#define MAXSCOREOBJS 8
extern ScoreAnimClass* ScoreObjs[MAXSCOREOBJS];

void Multi_Score_Presentation();

void Bit_It_In(int x, int y, int w, int h, GraphicBufferClass* src,
               GraphicBufferClass* dest, int delay = 0, int dagger = 0);
void Call_Back_Delay(int time);
int Alloc_Object(ScoreAnimClass* obj);

class ArchiveReader;
class ArchiveWriter;
extern template void ScoreClass::Serialize<ArchiveWriter>(ArchiveWriter&);
extern template void ScoreClass::Serialize<ArchiveReader>(ArchiveReader&);

#endif  // CNC_RED_ALERT_RA_SCORE_H_
