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

// The special game options: switches that sit outside the rules and the
// scenario, set from the command line, the multiplayer setup dialogs and the
// developer options dialog. Also declares that dialog and the difficulty
// prompt shown before a campaign.

#ifndef CNC_RED_ALERT_RA_AI_SPECIAL_OPTIONS_H_
#define CNC_RED_ALERT_RA_AI_SPECIAL_OPTIONS_H_

#include "base/installed.h"
#include "ra/core/defines.h"

class ArchiveReader;
class ArchiveWriter;

namespace ra {

// The option flags of one game. A multiplayer host sends them to every player
// before the game starts, and a change made in the options dialog travels as an
// event, so every machine in a game holds the same values.
//
// The struct must stay trivially constructible because EventClass keeps one in
// its data union; that is why it has Reset() rather than a constructor or
// default member initializers.
struct SpecialOptions {
  // Field-wise saved-game state; read and write share this field list.
  template <class Archive>
  void Serialize(Archive& ar);

  // Sets every flag to the value a game starts with: ore grows and spreads,
  // everything else is off.
  void Reset();

  // The shroud grows back over explored ground, at the rules' ShroudRate.
  unsigned shroud_regrows : 1;

  // Special weapons recharge in one tick. A testing aid: it only has an effect
  // in a build with the cheat keys enabled.
  unsigned speed_build : 1;

  // This is the first launch after installing: play the introduction and go
  // straight into the campaign instead of asking what type of game to play.
  unsigned from_install : 1;

  // Capture the flag is on: each house gets a flag at its start position when
  // the scenario begins.
  unsigned capture_the_flag : 1;

  // Weapons are inert: they do no damage, so nothing ever dies.
  unsigned inert_weapons : 1;

  // Wheeled vehicles make a three-point turn when rotating in place.
  // TODO: Nothing in Red Alert reads this flag (Tiberian Dawn's unit code
  // does), so the options dialog's check box for it changes nothing.
  unsigned three_point_turns : 1;

  // Ore grows and spreads. The multiplayer dialogs set these together with the
  // rules' IsTGrowth and IsTSpread, and the game reads only the rules' copies;
  // these are kept because they are part of the saved game and of the
  // multiplayer game-options packet.
  unsigned ore_grows : 1;
  unsigned ore_spreads : 1;
};

// Runs the developer options dialog: one check box per changeable flag, with OK
// and Cancel. OK and Escape apply the boxes; Cancel discards them. The game
// keeps running behind the dialog in multiplayer, and the dialog also closes,
// applying nothing, when that game ends.
//
// The new flags are queued as an event, so they take effect on every machine on
// the same frame, and the map is redrawn on the way out.
void RunSpecialOptionsDialog();

// The pair of difficulty settings a campaign is played at.
struct DifficultyChoice {
  DiffType player;    // How hard the game is on the player's side.
  DiffType computer;  // How capable the computer's side is.

  bool operator==(const DifficultyChoice&) const = default;
};

// Returns the settings for a difficulty level from 0, easiest for the player
// (an easy game against a computer played at hard, which is the scale's own
// naming for the handicaps), to 4, hardest; 2 is normal on both sides. `level`
// must be in that range.
DifficultyChoice DifficultyForLevel(int level);

// Asks the player to pick a difficulty on a slider, blocks until OK or Return
// and returns the settings for the level chosen. The slider has all five
// levels when the rules' IsFineDifficulty is on, otherwise the three stops 0,
// 2 and 4. "is_expansion_mission" selects the shorter prompt that leaves out
// the sentences about the campaign.
DifficultyChoice PromptForDifficulty(bool is_expansion_mission = false);

extern template void SpecialOptions::Serialize<ArchiveWriter>(ArchiveWriter&);
extern template void SpecialOptions::Serialize<ArchiveReader>(ArchiveReader&);

// Returns the special command overrides that Game installed. CHECK-fails
// outside a Game's lifetime unless a test installed its own.
inline SpecialOptions& TheSpecialOptions() {
  return base::Installed<SpecialOptions>::Get();
}

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_AI_SPECIAL_OPTIONS_H_
