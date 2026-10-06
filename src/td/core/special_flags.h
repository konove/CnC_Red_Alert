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

// The special game flags: switches that sit outside the scenario, set from
// the command line, CONQUER.INI, the multiplayer setup dialogs and the
// developer options dialog (td/screens/special_options.h).

#ifndef CNC_RED_ALERT_TD_CORE_SPECIAL_FLAGS_H_
#define CNC_RED_ALERT_TD_CORE_SPECIAL_FLAGS_H_

#include "base/installed.h"

namespace td {

// The special flags of one game. A multiplayer host sends them to every player
// before the game starts, and a change made in the options dialog travels as an
// event, so every machine in a game holds the same values.
//
// The struct must stay trivially constructible because EventClass keeps one in
// its data union, so it has no constructor and no default member initializers:
// a value-initialized one has every flag off. Game starts its own from
// InitialSpecialFlags().
//
// The flags are `unsigned` bit-fields in declaration order because EventClass
// and the multiplayer packets send the struct raw.
struct SpecialFlags {
  // Enemy units show their health bars too.
  unsigned enemy_health_bars : 1;

  // The game is flagged easy, or difficult; neither is normal.
  unsigned easy : 1;
  unsigned difficult : 1;

  // Building takes no time. A testing aid.
  unsigned speed_build : 1;

  // The helipad can be built without the helicopter that normally comes with
  // it.
  unsigned separate_helipad : 1;

  // The defender has the advantage in combat: moving units neither deal nor
  // take as much damage.
  unsigned defender_advantage : 1;

  // Civilians and their buildings show their own names instead of the generic
  // ones.
  unsigned named_civilians : 1;

  // This is the first launch after installing: play the installation movie
  // and skip asking what type of game to play.
  unsigned from_install : 1;

  // Capture the flag is on: each house gets a flag at its start position when
  // the scenario begins.
  unsigned capture_the_flag : 1;

  // The player sees what the other human players have targeted.
  unsigned visible_target : 1;

  // Use the sound effects made by human voices.
  unsigned juvenile_sounds : 1;

  // The player's units return fire when fired upon, as the enemy's always do.
  unsigned smart_defense : 1;

  // Trees can be targeted.
  unsigned tree_target : 1;

  // The construction yard can undeploy back into an MCV.
  unsigned mcv_deploy : 1;

  // Was the monochrome-debug flag; reserved to keep the wire layout.
  unsigned retired_mono_flag : 1;

  // Weapons are inert: they do no damage, so nothing ever dies.
  unsigned inert_weapons : 1;

  // The path finder shows the route it is examining; for tracing its bugs.
  unsigned show_find_path : 1;

  // Every unit shows its health bar, selected or not.
  unsigned all_health_bars : 1;

  // Wheeled vehicles make a three-point turn when rotating in place.
  unsigned three_point_turns : 1;

  // Tiberium grows, spreads, and does both quickly.
  unsigned tiberium_grows : 1;
  unsigned tiberium_spreads : 1;
  unsigned tiberium_fast : 1;

  // Buildings are placed without the road pieces along their bottom edge.
  unsigned no_road_pieces : 1;

  // Units, infantry above all, scatter from an immediate threat.
  unsigned scatter : 1;

  // The dinosaur bonus scenarios are enabled.
  unsigned jurassic : 1;

  // The music's alternate variations are played.
  unsigned score_variations : 1;

  // Crushed infantry leave a mark on the ground.
  unsigned squish_marks : 1;

  // The map does not scroll while the mouse is over the options and sidebar
  // tabs.
  unsigned no_scroll_over_tabs : 1;

  // The game was started from WChat.
  unsigned from_wchat : 1;
};

// Returns the flags a game starts with: enemy health bars, the defender's
// advantage and fast growing and spreading Tiberium are on, everything else
// is off. A function because a designated initializer would have to name every
// flag (-Wmissing-designated-field-initializers).
constexpr SpecialFlags InitialSpecialFlags() {
  SpecialFlags flags{};
  flags.enemy_health_bars = true;
  flags.defender_advantage = true;
  flags.tiberium_grows = true;
  flags.tiberium_spreads = true;
  flags.tiberium_fast = true;
  return flags;
}

// Returns the special flags that Game installed. CHECK-fails outside a Game's
// lifetime unless a test installed its own.
inline SpecialFlags& TheSpecialFlags() {
  return base::Installed<SpecialFlags>::Get();
}

}  // namespace td

#endif  // CNC_RED_ALERT_TD_CORE_SPECIAL_FLAGS_H_
