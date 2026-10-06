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

// The special game flags: switches that sit outside the rules and the
// scenario, set from the command line, the multiplayer setup dialogs and the
// developer options dialog (ra/screens/special_options.h).

#ifndef CNC_RED_ALERT_RA_CORE_SPECIAL_FLAGS_H_
#define CNC_RED_ALERT_RA_CORE_SPECIAL_FLAGS_H_

#include "base/installed.h"

class ArchiveReader;
class ArchiveWriter;

namespace ra {

// The special flags of one game. A multiplayer host sends them to every player
// before the game starts, and a change made in the options dialog travels as an
// event, so every machine in a game holds the same values.
//
// The struct must stay trivially constructible because EventClass keeps one in
// its data union, so it has no constructor and no default member initializers:
// a value-initialized one has every flag off. Game starts its own from
// InitialSpecialFlags().
//
// The flags are `unsigned` bit-fields, eight bits in a four-byte struct,
// because EventClass and the multiplayer packets send the struct raw.
struct SpecialFlags {
  // Field-wise saved-game state; read and write share this field list.
  template <class Archive>
  void Serialize(Archive& ar);

  // The shroud grows back over explored ground, at the rules' ShroudRate.
  unsigned shadow_grows : 1;

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

  // Wheeled vehicles make a three-point turn when rotating in place. Nothing
  // in Red Alert reads this flag (Tiberian Dawn's unit code does); it is kept
  // because it is part of the saved game and of the multiplayer game-options
  // packet.
  unsigned three_point_turns : 1;

  // Ore grows and spreads. The multiplayer dialogs set these together with the
  // rules' IsTGrowth and IsTSpread, and the game reads only the rules' copies;
  // these are kept because they are part of the saved game and of the
  // multiplayer game-options packet.
  unsigned ore_grows : 1;
  unsigned ore_spreads : 1;
};

extern template void SpecialFlags::Serialize<ArchiveWriter>(ArchiveWriter&);
extern template void SpecialFlags::Serialize<ArchiveReader>(ArchiveReader&);

// Returns the flags a game starts with: ore grows and spreads, everything else
// is off. A function because a designated initializer would have to name every
// flag (-Wmissing-designated-field-initializers).
constexpr SpecialFlags InitialSpecialFlags() {
  SpecialFlags flags{};
  flags.ore_grows = true;
  flags.ore_spreads = true;
  return flags;
}

// Returns the special flags that Game installed. CHECK-fails outside a Game's
// lifetime unless a test installed its own.
inline SpecialFlags& TheSpecialFlags() {
  return base::Installed<SpecialFlags>::Get();
}

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_CORE_SPECIAL_FLAGS_H_
