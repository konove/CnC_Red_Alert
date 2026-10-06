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

// The difficulty a campaign is played at, and the prompt that asks the player
// for it before the campaign starts.

#ifndef CNC_RED_ALERT_RA_SCREENS_DIFFICULTY_H_
#define CNC_RED_ALERT_RA_SCREENS_DIFFICULTY_H_

#include "ra/core/defines.h"

namespace ra {

// The pair of difficulty settings a campaign is played at.
struct DifficultyChoice {
  DifficultyLevel player;    // How hard the game is on the player's side.
  DifficultyLevel computer;  // How capable the computer's side is.

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
DifficultyChoice AskDifficulty(bool is_expansion_mission = false);

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_SCREENS_DIFFICULTY_H_
