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

/* $Header:   F:\projects\c&c\vcs\code\score.h_v   2.18   16 Oct 1995 16:46:18
 * JOE_BOSTIC  $ */
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

#ifndef CNC_RED_ALERT_TD_SCORE_H_
#define CNC_RED_ALERT_TD_SCORE_H_

#include <cstring>

#include "sdllib/gbuffer.h"
#include "sdllib/timer.h"
#include "sdllib/wwstd.h"
#include "tech/wwfile.h"

class ScoreClass {
 public:
  int Score;
  int NKilled;
  int GKilled;
  int CKilled;
  int NBKilled;
  int GBKilled;
  int CBKilled;
  int NHarvested;
  int GHarvested;
  int CHarvested;
  unsigned long ElapsedTime;

  void Init() { memset(this, 0, sizeof(ScoreClass)); }
  void Presentation();

  /*
  **	File I/O.
  */
  bool Load(FileClass& file);
  bool Save(FileClass& file);
  void Code_Pointers();
  void Decode_Pointers();

 protected:
 private:
  unsigned char* ChangingGun;

  void ScoreDelay(int ticks);
  void Pulse_Bar_Graph();
  void Print_Graph_Title(int, int);
  void Print_Minutes(int minutes);
  void Count_Up_Print(char* str, int percent, int max, int xpos, int ypos);
  void Show_Credits(int house, const unsigned char pal[]);
  void Do_GDI_Graph(const void* yellowptr, const void* redptr, int gdikilled,
                    int nodkilled, int ypos);
  void Do_Nod_Casualties_Graph();
  void Do_Nod_Buildings_Graph();
  void Input_Name(char str[], int xpos, int ypos, const unsigned char pal[]);
};

class ScoreAnimClass {
 public:
  ScoreAnimClass(int x, int y, const void* data);
  int XPos;
  int Stage;
  int YPos;
  CountDownTimerClass Timer;
  const void* DataPtr;
  virtual void Update() {}
  virtual ~ScoreAnimClass() {}
};

class ScoreCredsClass : public ScoreAnimClass {
 public:
  int MaxStage;
  int TimerReset;
  const void* CashTurn;
  const void* Clock1;

  void Update() override;
  ScoreCredsClass(int xpos, int ypos, const void* data, int max, int timer);
  ~ScoreCredsClass() override {}
};

class ScoreTimeClass : public ScoreAnimClass {
 public:
  int MaxStage;
  int TimerReset;
  void Update() override;
  ScoreTimeClass(int xpos, int ypos, const void* data, int max, int timer);
  ~ScoreTimeClass() override {}
};

class ScorePrintClass : public ScoreAnimClass {
 public:
  int Background;
  const void* PrimaryPalette;
  void Update() override;
  ScorePrintClass(const void* string, int xpos, int ypos, const void* palette,
                  int background = TBLACK);
  ScorePrintClass(int string, int xpos, int ypos, const void* palette,
                  int background = TBLACK);
  ~ScorePrintClass() override {}
};

class MultiStagePrintClass : public ScoreAnimClass {
 public:
  int Background;
  const void* PrimaryPalette;
  void Update() override;
  MultiStagePrintClass(const void* string, int xpos, int ypos,
                       const void* palette, int background = TBLACK);
  MultiStagePrintClass(int string, int xpos, int ypos, const void* palette,
                       int background = TBLACK);
  ~MultiStagePrintClass() override {}
};

class ScoreScaleClass : public ScoreAnimClass {
 public:
  const unsigned char* Palette;
  void Update() override;
  ScoreScaleClass(const void* data, int xpos, int ypos,
                  const unsigned char pal[]);
  ~ScoreScaleClass() override {}
};

#define MAXSCOREOBJS 8
extern ScoreAnimClass* ScoreObjs[MAXSCOREOBJS];

void Multi_Score_Presentation();

void Map_Selection();
void Bit_It_In(int x, int y, int w, int h, GraphicBufferClass* src,
               GraphicBufferClass* dest, int delay = 0, bool dagger = false);
void Call_Back_Delay(int time);
int Alloc_Object(ScoreAnimClass* obj);
extern GraphicBufferClass* PseudoSeenBuff;

#endif  // CNC_RED_ALERT_TD_SCORE_H_
