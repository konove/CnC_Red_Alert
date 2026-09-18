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

// End-of-mission score screens.
//
// ScoreClass holds the statistics of the campaign mission in progress and, once
// the mission is won, plays the single-player score screen: leadership and
// economy ratings, casualty and building bar graphs, remaining credits and the
// hall of fame. Multi_Score_Presentation() is the simpler multiplayer tally.
// Both screens animate their text and shapes through the small ScoreAnimClass
// hierarchy, which Call_Back_Delay() ticks while the presentation code waits.
//
// Originally SCORE.H by Joe L. Bostic, started April 19, 1994.

#ifndef CNC_RED_ALERT_RA_SCORE_H_
#define CNC_RED_ALERT_RA_SCORE_H_

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "absl/base/attributes.h"
#include "ra/jshell.h"
#include "tech/ftimer.h"

// Statistics for the mission in progress plus the score screen that presents
// them. There is one instance, the global `Score`; it is part of the saved
// game.
//
// The N/G/C prefixes are inherited from Tiberian Dawn (Nod, GDI, civilian). In
// Red Alert "N" is the Soviet side (the Soviet houses and HOUSE_BAD) and "G" is
// every other house.
class ScoreClass {
 public:
  // Field-wise saved-game state; read and write share this field list.
  template <class Archive>
  void Serialize(Archive& ar);

  ScoreClass() = default;

  // Never written by Red Alert; the total is computed inside Presentation().
  int Score = 0;

  // Units (*Killed) and buildings (*BKilled) each side has lost. Presentation()
  // adds the houses' loss counters into these, so they are only meaningful once
  // the score screen has run. The civilian counters are never written in Red
  // Alert and stay 0.
  int NKilled = 0;
  int GKilled = 0;
  int CKilled = 0;
  int NBKilled = 0;
  int GBKilled = 0;
  int CBKilled = 0;

  // Tiberian Dawn leftovers: never written in Red Alert, only saved and loaded.
  int NHarvested = 0;
  int GHarvested = 0;
  int CHarvested = 0;

  // Game time played, in timer ticks (kTimerSecond per second). The main loop
  // advances it once per game frame, so it follows game speed, not the wall
  // clock.
  int64_t ElapsedTime = 0;

  // Wall-clock counterpart of ElapsedTime. Reset by Init() and saved, but
  // nothing reads it.
  Stopwatch<SystemTickSource> RealTime;

  // Zeroes every statistic. Called when a scenario starts.
  void Init();

  // Plays the single-player score screen and returns when the player has
  // entered a hall of fame name or clicked to continue. Called at the end of a
  // won campaign mission; rates the player's battle, updates the hall of fame
  // file, and restores the game palette, font and mouse before returning.
  void Presentation();

 private:
  // Prints the mission time next to the clock animation, as hours and minutes
  // once it reaches an hour. The display tops out at 9:59.
  static void Print_Minutes(int minutes);

  // Formats min(percent, max) with the one-integer run-time format `str` and
  // prints it straight to the visible page over a solid black background, so a
  // counter can be reprinted in place every tick without flashing. `xpos` and
  // `ypos` are 320x200 coordinates.
  static void Count_Up_Print(const char* str, int percent, int max, int xpos,
                             int ypos);

  // Prints the "ending credits" caption, then counts the player's remaining
  // money up to its final value beside a spinning credits animation. `house`
  // is 0 for an Allied player and 1 for a Soviet one; it selects the artwork
  // and where on that side's background the readout goes.
  static void Show_Credits(int house, std::span<const uint8_t> pal);

  // Grows a pair of horizontal bar graphs, one after the other, each with a
  // number counting up beside it: `gkilled` Allied and `nkilled` Soviet losses
  // (units or buildings). The player's own side is always the top bar, at
  // 320x200 row `ypos`; the other follows 12 rows below. `yellowptr` and
  // `redptr` are the bar shape files for the top and bottom bar, whose frame N
  // is a bar N pixels long.
  static void Do_GDI_Graph(std::span<const std::byte> yellowptr,
                           std::span<const std::byte> redptr, int gkilled,
                           int nkilled, int ypos);

  // Lets the player type a hall of fame name into `str` until Return is
  // pressed. Letters are upper-cased and echoed at 320x200 position (`xpos`,
  // `ypos`) in font palette `pal`. `str` must hold MAX_FAMENAME_LENGTH chars.
  static void Input_Name(std::span<char> str, int xpos, int ypos,
                         std::span<const uint8_t> pal);
};

// Base class for the things that animate on the score screens. An object is
// heap-allocated, parked in ScoreObjs[] with Alloc_Object(), and has Update()
// called on every pass of Call_Back_Delay(). Text animations delete themselves
// and clear their slot when finished; the looping shape animations live until
// the screen tears ScoreObjs[] down.
//
// Constructors take 320x200 coordinates and double them; XPos and YPos are
// hi-res pixels. The data and text are viewed, not copied, so they must outlive
// the object.
class ScoreAnimClass {
 public:
  ScoreAnimClass(int x, int y,
                 std::span<const std::byte> data ABSL_ATTRIBUTE_LIFETIME_BOUND);
  ScoreAnimClass(int x, int y,
                 std::string_view text ABSL_ATTRIBUTE_LIFETIME_BOUND);
  int XPos;
  int YPos;
  // Counts down to the next animation step; Update() does nothing until then.
  Timer<SystemTickSource> AnimTimer;
  // Shape file, for the shape-style animations; empty otherwise.
  std::span<const std::byte> DataPtr;
  // Text, for the text-style animations; empty otherwise.
  std::string_view TextData;
  // The animation's text, for the text-style animations.
  [[nodiscard]] std::string_view Text() const { return TextData; }
  // Advances the animation if AnimTimer has run out. May `delete this`, so the
  // caller must not touch the object afterwards.
  virtual void Update() {}
  virtual ~ScoreAnimClass() = default;
  ScoreAnimClass(const ScoreAnimClass&) = delete;
  ScoreAnimClass& operator=(const ScoreAnimClass&) = delete;
  ScoreAnimClass(ScoreAnimClass&&) = delete;
  ScoreAnimClass& operator=(ScoreAnimClass&&) = delete;
};

