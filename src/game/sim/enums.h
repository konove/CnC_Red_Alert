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

// File: The simulation enums both games share: armor, building size and
// animation state, cloaking, firing and movement results, speeds, production
// failures, damage results and the scenario direction.

#ifndef CNC_RED_ALERT_GAME_SIM_ENUMS_H_
#define CNC_RED_ALERT_GAME_SIM_ENUMS_H_

namespace game {

/**********************************************************************
**	The various armor types are best suited to defend against a limited
**	kind of warheads. The game strategy revolves around proper
**	combination of armor and weaponry. Each vehicle or building has armor
**	rated according to one of the following types.
*/
enum class ArmorType {
  ARMOR_NONE,      // Vulnerable to SA and HE.
  ARMOR_WOOD,      // Vulnerable to HE and Fire.
  ARMOR_ALUMINUM,  // Vulnerable to AP and SA.
  ARMOR_STEEL,     // Vulnerable to AP.
  ARMOR_CONCRETE   // Vulnerable to HE and AP.
};
using enum ArmorType;

/**********************************************************************
**	Each building has a predetermined size. These are the size numbers.
**	The trailing number in each name is the width and height (in that
**	order) of the building in cells.
*/
enum class BSizeType {
  BSIZE_NONE = -1,
  BSIZE_11 = 0,
  BSIZE_21 = 1,
  BSIZE_12 = 2,
  BSIZE_22 = 3,
  BSIZE_23 = 4,
  BSIZE_32 = 5,
  BSIZE_33 = 6,
  BSIZE_42 = 7,
  BSIZE_55 = 8
};
using enum BSizeType;

/**********************************************************************
**	These are the enumerated animation sequences that a building may
**	be processing. These serve to control the way that a building
**	appears.
*/
enum class BStateType {
  BSTATE_NONE = -1,
  BSTATE_CONSTRUCTION,  // Construction animation.
  BSTATE_IDLE,          // Idle animation.
  BSTATE_ACTIVE,        // Animation when building is "doing its thing".
  BSTATE_FULL,          // Special alternate active state.
  BSTATE_AUX1,          // Auxiliary animation.
  BSTATE_AUX2           // Auxiliary animation.
};
using enum BStateType;

/**********************************************************************
**	If an object can cloak, then it will be in one of these states.
**	For objects that cannot cloak, they will always be in the
**	UNCLOAKED state. This state controls how the object transitions between
**	cloaked and uncloaked conditions.
*/
enum class CloakType {
  UNCLOAKED,  // Completely visible (normal state).
  CLOAKING,   // In process of cloaking.
  CLOAKED,    // Completely cloaked (invisible).
  UNCLOAKING  // In process of uncloaking.
};
using enum CloakType;

/**********************************************************************
**	These return values are used when determine if firing is legal.
**	By examining this value it can be determined what should be done
**	to fix the reason why firing wasn't allowed.
*/
enum class FireErrorType {
  FIRE_OK,        // Weapon is allowed to fire.
  FIRE_AMMO,      // No ammo available to fire?
  FIRE_FACING,    // Not correctly facing target?
  FIRE_REARM,     // It is busy rearming?
  FIRE_ROTATING,  // Is it in process of rotating?
  FIRE_ILLEGAL,   // Is it targeting something illegal?
  FIRE_CANT,      // Is this unit one that cannot fire anything?
  FIRE_MOVING,    // Is it moving and not allowed to fire while moving?
  FIRE_RANGE,     // Is the target out of range?
  FIRE_CLOAKED,   // Is the shooter currently cloaked?
  FIRE_BUSY       // Is shooter currently doing something else?
};
using enum FireErrorType;

/**********************************************************************
**	Units that move can move at different speeds. These enumerate the
**	different speeds that a unit can move.
*/
enum class MPHType {
  MPH_IMMOBILE = 0,
  MPH_VERY_SLOW = 5,       //	2
  MPH_KINDA_SLOW = 6,      //	3
  MPH_SLOW = 8,            //	4
  MPH_SLOW_ISH = 10,       // 5
  MPH_MEDIUM_SLOW = 12,    // 6
  MPH_MEDIUM = 18,         // 9
  MPH_MEDIUM_FAST = 30,    // 12
  MPH_MEDIUM_FASTER = 35,  // 14
  MPH_FAST = 40,           // 16
  MPH_ROCKET = 60,         // 24
  MPH_VERY_FAST = 100,     // 40
  MPH_LIGHT_SPEED = 255    // 100
};
using enum MPHType;

/**********************************************************************
**	These are the response values when checking to see if an object
**	can enter or exist at a specified location. By examining this
**	return value, appropriate action may be chosen.
**	NOTE: If this changes, update the static array in Find_Path module.
*/
enum class MoveType {
  MOVE_OK,            // No blockage.
  MOVE_CLOAK,         // A cloaked blocking enemy object.
  MOVE_MOVING_BLOCK,  // Blocked, but only temporarily.
  MOVE_DESTROYABLE,   // Enemy unit or building is blocking.
  MOVE_TEMP,          // Blocked by friendly unit.
  MOVE_NO             // Strictly prohibited terrain.
};
using enum MoveType;

/**********************************************************************
**	These are the various return conditions that production may
**	produce.
*/
enum class ProdFailType {
  PROD_OK,       // Production request successful.
  PROD_LIMIT,    // Failed with production capacity limit reached.
  PROD_ILLEGAL,  // Failed because of illegal request.
  PROD_CANT      // Failed because unable to comply (busy or occupied).
};
using enum ProdFailType;

/**********************************************************************
**	When a unit gets damaged, the result of the damage is returned as
**	this type. It can range from no damage taken to complete destruction.
*/
enum class ResultType {
  RESULT_NONE,   // No damage was taken by the target.
  RESULT_LIGHT,  // Some damage was taken, but no state change occurred.
  RESULT_HALF,  // Damaged to below half strength (only returned on transition).
  RESULT_MAJOR,     // Damaged down to 1 hit point.
  RESULT_DESTROYED  // Damaged to complete destruction.
};
using enum ResultType;

/**********************************************************************
** These are the directional parameters for a scenario.
*/
enum class ScenarioDirType { SCEN_DIR_NONE = -1, SCEN_DIR_EAST, SCEN_DIR_WEST };
using enum ScenarioDirType;

/**********************************************************************
**	For units that are cloaking, these value specify the visual character
**	of the object.
*/
enum class VisualType {
  VISUAL_NORMAL,      // Completely visible -- normal.
  VISUAL_INDISTINCT,  // The edges shimmer and become indistinct.
  VISUAL_DARKEN,      // Color and texture is muted along with shimmering.
  VISUAL_SHADOWY,     // Body is translucent in addition to shimmering.
  VISUAL_RIPPLE,      // Just a ripple (true predator effect).
  VISUAL_HIDDEN       // Nothing at all is visible.
};
using enum VisualType;

}  // namespace game

#endif  // CNC_RED_ALERT_GAME_SIM_ENUMS_H_