// The credits readout's animation: loops like a ScoreTimeClass and also plays
// a tick sound on every frame. Show_Credits() deletes it when the count ends.
class ScoreCredsClass : public ScoreAnimClass {
 public:
  int Stage{0};    // Current frame.
  int MaxStage;    // Frame count; Stage wraps to 0 on reaching it.
  int TimerReset;  // Ticks per frame.
  std::span<const std::byte> Clock1;  // The per-frame tick sound.

  void Update() override;
  ScoreCredsClass(int xpos, int ypos, std::span<const std::byte> data, int max,
                  int timer);
  ~ScoreCredsClass() override { Clock1 = {}; }
  ScoreCredsClass(const ScoreCredsClass&) = delete;
  ScoreCredsClass& operator=(const ScoreCredsClass&) = delete;
  ScoreCredsClass(ScoreCredsClass&&) = delete;
  ScoreCredsClass& operator=(ScoreCredsClass&&) = delete;
};

// A shape animation that loops forever (the clock and the two hall of fame
// ornaments), drawn straight onto the visible page.
class ScoreTimeClass : public ScoreAnimClass {
 public:
  int Stage{0};    // Current frame.
  int MaxStage;    // Frame count; Stage wraps to 0 on reaching it.
  int TimerReset;  // Ticks per frame.
  void Update() override;
  ScoreTimeClass(int xpos, int ypos, std::span<const std::byte> data, int max,
                 int timer);
  ~ScoreTimeClass() override = default;
  ScoreTimeClass(const ScoreTimeClass&) = delete;
  ScoreTimeClass& operator=(const ScoreTimeClass&) = delete;
  ScoreTimeClass(ScoreTimeClass&&) = delete;
  ScoreTimeClass& operator=(ScoreTimeClass&&) = delete;
};

// Types a string onto the screen one letter per tick: each new letter first
// appears as a white smear, then is reprinted cleanly in `palette` on the next
// tick. Deletes itself after the last letter. While any of these is still
// typing, the file-local StillUpdating flag in score.cc stays set.
//
// The string is given as text or as a TXT_ string table id.
class ScorePrintClass : public ScoreAnimClass {
 public:
  int Stage;  // Index of the next letter to appear.
  std::span<const uint8_t> PrimaryPalette;  // Font palette of the final text.
  void Update() override;
  ScorePrintClass(std::string_view string, int xpos, int ypos,
                  std::span<const uint8_t> palette
                      ABSL_ATTRIBUTE_LIFETIME_BOUND);
  ScorePrintClass(int string, int xpos, int ypos,
                  std::span<const uint8_t> palette
                      ABSL_ATTRIBUTE_LIFETIME_BOUND);
  ~ScorePrintClass() override { PrimaryPalette = {}; }
  ScorePrintClass(const ScorePrintClass&) = delete;
  ScorePrintClass& operator=(const ScorePrintClass&) = delete;
  ScorePrintClass(ScorePrintClass&&) = delete;
  ScorePrintClass& operator=(ScorePrintClass&&) = delete;
};

// Echoes one typed hall of fame letter. In the DOS game the letter zoomed in
// from a large size over five steps; see Stage.
class ScoreScaleClass : public ScoreAnimClass {
 public:
  // Zoom steps left. The DOS build started at 5; the Windows build, and so this
  // port, starts at 0, which skips the zoom and prints the letter at once.
  int Stage{0};
  std::span<const uint8_t> Palette;  // Font palette of the letter.
  void Update() override;
  ScoreScaleClass(std::string_view string, int xpos, int ypos,
                  std::span<const uint8_t> pal ABSL_ATTRIBUTE_LIFETIME_BOUND);
  ~ScoreScaleClass() override { Palette = {}; }
  ScoreScaleClass(const ScoreScaleClass&) = delete;
  ScoreScaleClass& operator=(const ScoreScaleClass&) = delete;
  ScoreScaleClass(ScoreScaleClass&&) = delete;
  ScoreScaleClass& operator=(ScoreScaleClass&&) = delete;
};

// The live score screen animations; nullptr marks a free slot. Eight is enough
// only because the presentation code paces its Alloc_Object() calls with
// delays that let earlier text finish.
#define MAXSCOREOBJS 8
extern ScoreAnimClass* ScoreObjs[MAXSCOREOBJS];

// Plays the multiplayer score screen: every player's name, games won and kills
// per game, then waits for a key or click. Restores the game palette, font and
// mouse before returning.
void Multi_Score_Presentation();

// Waits `time` timer ticks (clamped to 0..60, i.e. one second) while keeping
// the score screen alive: it services sound and video and updates every object
// in ScoreObjs[]. Even a zero delay runs one update pass. Once Ctrl-Q has been
// seen, every delay is cut to zero until the screen finishes.
void Call_Back_Delay(int time);

// Hands `obj` to the first free ScoreObjs[] slot, which then owns it, and
// returns the slot index. See the TODO at the definition for the full-table
// case.
int Alloc_Object(ScoreAnimClass* obj);

class ArchiveReader;
class ArchiveWriter;
extern template void ScoreClass::Serialize<ArchiveWriter>(ArchiveWriter&);
extern template void ScoreClass::Serialize<ArchiveReader>(ArchiveReader&);

#endif  // CNC_RED_ALERT_RA_SCORE_H_
